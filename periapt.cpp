/******************************************************************************
 *    Periapt.cpp
 *
 *    Copyright (C) 2020 Andrew Durdin <me@andy.durdin.net>
 *
 *    Permission is hereby granted, free of charge, to any person obtaining
 *    a copy of this software and associated documentation files (the 
 *    "Software"), to deal in the Software without restriction, including 
 *    without limitation the rights to use, copy, modify, merge, publish, 
 *    distribute, sublicense, and/or sell copies of the Software, and to 
 *    permit persons to whom the Software is furnished to do so.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 *    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 *    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-
 *    INFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS 
 *    BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN 
 *    AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
 *    IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 *    THE SOFTWARE.
 *
 *****************************************************************************/

// BAD BUG: "File->Save" in DromEd 1.26 doesn't work after `script_load periapt`!
//
//     What the hell is going on there?!?

// BUG: frobbiness is determined by rendering.
//
//     What happes in cam_render_scene() determines if something is frobbable.
//     For example, returning immediately prevented the switch from being
//     frobbed off. This will need to be considered for the periapt: we don't
//     want the game thinking things in the peripat are frobbable!
//
//     It might be possible to work around this by attaching the camera (or
//     setting the relevant "is attached" flag) before we do our periapt
//     rendering; there might also be a better way.

// BUG: osm is not unloaded on `script_drop`.
//
//     I don't understand why; both empty.osm and echo.osm get unloaded ok
//     when dropped. So why does this osm not? Is there a spurious
//     LoadLibrary() somewhere?
//
// Ignore for now!
//
//     Once in the game, it gets loaded and unloaded with the mission okay,
//     so this only affects DromEd. I guess I can live with that for now;
//     I just have to be careful with my hooks.

// NOTE: Hooking cam_render_scene like this, renders UNDER the hud, but
//     over the player's weapons.

// NOTE: CoronaFrame() stores a pointer to camera location to a global.
//
//     CoronaFrame(), called by cam_render_scene(), stores a pointer to the
//     camera location into a global: `g_pCamLoc = &pPos->loc;`. I don't
//     know yet how g_pCamLoc is used, but this implies that I must not
//     pass the address of a stack variable into cam_render_scene(); instead
//     I'll have to modify the pos and then restore the original afterwards.
//     In addition, there may be other side effects caused by this, or other
//     globals used below cam_render_scene(), so must investigate further.

// NOTE: Cheat Engine hooks d3d9, as an example:
//
//     Cheat Engine hooks d3d9 by: first creating a window and a d3d9 device,
//     then reading the function pointers directly from the device's vtable, and
//     then releasing the device and destroying the window. Once it has the function
//     pointers, then it hooks them. See DXHookBase.cpp:31, :281, :1200
//
//     I don't know if I'll need to do the same. Right now I only think I'll
//     need the IDirect3DDevice9* handle in order to do some of my own drawing.
//     However, hooking d3d9 would definitely be one way of getting that handle
//     for myself!

// TODO: Clean up this directory structure a bunch.

// TODO: Sort out better printing (mprintf if possible, and all prefixed).

// TODO: Replace go script with make install?

// QUERY: Can we hook into NewDark's .mtl rendering?

// QUERY: Can NewDark's .mtl rendering do useful Z or stencil work for us?
//
//     No. Not a bit in there about either. Although the new property
//     `Renderer->Render Order` will let us turn _off_ z write for a
//     given object, that's not really what I'd want.
//
//     I think I'll have to keep digging into the PlayerArm() rendering
//     (both in source and disassembly) and try to understand what I'd
//     have to replicate in order to get a correct-animation-frame shape
//     rendering only into the stencil.
//
//     Alternatively, perhaps I could hook into the object rendering and
//     only if it's the player arm, and the periapt, then turn on stencil
//     writes? But that would draw the entire arm and periapt into the
//     stencil buffer! Also not good! So I need to do a bunch more work
//     to figure out how this is going to work.

#include "Script.h"
#include "ScriptModule.h"

#include <lg/scrservices.h>

#include <cassert>
#include <cmath>
#include <cstring>
#include <new>
#include <exception>
#include <string>
#include <strings.h>
#include <d3d9.h>

#include "t2types.h"
#include "bypass.h"

using namespace std;

#define HOOKS_SPEW 0
#define PREFIX "periapt: "

// FIXME: temp
void readMem(void *addr, DWORD len)
{
    HANDLE hProcess = GetCurrentProcess();
    // HMODULE hModule = GetModuleHandle(NULL);
    // TODO: we probably can't see printfs, right?
    // if (g_pfnMPrintf) g_pfnMPrintf(...);

    UCHAR bytes[256];
    if (len > 256) len = 256;

    SIZE_T bytesRead = 0;
    BOOL ok = ReadProcessMemory(hProcess, (LPCVOID)(addr), bytes, len, &bytesRead);
    if (ok) {
        printf("Bytes at %08x:\n", (unsigned int)addr);
        for (int i=0; i<(int)len; ++i) {
            if (i%16 == 0) printf("%08x (+%02x)", ((unsigned int)addr+i), (unsigned int)i);
            printf(" %02X", bytes[i]);
            if (i%16 == 15) printf("\n");
        }
        if (len%16 != 0) printf("\n");
        printf("\n");
    }
}


/*** Exe identity and information ***/

#define ExeSignatureBytesMax 32
#define ExeSignatureFixupsMax 8
enum ExeIdentity {
    ExeThief_v126 = 0,
    ExeDromEd_v126 = 1,
    ExeThief_v127 = 2,
    ExeDromEd_v127 = 3,

    ExeIdentityCount,
    ExeIdentityUnknown = -1,
};
enum {
    ExeFixupsEnd = 0xff,
};
struct ExeSignature {
    const char *name;
    const char *version;
    DWORD offset;
    DWORD size;
    UCHAR bytes[ExeSignatureBytesMax];
    UCHAR fixups[ExeSignatureFixupsMax];
};
static const ExeSignature ExeSignatureTable[ExeIdentityCount] = {
    {
        "Thief2", "ND 1.26", 0x001bc7a0UL, 31,
        {
            0x83,0xec,0x28,0xdd,0x05,0xd8,0xde,0x38,
            0x00,0x53,0x56,0xdd,0x54,0x24,0x24,0xdd,
            0x05,0xe0,0xde,0x38,0x00,0x8b,0xf0,0x8b,
            0x06,0xdd,0x54,0x24,0x1c,0xd9,0x05,
        },
        { 5, 17, ExeFixupsEnd },
    },
    {
        "DromEd", "ND 1.26", 0x00286960UL, 31,
        {
            0x83,0xec,0x28,0xdd,0x05,0xa8,0xdf,0x4a,
            0x00,0x53,0x56,0xdd,0x54,0x24,0x24,0xdd,
            0x05,0xb0,0xdf,0x4a,0x00,0x8b,0xf0,0x8b,
            0x06,0xdd,0x54,0x24,0x1c,0xd9,0x05,
        },
        { 5, 17, ExeFixupsEnd },
    },
    {
        "Thief2", "ND 1.27", 0x001bd820UL, 31,
        {
            0x83,0xec,0x28,0xdd,0x05,0xd8,0xee,0x38,
            0x00,0x53,0x56,0xdd,0x54,0x24,0x24,0xdd,
            0x05,0xe0,0xee,0x38,0x00,0x8b,0xf0,0x8b,
            0x06,0xdd,0x54,0x24,0x1c,0xd9,0x05,
        },
        { 5, 17, ExeFixupsEnd },
    },
    {
        "DromEd", "ND 1.27", 0x002895c0UL, 31,
        {
            0x83,0xec,0x28,0xdd,0x05,0xa8,0x1f,0x4b,
            0x00,0x53,0x56,0xdd,0x54,0x24,0x24,0xdd,
            0x05,0xb0,0x1f,0x4b,0x00,0x8b,0xf0,0x8b,
            0x06,0xdd,0x54,0x24,0x1c,0xd9,0x05,
        },
        { 5, 17, ExeFixupsEnd },
    },
};

bool DoesSignatureMatch(const ExeSignature *sig) {
    HANDLE hProcess = GetCurrentProcess();
    HMODULE hModule = GetModuleHandle(NULL);
    DWORD baseAddress = (DWORD)hModule;
    // Read memory
    UCHAR mem[ExeSignatureBytesMax];
    SIZE_T bytesRead = 0;
    bool ok = ReadProcessMemory(hProcess,
        (LPCVOID)(baseAddress+sig->offset), mem, ExeSignatureBytesMax, &bytesRead);
    if (!ok || bytesRead != ExeSignatureBytesMax) return false;
    // Apply fixups
    for (int i=0; i<ExeSignatureFixupsMax; ++i) {
        UCHAR o = sig->fixups[i];
        if (o == ExeFixupsEnd) break;
        if (o >= (ExeSignatureBytesMax - sizeof(DWORD))) {
#ifndef NDEBUG
            printf("Fixup outwith signature bounds.\n");
#endif
            return false;
        }
        // Subtract base address
        DWORD v;
        memcpy(&v, &mem[o], sizeof(DWORD));
        v -= baseAddress;
        memcpy(&mem[o], &v, sizeof(DWORD));
    }
    // Compare the signature
    if (sig->size > ExeSignatureBytesMax) {
#ifndef NDEBUG
        printf("Signature size too large.\n");
#endif
        return false;
    }
    int r = memcmp(mem, sig->bytes, sig->size);
    return (r == 0);
}

ExeIdentity IdentifyExe() {
    for (int i=0; i<ExeIdentityCount; ++i) {
        const ExeSignature *sig = &(ExeSignatureTable[i]);
        if (DoesSignatureMatch(sig)) {
            return static_cast<ExeIdentity>(i);
        }
    }
    return ExeIdentityUnknown;
}

/*** Game info ***/

// Periapt properties:
// TODO: on BeginScript(), load these with GetData();
//       when setting them, save them with SetData();
static struct {
    bool dualRender;
    t2vector dualOffset;
    bool dualCull;
    float dualCullLeft, dualCullTop, dualCullRight, dualCullBottom;
    bool depthCull;
    float depthCullDistance;
} g_Periapt = { };

// Functions to be called:
t2position* __cdecl (*t2_ObjPosGet)(t2id obj);
bool __cdecl (*t2_SphrSphereInWorld)(t2location *center_loc, float radius);

// Data to be accessed:
IDirect3DDevice9 **t2_d3d9device_ptr;
void *t2_modelnameprop_ptr;
t2position *t2_portal_camera_pos_ptr;
t2cachedmaterial *t2_matcache_ptr;

struct t2_modelname_vtable {
    DWORD reserved0;
    DWORD reserved1;
    DWORD reserved2;
    DWORD reserved3;

    DWORD reserved4;
    DWORD reserved5;
    DWORD reserved6;
    DWORD reserved7;

    DWORD reserved8;
    DWORD reserved9;
    DWORD reserved10;
    DWORD reserved11;

    DWORD reserved12;
    DWORD reserved13;
    DWORD reserved14;
    DWORD reserved15;

    DWORD reserved16;
    DWORD reserved17;
    void __stdcall (*Set)(void* thisptr, t2id obj, const char* name);
    DWORD reserved19;

    BOOL __stdcall (*Get)(void* thisptr, t2id obj, const char** pName);
};

const char* t2_modelname_Get(t2id obj) {
    static char buf[256];
    buf[0] = 0;
    if (t2_modelnameprop_ptr) {
        void* modelnameprop = *(void**)t2_modelnameprop_ptr;
        t2_modelname_vtable *vtable = *(t2_modelname_vtable**)modelnameprop;
        if (vtable) {
            if (vtable->Get) {
                const char *name = NULL;
                if (vtable->Get(modelnameprop, obj, &name)) {
                    strncpy(buf, name, 256);
                    buf[255] = 0;
                }
            }
        }
    }
    return buf;
}

struct GameInfo {
    // Functions to be hooked:
    DWORD cam_render_scene;
    DWORD cam_render_scene_preamble;
    DWORD cD8Renderer_Clear;
    DWORD cD8Renderer_Clear_preamble;
    DWORD dark_render_overlays;
    DWORD dark_render_overlays_preamble;
    DWORD rendobj_render_object;
    DWORD rendobj_render_object_preamble;
    DWORD explore_portals;
    DWORD explore_portals_preamble;
    DWORD initialize_first_region_clip;
    DWORD initialize_first_region_clip_preamble;
    DWORD initialize_first_region_clip_resume;
    DWORD mm_hardware_render;
    DWORD mm_hardware_render_preamble;
    DWORD mDrawTriangleLists;
    DWORD mDrawTriangleLists_preamble;
    DWORD mDrawTriangleLists_resume;
    // Functions to be called:
    DWORD ObjPosGet;
    DWORD ObjPosSetLocation;
    DWORD ComputeCellForLocation;
    DWORD SphrSphereInWorld;
    // Data to be accessed:
    DWORD d3d9device_ptr;
    DWORD modelnameprop;
    DWORD portal_camera_pos;
    DWORD matcache;
};

static GameInfo GameInfoTable = {};

static const GameInfo PerIdentityGameTable[ExeIdentityCount] = {
    // ExeThief_v126
    {
        0x001bc7a0UL, 9,    // cam_render_scene
        0x0020ce80UL, 6,    // cD8Renderer_Clear
        0, 0, /* TODO */    // dark_render_overlays
        0, 0, /* TODO */    // rendobj_render_object
        0, 0, /* TODO */    // explore_portals
        0, 0, /* TODO */    // initialize_first_region_clip
        0,                  // initialize_first_region_clip_resume
        0, 0, /* TODO */    // mm_hardware_render
        0, 0, /* TODO */    // mDrawTriangleLists
        0,                  // mDrawTriangleLists_resume
        0,                  // ObjPosGet
        0,                  // ObjPosSetLocation
        0, /* TODO */       // ComputeCellForLocation
        0, /* TODO */       // SphrSphereInWorld
        0x005d8118UL,       // d3d9device_ptr
        0,                  // modelnameprop
        0,                  // portal_camera_pos
        0,                  // matcache
    },
    // ExeDromEd_v126
    {
        0x00286960UL, 9,    // cam_render_scene
        0x002e62a0UL, 6,    // cD8Renderer_Clear
        0, 0, /* TODO */    // dark_render_overlays
        0, 0, /* TODO */    // rendobj_render_object
        0, 0, /* TODO */    // explore_portals
        0, 0, /* TODO */    // initialize_first_region_clip
        0,                  // initialize_first_region_clip_resume
        0, 0, /* TODO */    // mm_hardware_render
        0, 0, /* TODO */    // mDrawTriangleLists
        0,                  // mDrawTriangleLists_resume
        0,                  // ObjPosGet
        0,                  // ObjPosSetLocation
        0, /* TODO */       // ComputeCellForLocation
        0, /* TODO */       // SphrSphereInWorld
        0x016e7b50UL,       // d3d9device_ptr
        0,                  // modelnameprop
        0,                  // portal_camera_pos
        0,                  // matcache
    },
    // ExeThief_v127
    {
        0x001bd820UL, 9,    // cam_render_scene
        0x0020dff0UL, 6,    // cD8Renderer_Clear
        0x00058330UL, 6,    // dark_render_overlays
        0x001c2870UL, 6,    // rendobj_render_object
        0x000cc0f0UL, 6,    // explore_portals
        0x000cda79UL, 5,    // initialize_first_region_clip
        0x000cda9eUL,       // initialize_first_region_clip_resume
        0x0020eaf0UL, 8,    // mm_hardware_render
        0x0020ce47UL, 6,    // mDrawTriangleLists
        0x0020ce4fUL,       // mDrawTriangleLists_resume
        0,                  // ObjPosGet
        0,                  // ObjPosSetLocation
        0x000e06f0UL,       // ComputeCellForLocation
        0x001cfc60UL,       // SphrSphereInWorld
        0x005d915cUL,       // d3d9device_ptr
        0x005ce4d8UL,       // modelnameprop
        0x00460bf0UL,       // portal_camera_pos
        0,                  // matcache
    },
    // ExeDromEd_v127
    {
        0x002895c0UL, 9,    // cam_render_scene
        0x002e8e60UL, 6,    // cD8Renderer_Clear
        0x00068750UL, 6,    // dark_render_overlays
        0x00290950UL, 6,    // rendobj_render_object
        0x001501e0UL, 6,    // explore_portals
        0x00151d09UL, 5,    // initialize_first_region_clip
        0x00151d2eUL,       // initialize_first_region_clip_resume
        0x002e9ab0UL, 8,    // mm_hardware_render
        0x002e7a38UL, 6,    // mDrawTriangleLists
        0x002e7a40UL,       // mDrawTriangleLists_resume
        0x001e4680UL,       // ObjPosGet
        0x001e49e0UL,       // ObjPosSetLocation
        0x00170240UL,       // ComputeCellForLocation
        0x0029f520UL,       // SphrSphereInWorld
        0x016ebce0UL,       // d3d9device_ptr
        0x016e0f84UL,       // modelnameprop
        0x0140216cUL,       // portal_camera_pos
        0x01660ba0UL,       // matcache
    },
};

static void fixup_addr(DWORD* address, DWORD baseAddress) {
    *address = (*address + baseAddress);
}

void LoadGameInfoTable(ExeIdentity identity) {
    GameInfoTable = PerIdentityGameTable[identity];

    DWORD base = (DWORD)GetModuleHandle(NULL);
    fixup_addr(&GameInfoTable.cam_render_scene, base);
    fixup_addr(&GameInfoTable.cD8Renderer_Clear, base);
    fixup_addr(&GameInfoTable.dark_render_overlays, base);
    fixup_addr(&GameInfoTable.rendobj_render_object, base);
    fixup_addr(&GameInfoTable.explore_portals, base);
    fixup_addr(&GameInfoTable.initialize_first_region_clip, base);
    fixup_addr(&GameInfoTable.initialize_first_region_clip_resume, base);
    fixup_addr(&GameInfoTable.mm_hardware_render, base);
    fixup_addr(&GameInfoTable.mDrawTriangleLists, base);
    fixup_addr(&GameInfoTable.mDrawTriangleLists_resume, base);
    fixup_addr(&GameInfoTable.ObjPosGet, base);
    fixup_addr(&GameInfoTable.ObjPosSetLocation, base);
    fixup_addr(&GameInfoTable.ComputeCellForLocation, base);
    fixup_addr(&GameInfoTable.SphrSphereInWorld, base);
    fixup_addr(&GameInfoTable.d3d9device_ptr, base);
    fixup_addr(&GameInfoTable.modelnameprop, base);
    fixup_addr(&GameInfoTable.portal_camera_pos, base);
    fixup_addr(&GameInfoTable.matcache, base);

    t2_ObjPosGet = (t2position*(*)(t2id))GameInfoTable.ObjPosGet;
    ADDR_ObjPosSetLocation = GameInfoTable.ObjPosSetLocation;
    ADDR_ComputeCellForLocation = (uint32_t)GameInfoTable.ComputeCellForLocation;
    t2_SphrSphereInWorld = (bool __cdecl (*)(t2location*,float))GameInfoTable.SphrSphereInWorld;
    t2_d3d9device_ptr = (IDirect3DDevice9**)GameInfoTable.d3d9device_ptr;
    t2_modelnameprop_ptr = (void*)GameInfoTable.modelnameprop;
    t2_portal_camera_pos_ptr = (t2position*)GameInfoTable.portal_camera_pos;
    t2_matcache_ptr = (t2cachedmaterial*)GameInfoTable.matcache;
    RESUME_initialize_first_region_clip = GameInfoTable.initialize_first_region_clip_resume;
    RESUME_mDrawTriangleLists = GameInfoTable.mDrawTriangleLists_resume;

#if HOOKS_SPEW
    printf("periapt: cam_render_scene = %08x\n", (unsigned int)GameInfoTable.cam_render_scene);
    printf("periapt: cD8Renderer_Clear = %08x\n", (unsigned int)GameInfoTable.cD8Renderer_Clear);
    printf("periapt: dark_render_overlays = %08x\n", (unsigned int)GameInfoTable.dark_render_overlays);
    printf("periapt: rendobj_render_object = %08x\n", (unsigned int)GameInfoTable.rendobj_render_object);
    printf("periapt: explore_portals = %08x\n", (unsigned int)GameInfoTable.explore_portals);
    printf("periapt: initialize_first_region_clip = %08x\n", (unsigned int)GameInfoTable.initialize_first_region_clip);
    printf("periapt: initialize_first_region_clip_resume = %08x\n", (unsigned int)GameInfoTable.initialize_first_region_clip_resume);
    printf("periapt: mm_hardware_render = %08x\n", (unsigned int)GameInfoTable.mm_hardware_render);
    printf("periapt: mDrawTriangleLists = %08x\n", (unsigned int)GameInfoTable.mDrawTriangleLists);
    printf("periapt: mDrawTriangleLists_resume = %08x\n", (unsigned int)GameInfoTable.mDrawTriangleLists_resume);
    printf("periapt: t2_ObjPosGet = %08x\n", (unsigned int)t2_ObjPosGet);
    printf("periapt: ADDR_ObjPosSetLocation = %08x\n", (unsigned int)ADDR_ObjPosSetLocation);
    printf("periapt: ADDR_ComputeCellForLocation = %08x\n", (unsigned int)ADDR_ComputeCellForLocation);
    printf("periapt: t2_SphrSphereInWorld = %08x\n", (unsigned int)t2_SphrSphereInWorld);
    printf("periapt: t2_d3d9device_ptr = %08x\n", (unsigned int)t2_d3d9device_ptr);
    printf("periapt: t2_modelnameprop_ptr = %08x\n", (unsigned int)t2_modelnameprop_ptr);
    printf("periapt: t2_portal_camera_pos_ptr = %08x\n", (unsigned int)t2_portal_camera_pos_ptr);
    printf("periapt: t2_matcache_ptr = %08x\n", (unsigned int)t2_matcache_ptr);
#endif
}

/*** Hooks and crooks ***/

#if HOOKS_SPEW
#define hooks_spew(...) printf("periapt: " __VA_ARGS__)
#else
#define hooks_spew(...) ((void)0)
#endif

void enable_hooks() {
    bypass_enable = 1;
    printf("hooks enabled\n");
}

void disable_hooks() {
    bypass_enable = 0;
    printf("hooks disabled\n");
}

void patch_jmp(uint32_t jmp_address, uint32_t dest_address) {
    const uint8_t jmp = 0xE9;
    int32_t offset = (dest_address - (jmp_address+5));
    hooks_spew("  offset: %08x (%d)\n", offset, offset);
    hooks_spew("  memcpy %08x, %08x, %u\n", jmp_address, (uint32_t)&jmp, 1);
    memcpy((void *)jmp_address, &jmp, 1);
    hooks_spew("  memcpy %08x, %08x, %u\n", jmp_address+1, (uint32_t)&offset, 4);
    memcpy((void *)(jmp_address+1), &offset, 4);
}

void install_hook(bool *hooked, uint32_t target, uint32_t trampoline, uint32_t bypass, uint32_t size) {
    if (! *hooked) {
        *hooked = true;
        hooks_spew("hooking target %08x, trampoline %08x, bypass %08x, size %u\n", target, trampoline, bypass, size);
        DWORD targetProtection, trampolineProtection;
        VirtualProtect((void *)target, size, PAGE_EXECUTE_READWRITE, &targetProtection);
        VirtualProtect((void *)trampoline, size+5, PAGE_EXECUTE_READWRITE, &trampolineProtection);
#if HOOKS_SPEW
        readMem((void *)target, 32);
        readMem((void *)ORIGINAL_cam_render_scene, 32);
        readMem((void *)trampoline, 32);
        readMem((void *)bypass, 32);
#endif
        hooks_spew("memcpy %08x, %08x, %u\n", trampoline, target, size);
        memcpy((void *)trampoline, (void *)target, size);
        hooks_spew("patch_jmp %08x, %08x\n", trampoline+size, target+size);
        patch_jmp(trampoline+size, target+size);
        hooks_spew("patch_jmp %08x, %08x\n", target, bypass);
        patch_jmp(target, bypass);
        VirtualProtect((void *)target, size, targetProtection, NULL);
        VirtualProtect((void *)trampoline, size+5, trampolineProtection, NULL);
#if HOOKS_SPEW
        readMem((void *)target, 32);
        readMem((void *)ORIGINAL_cam_render_scene, 32);
        readMem((void *)trampoline, 32);
        readMem((void *)bypass, 32);
#endif
        hooks_spew("hook complete\n");
    }
}

void remove_hook(bool *hooked, uint32_t target, uint32_t trampoline, uint32_t bypass, uint32_t size) {
    (void)bypass; // Unused
    if (*hooked) {
        *hooked = false;
        hooks_spew("unhooking target %08x, trampoline %08x, bypass %08x, size %u\n", target, trampoline, bypass, size);
        DWORD targetProtection;
        VirtualProtect((void *)target, size, PAGE_EXECUTE_READWRITE, &targetProtection);
#if HOOKS_SPEW
        readMem((void *)target, 32);
        readMem((void *)ORIGINAL_cam_render_scene, 32);
        readMem((void *)trampoline, 32);
        readMem((void *)bypass, 32);
#endif
        hooks_spew("memcpy %08x, %08x, %u\n", target, trampoline, size);
        memcpy((void *)target, (void *)trampoline, size);
#if HOOKS_SPEW
        readMem((void *)target, 32);
        readMem((void *)ORIGINAL_cam_render_scene, 32);
        readMem((void *)trampoline, 32);
        readMem((void *)bypass, 32);
#endif
        VirtualProtect((void *)target, size, targetProtection, NULL);
        hooks_spew("unhook complete\n");
    }
}

static struct {
    bool isTransitioning;
    bool isRenderingDual;
    bool isDrawingOverlays;
    bool dontClearTarget;
    bool dontClearStencil;
    bool readyToDrawViewmodel;
    int partIndex;
    IDirect3DBaseTexture9 *crystalTexture;
    IDirect3DBaseTexture9 *overlayTexture;
    IDirect3DBaseTexture9 *part1Texture;
    IDirect3DBaseTexture9 *part2Texture;
} g_State = {};

extern "C"
void __cdecl HOOK_cam_render_scene(t2position* pos, double zoom) {
    // TODO: for developing the transition, we are always transitioning
    // g_State.isTransitioning = TRUE;

    // if (g_State.isTransitioning) {
    //     g_State.dontClearTarget = false;
    //     g_State.dontClearStencil = true;
    // } else {
        g_State.dontClearTarget = false;
        g_State.dontClearStencil = false;
    // }

    // Render the scene normally.
    ORIGINAL_cam_render_scene(pos, zoom);
    if (!g_Periapt.dualRender) return;

#if HOOKS_SPEW
    static bool spewed = false;
    if (! spewed) {
        spewed = true;
        printf("periapt: t2_d3d9device_ptr = %08x\n", (unsigned int)t2_d3d9device_ptr);
        if (t2_d3d9device_ptr) printf("periapt: d3d9device = %08x\n", (unsigned int)(*t2_d3d9device_ptr));
    }
#endif

    if (t2_d3d9device_ptr) {
        IDirect3DDevice9* device = *t2_d3d9device_ptr;

        // We modify the pos parameter we're given (and later restore it) because
        // CoronaFrame (at least) stores a pointer to the loc! So if we passed
        // in a local variable, that would end up pointing to random stack gibberish.
        // It might not matter--I don't know--but safer not to risk it.
        t2position originalPos = *pos;
        // Move the camera into the otherworld:
        pos->loc.vec.x += g_Periapt.dualOffset.x;
        pos->loc.vec.y += g_Periapt.dualOffset.y;
        pos->loc.vec.z += g_Periapt.dualOffset.z;
        // If we move the location, then we ought to cancel the cell+hint metadata:
        pos->loc.cell = -1;
        pos->loc.hint = -1;
        // And compute the cell and hint for it (must have them for collision testing).
        CALL_ComputeCellForLocation(&pos->loc);
        bool isCameraInWorld = (pos->loc.cell != -1);

        // TODO: need to:
        //       a) make this a much better check. right now it fails when walking
        //          diagonally along a ruined wall, when the head dips into the wall.
        //          also right now it doesnt check foot position or anything.
        //          basically, there is "camera okay" and "player okay", and we
        //          must only render the dual if camera okay, but also only allow
        //          translocation if BOTH are okay.
        //       b) make this validity check result available to scripts so that
        //          they can enable/disable the translocation and swap out
        //          the viewmodel for the occluded one.
        // const float PLAYER_RADIUS = 1.2;
        // bool isValidPosition = t2_SphrSphereInWorld(&pos->loc, PLAYER_RADIUS);

        if (isCameraInWorld) {
            // Render this second pass of the world only where the stencil value
            // is 1.
            device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
            device->SetRenderState(D3DRS_STENCILREF, 0x01);
            device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
            device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
            device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
            device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
            device->SetRenderState(D3DRS_STENCILMASK, 0xFFFFFFFF);
            device->SetRenderState(D3DRS_STENCILWRITEMASK, 0xFFFFFFFF);

            // The original cam_render_scene will clear target+zbuffer+stencil
            // before drawing. We want to keep the previous scene render, so we
            // prevent clearing the target. And we want to keep what we've just
            // put in the stencil, so we prevent clearing the stencil too.
            g_State.dontClearTarget = true;
            g_State.dontClearStencil = true;

            // Calling this again might have undesirable side effects; needs research.
            // Although right now I'm not seeing frobbiness being affected... not a
            // very conclusive test ofc.
            g_State.isRenderingDual = true;
            ORIGINAL_cam_render_scene(pos, zoom);
            g_State.isRenderingDual = false;
            g_State.dontClearTarget = false;
            g_State.dontClearStencil = false;

            device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        }

        // Restore the original position.
        *pos = originalPos;
    }
}

extern "C"
void __stdcall HOOK_cD8Renderer_Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    if (g_State.dontClearTarget)
        Flags &= ~D3DCLEAR_TARGET;
    if (g_State.dontClearStencil)
        Flags &= ~D3DCLEAR_STENCIL;
    ORIGINAL_cD8Renderer_Clear(Count, pRects, Flags, Color, Z, Stencil);
}

extern "C"
void __cdecl HOOK_dark_render_overlays(void) {
    // Just don't render overlays in the dual view.
    if (g_State.isRenderingDual)
        return;

    g_State.isDrawingOverlays = true;
    ORIGINAL_dark_render_overlays();
    g_State.isDrawingOverlays = false;
}

// TODO: we can ditch this hook -- we just skip rendering overlays in the dual instead.
extern "C"
void __cdecl HOOK_rendobj_render_object(t2id obj, UCHAR* clut, ULONG fragment) {
    // Check if we are rendering the periapt object.
    bool isPeriapt = false;
    if (g_Periapt.dualRender && g_State.isDrawingOverlays) {
        const char* name = t2_modelname_Get(obj);
        isPeriapt = (name && (stricmp(name, "periaptv") == 0));
    }

    if (isPeriapt && g_State.isRenderingDual) {
        // Skip the viewmodel in the dual pass.
    } else {
        ORIGINAL_rendobj_render_object(obj, clut, fragment);
    }
}

extern "C"
void __cdecl HOOK_explore_portals(t2portalcell* cell) {
    if (g_Periapt.depthCull) {
        // Skip rendering far portals (that should be fogged out):

        // Get the camera position and facing.
        t2position pos = *t2_portal_camera_pos_ptr;
        // Build the camera's forward unit vector from its facing.
        float yaw = pos.fac.z*3.1416f/32768.0f;
        float pitch = -pos.fac.y*3.1416f/32768.0f;
        t2vector look;
        look.x = cos(yaw)*cos(pitch);
        look.y = sin(yaw)*cos(pitch);
        look.z = sin(pitch);
        // Get a vector from the camera position to the center of
        // the cell's bounding sphere.
        t2vector center;
        center.x = cell->sphere_center.x - pos.loc.vec.x;
        center.y = cell->sphere_center.y - pos.loc.vec.y;
        center.z = cell->sphere_center.z - pos.loc.vec.z;
        // Find the distance (parallel to the forward vector) to the
        // cell's center, by projecting the center vector onto the
        // forward vector.
        float dot = center.x*look.x + center.y*look.y + center.z*look.z;
        // Find the distance to the near edge of the cell's bounding sphere.
        float cell_near_dist = dot - cell->sphere_radius;
        if (cell_near_dist > g_Periapt.depthCullDistance)
            return;
    }

    ORIGINAL_explore_portals(cell);
}

extern "C"
void __cdecl HOOK_initialize_first_region_clip(int w, int h, t2clipdata *clip) {
    int l=0, r=w, t=0, b=h;

#if 0 // TEMP: disable clipping while developing the transition
    if (g_State.isRenderingDual
    && g_Periapt.dualCull) {
        // Adjust the portal clipping rectangle for the second render pass.
        // For the first pass l,t = 0,0 and r,b = w,h (screen dimensions);
        // but for the second pass we want to only include portals that would
        // be rendered in the periapt viewmodel.
        //
        // For now, let's just say "draw the right half of the screen only":
        l = (int)(w*g_Periapt.dualCullLeft);
        r = (int)(w*g_Periapt.dualCullRight);
        t = (int)(h*g_Periapt.dualCullTop);
        b = (int)(h*g_Periapt.dualCullBottom);
    }
#endif
    clip->l = l<<16;
    clip->t = t<<16;
    clip->r = r<<16;
    clip->b = b<<16;
    clip->tl = clip->l + clip->t;
    clip->tr = clip->r - clip->t;
    clip->bl = clip->l - clip->b;
    clip->br = clip->r + clip->b;
}

extern "C"
void __cdecl HOOK_mm_hardware_render(t2mmsmodel *m) {
    if (g_State.isDrawingOverlays
    && g_Periapt.dualRender
    && !g_State.isRenderingDual) {
        char *base = ((char *)m) + m->smatr_off;
        // Find the textures for all parts of the periapt.
        g_State.crystalTexture = NULL;
        g_State.overlayTexture = NULL;
        g_State.part1Texture = NULL;
        g_State.part2Texture = NULL;
        for (int i=0; i<m->smatrs; ++i) {
            char *name;
            uint32_t handle;
            if (m->version==1) {
                t2smatr_v1 *smatrs = (t2smatr_v1*)base;
                name = smatrs[i].name;
                handle = smatrs[i].handle;
            } else {
                t2smatr_v2 *smatrs = (t2smatr_v2*)base;
                name = smatrs[i].name;
                handle = smatrs[i].handle;
            }

            if (handle) {
                int32_t i = ((t2material*)handle)->cache_index;
                // The 'cache_index' is a pointer to something else before the
                // texture is loaded in to d3d. I am guessing at the range of
                // cached material handles here, but certainly no pointer will
                // be in this range!
                if (i>=0 && i<65536) {
                    IDirect3DBaseTexture9 *tex = t2_matcache_ptr[i].d3d_texture;
                    if (stricmp(name, "pericry1.png")==0) {
                        g_State.crystalTexture = tex;
                    } else if (stricmp(name, "pericry2.png")==0) {
                        g_State.overlayTexture = tex;
                    } else if (stricmp(name, "peri1.png")==0) {
                        g_State.part1Texture = tex;
                    } else if (stricmp(name, "peri2.png")==0) {
                        g_State.part2Texture = tex;
                    }
                }
            }
        }
    }

    // We don't want to do any drawing into the stencil until all textures
    // are ready and loaded into the cache.
    g_State.readyToDrawViewmodel =
        (g_State.crystalTexture && g_State.overlayTexture
        && g_State.part1Texture && g_State.part2Texture);
    g_State.partIndex = 0;
    
    ORIGINAL_mm_hardware_render(m);
}

// We save transformed vertices here every frame, so we can draw it our way.
#define PERIAPT_PART_COUNT  4
#define PERIAPT_CRYSTAL 0
#define PERIAPT_PART_1  1
#define PERIAPT_PART_2  2
#define PERIAPT_IGNORE  -1
#define PERIAPT_PRIMITIVE_COUNT  256
// For the viewmodel, FVF is D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1
struct primitive {
    float x,y,z,w;
    D3DCOLOR diffuse;
    D3DCOLOR specular;
    float u,v;
} __attribute__((aligned(4)));
static struct primitive PeriaptPrimitive[3][3*PERIAPT_PRIMITIVE_COUNT];
static UINT PeriaptPrimitiveCount[3];

extern "C"
int __cdecl HOOK_mDrawTriangleLists(IDirect3DDevice9 *device, D3DPRIMITIVETYPE PrimitiveType,
    UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) {

    if (!g_State.isDrawingOverlays
    || !g_State.readyToDrawViewmodel
    || !g_Periapt.dualRender) {
        // Just do the original draw.
        return device->DrawPrimitiveUP(PrimitiveType, PrimitiveCount,
            pVertexStreamZeroData, VertexStreamZeroStride);
    }

    // Just some sanity checks.
    assert(PrimitiveType==D3DPT_TRIANGLELIST);
    assert(PrimitiveCount<=PERIAPT_PRIMITIVE_COUNT);
    assert(VertexStreamZeroStride==sizeof(struct primitive));

    // Save the transformed vertices for all the parts, so we can draw them in
    // the order that we need.
    IDirect3DBaseTexture9 *texture;
    device->GetTexture(0, &texture);
    int partIndex = PERIAPT_IGNORE;
    if (texture==g_State.crystalTexture) {
        partIndex = PERIAPT_CRYSTAL;
    } else if (texture==g_State.overlayTexture) {
        // ignore it
    } else if (texture==g_State.part1Texture) {
        partIndex = PERIAPT_PART_1;
    } else if (texture==g_State.part2Texture) {
        partIndex = PERIAPT_PART_2;
    }
    if (partIndex!=PERIAPT_IGNORE) {
        memcpy(PeriaptPrimitive[partIndex], pVertexStreamZeroData,
            3*PrimitiveCount*sizeof(struct primitive));
        PeriaptPrimitiveCount[partIndex] = PrimitiveCount;
    }

    // We don't know what order the draw calls for the periapt are issued in,
    // so we do nothing until the last one, then we do all the drawing we need.
    int isLastPart = (g_State.partIndex==PERIAPT_PART_COUNT-1);
    ++g_State.partIndex;
    if (!isLastPart)
        return S_OK;    

    // TODO: allow the alpha threshold (or at least changes to it) to
    //       be driven by script. For now, this will do to let me do
    //       the art bits.
    static uint32_t frameCounter;
    float time = (float)(frameCounter&0x3FF)/255.0f;
    ++frameCounter;

    // OKAY: lets build this up slowly. Start with just drawing the periapt
    //       the way i want it.

    // Save the current alpha blend, alpha test, and z state.
    DWORD alphaBlend, alphaTest, alphaRef, alphaFunc;
    DWORD zEnable, zFunc;
    device->GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
    device->GetRenderState(D3DRS_ALPHATESTENABLE, &alphaTest);
    device->GetRenderState(D3DRS_ALPHAREF, &alphaRef);
    device->GetRenderState(D3DRS_ALPHAFUNC, &alphaFunc);
    // device->GetRenderState(D3DRS_ZWRITEENABLE, &zEnable);
    // device->GetRenderState(D3DRS_ZFUNC, &zFunc);

    // Draw the main parts of the periapt.
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    // Part 1 is alpha-tested.
    device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    device->SetRenderState(D3DRS_ALPHAREF, 127);
    device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    device->SetTexture(0, g_State.part2Texture);
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
        PeriaptPrimitiveCount[PERIAPT_PART_2],
        PeriaptPrimitive[PERIAPT_PART_2],
        sizeof(struct primitive));
    // Part 2 is opaque.
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetTexture(0, g_State.part1Texture);
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
        PeriaptPrimitiveCount[PERIAPT_PART_1],
        PeriaptPrimitive[PERIAPT_PART_1],
        sizeof(struct primitive));
    // The crystal is opaque, but must render into the stencil buffer.
    device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
    device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
    device->SetRenderState(D3DRS_STENCILREF, 0x1);
    device->SetRenderState(D3DRS_STENCILMASK, 0xffffffff);
    device->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff);
    device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
    device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
    device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
    device->SetTexture(0, g_State.crystalTexture);
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
        PeriaptPrimitiveCount[PERIAPT_CRYSTAL],
        PeriaptPrimitive[PERIAPT_CRYSTAL],
        sizeof(struct primitive));
    device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

    // Restore alpha blend, alpha test, and z state.
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest);
    device->SetRenderState(D3DRS_ALPHAREF, alphaRef);
    device->SetRenderState(D3DRS_ALPHAFUNC, alphaFunc);
    // device->SetRenderState(D3DRS_ZWRITEENABLE, zEnable);
    // device->SetRenderState(D3DRS_ZFUNC, zFunc);

    return S_OK;

#if 0
    if (g_State.isTransitioning) {
    } else {
    }

        g_State.crystalTexture;
        g_State.overlayTexture;

        // We want to clear the stencil for all the non-crystal parts of the
        // periapt. Particularly for the chain and filigree, this allows them
        // to be visible in front of the dual view.
        //
        // For the crystal, we want to draw it twice. The first time opaquely
        // and clearing the stencil. The second time with alpha testing and
        // clearing the stencil, so the dual view is limited to the areas where
        // the alpha test passes. We can then modulate the dual view appearing
        // or disappearing by changing the alpha test threshold.
        //
        // For the overlay, we want to ignore its actual vertex data, and
        // render our own, full screen. It should not... figure this out later!

        // Save the current alpha blend, alpha test, and z state.
        DWORD alphaBlend = 0;
        DWORD alphaTest = 0;
        DWORD alphaRef = 0;
        DWORD alphaFunc = 0;
        DWORD zEnable = 0;
        DWORD zFunc = 0;
        device->GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
        device->GetRenderState(D3DRS_ALPHATESTENABLE, &alphaTest);
        device->GetRenderState(D3DRS_ALPHAREF, &alphaRef);
        device->GetRenderState(D3DRS_ALPHAFUNC, &alphaFunc);
        device->GetRenderState(D3DRS_ZWRITEENABLE, &zEnable);
        device->GetRenderState(D3DRS_ZFUNC, &zFunc);

        // Set up the stencil rendering.
        device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
        device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
        device->SetRenderState(D3DRS_STENCILREF, 0x1);
        device->SetRenderState(D3DRS_STENCILMASK, 0xffffffff);
        device->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff);
        device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
        device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);

        // TODO: maybe add a flag as to whether we bother drawing this overlay
        //       at all?
        if (g_State.partIndex==0) {
            // Draw a full screen quad with the overlay texture, for world
            // transitions.

            // Get the render target size.
            IDirect3DSurface9* surface = NULL;
            D3DSURFACE_DESC surfaceDesc = {};
            device->GetRenderTarget(0, &surface);
            surface->GetDesc(&surfaceDesc);

            // FVF is D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1
            // so we just match that.
            struct vertex {
                float x,y,z,w;
                D3DCOLOR diffuse;
                D3DCOLOR specular;
                float u,v;
            } __attribute__((aligned(4)));
            struct vertex overlayVertices[4];
            for (int i=0; i<4; ++i) {
                if (i&1) {
                    overlayVertices[i].x = (float)surfaceDesc.Width+0.5f;
                    overlayVertices[i].u = 1.0f;
                } else {
                    overlayVertices[i].x = 0.5f;
                    overlayVertices[i].u = 0.0f;
                }
                if (i&2) {
                    overlayVertices[i].y = (float)surfaceDesc.Height+0.5f;
                    overlayVertices[i].v = 1.0f;
                } else {
                    overlayVertices[i].y = 0.5f;
                    overlayVertices[i].v = 0.0f;
                }
                overlayVertices[i].z = 0.0f; // TODO: ??? is this necessarily z-reversed, or what?
                overlayVertices[i].w = 1.0f;
                overlayVertices[i].diffuse = D3DCOLOR_COLORVALUE(1.0,1.0,1.0,1.0);
                overlayVertices[i].specular = D3DCOLOR_COLORVALUE(0.0,0.0,0.0,1.0);
            }

            int threshold;
            if (time>=1.0f && time<2.0f) {
                threshold = (int)((time-1.0f)*255.0f);
            } else if (time>=2.0f && time<3.0f) {
                threshold = (int)((1.0f-(time-2.0f))*255.0f);
            } else {
                threshold = 0;
            }

            // Draw the overlay, alpha tested, not z tested, setting the stencil
            device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            device->SetRenderState(D3DRS_ALPHAREF, threshold);
            device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESSEQUAL);
            device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
            device->SetTexture(0, g_State.overlayTexture);

            result = device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2,
                overlayVertices, sizeof(struct vertex));

            // Restore alpha blend, alpha test, z state, and texture.
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest);
            device->SetRenderState(D3DRS_ALPHAREF, alphaRef);
            device->SetRenderState(D3DRS_ALPHAFUNC, alphaFunc);
            device->SetRenderState(D3DRS_ZWRITEENABLE, zEnable);
            device->SetRenderState(D3DRS_ZFUNC, zFunc);
            device->SetTexture(0, texture);
        }

        // Now draw this part of the periapt
        if (isTheCrystal) {
            // Draw the crystal opaquely, clearing the stencil
            device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_ZERO);
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            result = device->DrawPrimitiveUP(PrimitiveType, PrimitiveCount,
                pVertexStreamZeroData, VertexStreamZeroStride);

            int threshold;
            if (time>=0.0f && time<1.0f) {
                threshold = (int)(time*255.0f);
            } else if (time>=3.0f && time<4.0f) {
                threshold = (int)((1.0f-(time-3.0f))*255.0f);
            } else {
                threshold = 255;
            }

            // Draw it again, alpha tested, setting the stencil
            device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            device->SetRenderState(D3DRS_ALPHAREF, threshold);
            device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESSEQUAL);
            result = device->DrawPrimitiveUP(PrimitiveType, PrimitiveCount,
                pVertexStreamZeroData, VertexStreamZeroStride);
        } else if (isTheOverlay) {
            // Don't bother drawing it, it only exists to get the game to load
            // the texture for us.
            result = S_OK;
        } else {
            // Clear the stencil with this part.
            device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_ZERO);
            if (alphaBlend) {
                // Force alpha clip instead of alpha blend.
                device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
                device->SetRenderState(D3DRS_ALPHAREF, 127);
            }
            result = device->DrawPrimitiveUP(PrimitiveType, PrimitiveCount,
                pVertexStreamZeroData, VertexStreamZeroStride);
        }

        // Restore alpha blend, alpha test, and z state.
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest);
        device->SetRenderState(D3DRS_ALPHAREF, alphaRef);
        device->SetRenderState(D3DRS_ALPHAFUNC, alphaFunc);

        // Disable stencil rendering again.
        device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

        ++g_State.partIndex;

        return result;
    } else {
        return device->DrawPrimitiveUP(PrimitiveType, PrimitiveCount,
            pVertexStreamZeroData, VertexStreamZeroStride);
    }
#endif
}

// TODO: I don't think we want to hook and unhook many parts individually,
// so change this to use a single flag for if all hooks are installed or not.
bool hooked_cam_render_scene;
bool hooked_cD8Renderer_Clear;
bool hooked_dark_render_overlays;
bool hooked_rendobj_render_object;
bool hooked_explore_portals;
bool hooked_initialize_first_region_clip;
bool hooked_mm_hardware_render;
bool hooked_mDrawTriangleLists;

void install_all_hooks() {
    hooks_spew("Hooking cam_render_scene...\n");
    install_hook(&hooked_cam_render_scene,
        (uint32_t)GameInfoTable.cam_render_scene,
        (uint32_t)&TRAMPOLINE_cam_render_scene,
        (uint32_t)&BYPASS_cam_render_scene,
        GameInfoTable.cam_render_scene_preamble);
    hooks_spew("Hooking cD8Renderer::Clear...\n");
    install_hook(&hooked_cD8Renderer_Clear,
        (uint32_t)GameInfoTable.cD8Renderer_Clear,
        (uint32_t)&TRAMPOLINE_cD8Renderer_Clear,
        (uint32_t)&BYPASS_cD8Renderer_Clear,
        GameInfoTable.cD8Renderer_Clear_preamble);
    hooks_spew("Hooking dark_render_overlays...\n");
    install_hook(&hooked_dark_render_overlays,
        (uint32_t)GameInfoTable.dark_render_overlays,
        (uint32_t)&TRAMPOLINE_dark_render_overlays,
        (uint32_t)&BYPASS_dark_render_overlays,
        GameInfoTable.dark_render_overlays_preamble);
    hooks_spew("Hooking rendobj_render_object...\n");
    install_hook(&hooked_rendobj_render_object,
        (uint32_t)GameInfoTable.rendobj_render_object,
        (uint32_t)&TRAMPOLINE_rendobj_render_object,
        (uint32_t)&BYPASS_rendobj_render_object,
        GameInfoTable.rendobj_render_object_preamble);
    hooks_spew("Hooking explore_portals...\n");
    install_hook(&hooked_explore_portals,
        (uint32_t)GameInfoTable.explore_portals,
        (uint32_t)&TRAMPOLINE_explore_portals,
        (uint32_t)&BYPASS_explore_portals,
        GameInfoTable.explore_portals_preamble);
    hooks_spew("Hooking initialize_first_region_clip...\n");
    install_hook(&hooked_initialize_first_region_clip,
        (uint32_t)GameInfoTable.initialize_first_region_clip,
        (uint32_t)&TRAMPOLINE_initialize_first_region_clip,
        (uint32_t)&BYPASS_initialize_first_region_clip,
        GameInfoTable.initialize_first_region_clip_preamble);
    hooks_spew("Hooking mm_hardware_render...\n");
    install_hook(&hooked_mm_hardware_render,
        (uint32_t)GameInfoTable.mm_hardware_render,
        (uint32_t)&TRAMPOLINE_mm_hardware_render,
        (uint32_t)&BYPASS_mm_hardware_render,
        GameInfoTable.mm_hardware_render_preamble);
    hooks_spew("Hooking mDrawTriangleLists...\n");
    install_hook(&hooked_mDrawTriangleLists,
        (uint32_t)GameInfoTable.mDrawTriangleLists,
        (uint32_t)&TRAMPOLINE_mDrawTriangleLists,
        (uint32_t)&BYPASS_mDrawTriangleLists,
        GameInfoTable.mDrawTriangleLists_preamble);
}

void remove_all_hooks() {
    hooks_spew("Unhooking cam_render_scene...\n");
    remove_hook(&hooked_cam_render_scene,
        (uint32_t)GameInfoTable.cam_render_scene,
        (uint32_t)&TRAMPOLINE_cam_render_scene,
        (uint32_t)&BYPASS_cam_render_scene,
        GameInfoTable.cam_render_scene_preamble);
    hooks_spew("Unhooking cD8Renderer::Clear...\n");
    remove_hook(&hooked_cD8Renderer_Clear,
        (uint32_t)GameInfoTable.cD8Renderer_Clear,
        (uint32_t)&TRAMPOLINE_cD8Renderer_Clear,
        (uint32_t)&BYPASS_cD8Renderer_Clear,
        GameInfoTable.cD8Renderer_Clear_preamble);
    hooks_spew("Unhooking dark_render_overlays...\n");
    remove_hook(&hooked_dark_render_overlays,
        (uint32_t)GameInfoTable.dark_render_overlays,
        (uint32_t)&TRAMPOLINE_dark_render_overlays,
        (uint32_t)&BYPASS_dark_render_overlays,
        GameInfoTable.dark_render_overlays_preamble);
    hooks_spew("Unhooking rendobj_render_object...\n");
    remove_hook(&hooked_rendobj_render_object,
        (uint32_t)GameInfoTable.rendobj_render_object,
        (uint32_t)&TRAMPOLINE_rendobj_render_object,
        (uint32_t)&BYPASS_rendobj_render_object,
        GameInfoTable.rendobj_render_object_preamble);
    hooks_spew("Unhooking explore_portals...\n");
    remove_hook(&hooked_explore_portals,
        (uint32_t)GameInfoTable.explore_portals,
        (uint32_t)&TRAMPOLINE_explore_portals,
        (uint32_t)&BYPASS_explore_portals,
        GameInfoTable.explore_portals_preamble);
    hooks_spew("Unhooking initialize_first_region_clip...\n");
    remove_hook(&hooked_initialize_first_region_clip,
        (uint32_t)GameInfoTable.initialize_first_region_clip,
        (uint32_t)&TRAMPOLINE_initialize_first_region_clip,
        (uint32_t)&BYPASS_initialize_first_region_clip,
        GameInfoTable.initialize_first_region_clip_preamble);
    hooks_spew("Unhooking mm_hardware_render...\n");
    remove_hook(&hooked_mm_hardware_render,
        (uint32_t)GameInfoTable.mm_hardware_render,
        (uint32_t)&TRAMPOLINE_mm_hardware_render,
        (uint32_t)&BYPASS_mm_hardware_render,
        GameInfoTable.mm_hardware_render_preamble);
    hooks_spew("Unhooking mDrawTriangleLists...\n");
    remove_hook(&hooked_mDrawTriangleLists,
        (uint32_t)GameInfoTable.mDrawTriangleLists,
        (uint32_t)&TRAMPOLINE_mDrawTriangleLists,
        (uint32_t)&BYPASS_mDrawTriangleLists,
        GameInfoTable.mDrawTriangleLists_preamble);
}

/*** Script class declarations (this will usually be in a header file) ***/

class cScr_PeriaptControl : public cScript
{
public:
    virtual ~cScr_PeriaptControl() { }
    cScr_PeriaptControl(const char* pszName, int iHostObjId)
        : cScript(pszName,iHostObjId)
    { }

    STDMETHOD_(long,ReceiveMessage)(sScrMsg*,sMultiParm*,eScrTraceAction);

public:
    static IScript* __cdecl ScriptFactory(const char* pszName, int iHostObjId);
};

/*** Script implementations ***/

long cScr_PeriaptControl::ReceiveMessage(sScrMsg* pMsg, sMultiParm* pReply, eScrTraceAction eTrace)
{
    long iRet = cScript::ReceiveMessage(pMsg, pReply, eTrace);

    if (stricmp(pMsg->message, "Sim") == 0) {
        bool fStarting = static_cast<sSimMsg*>(pMsg)->fStarting;
        printf(PREFIX "Sim: fStarting=%s\n", (fStarting ? "true" : "false"));
        if (fStarting) {
            // TODO: later we might want the switch to control just the specific
            // periapt rendering, not necessarily enable/disable hooks generally;
            // and at that point we'll want to enable hooks OnSim starting.
            //enable_hooks();
        } else {
            // Make sure hooks are disabled when the sim stops (e.g. returning to editor).
            disable_hooks();
        }
    }
    else if (stricmp(pMsg->message, "DarkGameModeChange") == 0) {
        bool fEntering = static_cast<sDarkGameModeScrMsg*>(pMsg)->fEntering;
        printf(PREFIX "DarkGameModeChange: fEntering=%s\n", (fEntering ? "true" : "false"));
    }
    else if (stricmp(pMsg->message, "BeginScript") == 0) {
        printf(PREFIX "BeginScript\n");
        // TODO: load settings from saved data (if any)
        g_Periapt.dualRender = false;
        g_Periapt.dualOffset.x = 0.0f;
        g_Periapt.dualOffset.y = 0.0f;
        g_Periapt.dualOffset.z = 1.0f;
        g_Periapt.dualCull = false;
        g_Periapt.dualCullLeft = 0.0f;
        g_Periapt.dualCullTop = 0.0f;
        g_Periapt.dualCullRight = 1.0f;
        g_Periapt.dualCullBottom = 1.0f;
        g_Periapt.depthCull = false;
        g_Periapt.depthCullDistance = 256.0f;
    }
    else if (stricmp(pMsg->message, "EndScript") == 0) {
        printf(PREFIX "EndScript\n");
    }
    else if (stricmp(pMsg->message, "TurnOn") == 0) {
        printf(PREFIX "TurnOn\n");
        enable_hooks();
    }
    else if (stricmp(pMsg->message, "TurnOff") == 0) {
        printf(PREFIX "TurnOn\n");
        disable_hooks();
    }
    else if (stricmp(pMsg->message, "SetDualRender") == 0) {
        bool v = static_cast<bool>(pMsg->data);
        printf(PREFIX "SetDualRender(%s)\n", (v?"true":"false"));
        g_Periapt.dualRender = v;
    }
    else if (stricmp(pMsg->message, "SetDualOffset") == 0) {
        const mxs_vector *v = static_cast<const mxs_vector *>(pMsg->data);
        if (v) {
            printf(PREFIX "SetDualOffset(<%0.3f,%0.3f,%0.3f>)\n", v->x, v->y, v->z);
            g_Periapt.dualOffset.x = v->x;
            g_Periapt.dualOffset.y = v->y;
            g_Periapt.dualOffset.z = v->z;
        } else {
            printf(PREFIX "SetDualOffset(<not a vector>)\n");
        }
    }
    else if (stricmp(pMsg->message, "SetDualCull") == 0) {
        bool v = static_cast<bool>(pMsg->data);
        printf(PREFIX "SetDualCull(%s)\n", (v?"true":"false"));
        g_Periapt.dualCull = v;
    }
    else if (stricmp(pMsg->message, "SetDualCullRect") == 0) {
        const mxs_vector *v0 = static_cast<const mxs_vector *>(pMsg->data);
        const mxs_vector *v1 = static_cast<const mxs_vector *>(pMsg->data2);
        if (v0 && v1) {
            printf(PREFIX "SetDualCullRect(<%0.3f,%0.3f,%0.3f>, <%0.3f,%0.3f,%0.3f>)\n",
                v0->x, v0->y, v0->z, v1->x, v1->y, v1->z);
            g_Periapt.dualCullLeft = v0->x;
            g_Periapt.dualCullTop = v0->y;
            g_Periapt.dualCullRight = v1->x;
            g_Periapt.dualCullBottom = v1->y;
            if (g_Periapt.dualCullLeft<0) g_Periapt.dualCullLeft = 0;
            if (g_Periapt.dualCullLeft>1) g_Periapt.dualCullLeft = 1;
            if (g_Periapt.dualCullTop<0) g_Periapt.dualCullTop = 0;
            if (g_Periapt.dualCullTop>1) g_Periapt.dualCullTop = 1;
            if (g_Periapt.dualCullRight<0) g_Periapt.dualCullRight = 0;
            if (g_Periapt.dualCullRight>1) g_Periapt.dualCullRight = 1;
            if (g_Periapt.dualCullBottom<0) g_Periapt.dualCullBottom = 0;
            if (g_Periapt.dualCullBottom>1) g_Periapt.dualCullBottom = 1;
        } else {
            printf(PREFIX "SetDualCullRect(<not a vector>, <not a vector>)\n");
        }
    }
    else if (stricmp(pMsg->message, "SetDepthCull") == 0) {
        bool v = static_cast<bool>(pMsg->data);
        printf(PREFIX "SetDepthCull(%s)\n", (v?"true":"false"));
        g_Periapt.depthCull = v;
    }
    else if (stricmp(pMsg->message, "SetDepthCullDistance") == 0) {
        float d = static_cast<float>(pMsg->data);
        printf(PREFIX "SetDepthCullDistance(%0.3f)\n", d);
        if (d>0) {
            g_Periapt.depthCullDistance = d;
        }
    }

    return iRet;
}

/*** Script Factories ***/

IScript* cScr_PeriaptControl::ScriptFactory(const char* pszName, int iHostObjId)
{
    if (stricmp(pszName,"PeriaptControl") != 0)
        return NULL;

    // Use a static string, so I don't have to make a copy.
    cScr_PeriaptControl* pscrRet = new(nothrow) cScr_PeriaptControl("PeriaptControl", iHostObjId);
    return static_cast<IScript*>(pscrRet);
}

const sScrClassDesc cScriptModule::sm_ScriptsArray[] = {
    { "periapt", "PeriaptControl", "CustomScript", cScr_PeriaptControl::ScriptFactory },
};
const unsigned int cScriptModule::sm_ScriptsArraySize = sizeof(sm_ScriptsArray)/sizeof(sm_ScriptsArray[0]);

/*** Entry point ***/

#include <stdio.h>
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    (void)hModule; // Unused
    (void)lpReserved; // Unused
    static bool didAllocConsole = false;
    switch (reason) {
    case DLL_PROCESS_ATTACH: {
        // Try to identify this exe.
        ExeIdentity identity = IdentifyExe();
        bool isEditor, isIdentified;
        switch (identity) {
        case ExeThief_v126:
        case ExeThief_v127:
            isEditor = false;
            isIdentified = true;
            break;
        case ExeDromEd_v126:
        case ExeDromEd_v127:
            isEditor = true;
            isIdentified = true;
            break;
        default:
            isEditor = false;
            isIdentified = false;
            break;
        }
        // Decide accordingly how to handle console output.
        bool shouldAllocConsole = (!isEditor || !isIdentified);
        bool shouldRedirectStdout = (isEditor || shouldAllocConsole);
        if (shouldAllocConsole) {
            didAllocConsole = AllocConsole();
        }
        if (shouldRedirectStdout) {
            // FIXME: I should really use mprintf instead of 
            // printf-and-redirecting-stdout! Cause I really need the
            // output to go to monolog.txt too...
            // problem is, we don't get the monolog until ScriptModuleInit
            // is called, which is later than ideal.
            // Also, in game mode, I'd like the monolog _and_ a console
            // for debugging purposes.
            freopen("CONOUT$", "w", stdout);

            // FIXME: for my convenience, let's put the console in
            //        a handy location.
            HWND hwnd = GetConsoleWindow();
            RECT rect;
            GetWindowRect(hwnd, &rect);
            int left = 1930;
            int top = 10;
            int width = (int)(rect.right - rect.left);
            int height = (int)(rect.bottom - rect.top);
            MoveWindow(hwnd, left, top, width, height, TRUE);
        }

        // Now that we (hopefully) have somewhere for output to go,
        // we can print some useful messages.
        printf(PREFIX "DLL_PROCESS_ATTACH\n");
        printf(PREFIX "current thread: %u\n", (unsigned int)GetCurrentThreadId());
        printf(PREFIX "Base address: 0x%08x\n", (unsigned int)GetModuleHandle(NULL));
        printf(PREFIX "DLL base address: 0x%08x\n", (unsigned int)hModule);

        // Display the results of identifying the exe.
        if (isIdentified) {
            const ExeSignature *info = &ExeSignatureTable[identity];
            printf(PREFIX "Identified exe as %s %s (%s)\n",
                info->name, info->version, (isEditor ? "EDITOR" : "GAME"));
        } else {
            printf(PREFIX "Cannot identify exe; must not continue!\n");
            return false;
        }

        LoadGameInfoTable(identity);
        install_all_hooks();
    } break;
    case DLL_PROCESS_DETACH: {
        remove_all_hooks();

        printf(PREFIX "DLL_PROCESS_DETACH\n");
        printf(PREFIX "current thread: %u\n", (unsigned int)GetCurrentThreadId());
        if (didAllocConsole) {
            FreeConsole();
        }
    } break;
    // case DLL_THREAD_ATTACH: {
    //     printf(PREFIX "DLL_THREAD_ATTACH\n");
    //     printf(PREFIX "current thread: %u\n", (unsigned int)GetCurrentThreadId());
    // } break;
    // case DLL_THREAD_DETACH: {
    //     printf(PREFIX "DLL_THREAD_DETACH\n");
    //     printf(PREFIX "current thread: %u\n", (unsigned int)GetCurrentThreadId());
    // } break;
    }
    return true;
}
