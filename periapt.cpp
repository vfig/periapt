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
    BOOL ok = ReadProcessMemory(hProcess,
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
    bool depthCull;
    float depthCullDistance;
} g_Periapt = { };

// Functions to be called:
t2position* (__cdecl *t2_ObjPosGet)(t2id obj);
bool (__cdecl *t2_SphrSphereInWorld)(t2location *center_loc, float radius);

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
    void (__stdcall *Set)(void* thisptr, t2id obj, const char* name);
    DWORD reserved19;

    BOOL (__stdcall *Get)(void* thisptr, t2id obj, const char** pName);
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
        0x0054f190UL,       // matcache
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
    ADDR_ComputeCellForLocation = (unsigned int)GameInfoTable.ComputeCellForLocation;
    t2_SphrSphereInWorld = (bool (__cdecl *)(t2location*,float))GameInfoTable.SphrSphereInWorld;
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

void patch_jmp(unsigned int jmp_address, unsigned int dest_address) {
    const unsigned char jmp = 0xE9;
    int offset = (dest_address - (jmp_address+5));
    hooks_spew("  offset: %08x (%d)\n", offset, offset);
    hooks_spew("  memcpy %08x, %08x, %u\n", jmp_address, (unsigned int)&jmp, 1);
    memcpy((void *)jmp_address, &jmp, 1);
    hooks_spew("  memcpy %08x, %08x, %u\n", jmp_address+1, (unsigned int)&offset, 4);
    memcpy((void *)(jmp_address+1), &offset, 4);
}

void install_hook(bool *hooked, unsigned int target, unsigned int trampoline, unsigned int bypass, unsigned int size) {
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

void remove_hook(bool *hooked, unsigned int target, unsigned int trampoline, unsigned int bypass, unsigned int size) {
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

/***************************************************************************/
/*** D3D state saving, comparing, and restoration. ***/

typedef struct D3DState {
    // TODO: probably need to store more state, otherwise other things could
    //       bleed through or be set wrong.
    // DWORD FVF;
    // struct {
    //     DWORD addressU;
    //     DWORD addressV;
    //     DWORD magFilter;
    //     DWORD minFilter;
    // } samplerState;
    struct {
        DWORD alphaBlendEnable;
        DWORD srcBlend;
        DWORD destBlend;
        DWORD alphaTestEnable;
        DWORD alphaRef;
        DWORD alphaFunc;
        DWORD zWriteEnable;
        DWORD zFunc;
        DWORD stencilEnable;
        DWORD stencilMask;
        DWORD stencilWriteMask;
        DWORD stencilFunc;
        DWORD stencilRef;
        DWORD stencilFail;
        DWORD stencilZFail;
        DWORD stencilPass;
    } rs;
    struct {
        DWORD colorOp;
        DWORD colorArg1;
        DWORD colorArg2;
        DWORD alphaOp;
        DWORD alphaArg1;
        DWORD alphaArg2;
    } tss[2];
    struct {
        // TODO: not actually using these yet. ditch them?
        D3DMATRIX world;
        D3DMATRIX view;
        D3DMATRIX projection;
    } ts;
    IDirect3DBaseTexture9 *texture[2];
    bool valid;
} D3DState;

#define D3DSTATE_STACK_COUNT 2
static int g_D3DStateStackIndex = -1;
static D3DState g_priorD3DStateStack[D3DSTATE_STACK_COUNT];
static D3DState g_activeD3DStateStack[D3DSTATE_STACK_COUNT];

static void BeginD3DStateChanges(IDirect3DDevice9* device, D3DState *state) {
    assert(g_D3DStateStackIndex<D3DSTATE_STACK_COUNT);
    ++g_D3DStateStackIndex;
    assert(g_D3DStateStackIndex>=0);
    D3DState *priorState = &g_priorD3DStateStack[g_D3DStateStackIndex];
    D3DState *activeState = &g_activeD3DStateStack[g_D3DStateStackIndex];
    assert(!priorState->valid);
    assert(!activeState->valid);
    #define GET(FLAG,VAR) \
        device->GetRenderState(FLAG, &activeState->rs. VAR);
    GET(D3DRS_ALPHABLENDENABLE, alphaBlendEnable);
    GET(D3DRS_SRCBLEND, srcBlend);
    GET(D3DRS_DESTBLEND, destBlend);
    GET(D3DRS_ALPHATESTENABLE, alphaTestEnable);
    GET(D3DRS_ALPHAREF, alphaRef);
    GET(D3DRS_ALPHAFUNC, alphaFunc);
    GET(D3DRS_ZWRITEENABLE, zWriteEnable);
    GET(D3DRS_ZFUNC, zFunc);
    GET(D3DRS_STENCILENABLE, stencilEnable);
    GET(D3DRS_STENCILMASK, stencilMask);
    GET(D3DRS_STENCILWRITEMASK, stencilWriteMask);
    GET(D3DRS_STENCILFUNC, stencilFunc);
    GET(D3DRS_STENCILREF, stencilRef);
    GET(D3DRS_STENCILFAIL, stencilFail);
    GET(D3DRS_STENCILZFAIL, stencilZFail);
    GET(D3DRS_STENCILPASS, stencilPass);
    #undef GET
    #define GET(FLAG,VAR) \
        device->GetTextureStageState(i, FLAG, &activeState->tss[i]. VAR);
    for (int i=0; i<2; ++i) {
        GET(D3DTSS_COLOROP, colorOp);
        GET(D3DTSS_COLORARG1, colorArg1);
        GET(D3DTSS_COLORARG2, colorArg2);
        GET(D3DTSS_ALPHAOP, alphaOp);
        GET(D3DTSS_ALPHAARG1, alphaArg1);
        GET(D3DTSS_ALPHAARG2, alphaArg2);
        device->GetTexture(i, &activeState->texture[i]);
    }
    #undef GET
    #define GET(FLAG,VAR) \
        device->GetTransform(FLAG, &activeState->ts. VAR);
    GET(D3DTS_WORLD, world);
    GET(D3DTS_VIEW, view);
    GET(D3DTS_PROJECTION, projection);
    #undef GET

    activeState->valid = true;
    *priorState = *activeState;
    *state = *activeState;
}
static void ApplyD3DState(IDirect3DDevice9* device, D3DState *state) {
    assert(g_D3DStateStackIndex>=0);
    assert(g_D3DStateStackIndex<D3DSTATE_STACK_COUNT);
    D3DState *activeState = &g_activeD3DStateStack[g_D3DStateStackIndex];
    assert(activeState->valid);
    assert(state->valid);
    #define UPDATE(FLAG,VAR) \
        if (state->rs. VAR != activeState->rs. VAR) {   \
            activeState->rs. VAR = state->rs. VAR;      \
            device->SetRenderState(FLAG, state->rs. VAR);  \
        }
    UPDATE(D3DRS_ALPHABLENDENABLE, alphaBlendEnable);
    UPDATE(D3DRS_SRCBLEND, srcBlend);
    UPDATE(D3DRS_DESTBLEND, destBlend);
    UPDATE(D3DRS_ALPHATESTENABLE, alphaTestEnable);
    UPDATE(D3DRS_ALPHAREF, alphaRef);
    UPDATE(D3DRS_ALPHAFUNC, alphaFunc);
    UPDATE(D3DRS_ZWRITEENABLE, zWriteEnable);
    UPDATE(D3DRS_ZFUNC, zFunc);
    UPDATE(D3DRS_STENCILENABLE, stencilEnable);
    UPDATE(D3DRS_STENCILMASK, stencilMask);
    UPDATE(D3DRS_STENCILWRITEMASK, stencilWriteMask);
    UPDATE(D3DRS_STENCILFUNC, stencilFunc);
    UPDATE(D3DRS_STENCILREF, stencilRef);
    UPDATE(D3DRS_STENCILFAIL, stencilFail);
    UPDATE(D3DRS_STENCILZFAIL, stencilZFail);
    UPDATE(D3DRS_STENCILPASS, stencilPass);
    #undef UPDATE
    #define UPDATE(FLAG,VAR) \
        if (state->tss[i]. VAR != activeState->tss[i]. VAR) {           \
            activeState->tss[i]. VAR = state->tss[i]. VAR;              \
            device->SetTextureStageState(i, FLAG, state->tss[i]. VAR);  \
        }
    for (int i=0; i<2; ++i) {
        UPDATE(D3DTSS_COLOROP, colorOp);
        UPDATE(D3DTSS_COLORARG1, colorArg1);
        UPDATE(D3DTSS_COLORARG2, colorArg2);
        UPDATE(D3DTSS_ALPHAOP, alphaOp);
        UPDATE(D3DTSS_ALPHAARG1, alphaArg1);
        UPDATE(D3DTSS_ALPHAARG2, alphaArg2);
        if (state->texture[i] != activeState->texture[i]) {
            activeState->texture[i] = state->texture[i];
            device->SetTexture(i, state->texture[i]);
        }
    }
    #undef UPDATE
    #define UPDATE(FLAG,VAR) \
        if (memcmp(&state->ts. VAR, &activeState->ts. VAR,  \
                   sizeof(D3DMATRIX))!=0) {                 \
            activeState->ts. VAR = state->ts. VAR;          \
            device->SetTransform(FLAG, &state->ts. VAR);    \
        }
    UPDATE(D3DTS_WORLD, world);
    UPDATE(D3DTS_VIEW, view);
    UPDATE(D3DTS_PROJECTION, projection);
    #undef UPDATE
}
static void EndD3DStateChanges(IDirect3DDevice9* device) {
    assert(g_D3DStateStackIndex>=0);
    assert(g_D3DStateStackIndex<D3DSTATE_STACK_COUNT);
    D3DState *priorState = &g_priorD3DStateStack[g_D3DStateStackIndex];
    D3DState *activeState = &g_activeD3DStateStack[g_D3DStateStackIndex];
    assert(priorState->valid);
    assert(activeState->valid);
    ApplyD3DState(device, priorState);
    priorState->valid = false;
    activeState->valid = false;
    --g_D3DStateStackIndex;
}

/***************************************************************************/
/*** Hooked functions: periapt rendering, and distance/view clipping ***/

#define FADE_DURATION 0.3f              // seconds

#define TRANSITION_DURATION 1.5f        // seconds
#define TRANSITION_GAP 4                // 0-255
// The value of TRANSITION_FIRSTHALF_END depends on the gradient in the art
// and the gap size. Once those are fixed, tune it to fit.
#define TRANSITION_FIRSTHALF_END 0.244f // progress t
#define TRANSITION_ZOOM_FACTOR 0.2f

// Stencil values
#define STENCILREF_ZERO 0
#define STENCILREF_DUAL 1
#define STENCILREF_REAL 2      // Only used during the transition effect.

typedef enum PeriaptMode {
    PERIAPT_NORMAL,
    PERIAPT_FADE,
    PERIAPT_ACTIVE,
    PERIAPT_TRANSITION_BODY_REAL,
    PERIAPT_TRANSITION_BODY_DUAL,
    PERIAPT_TRANSITION_CRYSTAL,
    PERIAPT_TRANSITION_CRYSTAL_Z_ONLY,
    PERIAPT_FACETS,
    PERIAPT_SAVE_STATE_ONLY,
} PeriaptMode;

typedef enum RenderWorldMode {
    RENDER_WORLD_REAL,
    RENDER_WORLD_DUAL,
} RenderWorldMode;

static struct {
    unsigned int frame;
    float frameTime;
    // TODO: rework isActive, isFading, isTransitioning into a single mode variable
    bool isActive;
    bool isFading;
    bool isFadingOut;
    float fadeStartTime;
    float fadeProgress;
    bool isTransitioning;
    bool isTransitionFirstHalf;
    float transitionStartTime;
    float transitionProgress;
    bool isRenderingDual;
    bool isDrawingOverlays;
    bool isDrawingPeriapt;
    bool didDrawPeriapt;
    bool dontClearTarget;
    bool dontClearStencil;
    bool texturesReady;
    bool verticesReady;
    int partCount;
    IDirect3DBaseTexture9 *crystalTexture;
    IDirect3DBaseTexture9 *facetsTexture;
    IDirect3DBaseTexture9 *overlayTexture;
    IDirect3DBaseTexture9 *part1Texture;
    IDirect3DBaseTexture9 *part2Texture;
} g_State = {};

// FVF is hardcoded as D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1
// for the viewmodel, so we just use that layout here.
struct PrimVertex {
    float x,y,z,w;
    D3DCOLOR diffuse;
    D3DCOLOR specular;
    float u,v;
} __attribute__((aligned(4)));
// We save transformed vertices here every frame, so we can draw it our way.
#define PERIAPT_PART_COUNT  4
#define PERIAPT_CRYSTAL 0
#define PERIAPT_PART_1  1
#define PERIAPT_PART_2  2
#define PERIAPT_IGNORE  -1
#define PERIAPT_VERTEX_COUNT  768
static struct PrimVertex PeriaptVertex[PERIAPT_VERTEX_COUNT];
static int PeriaptVertexIndex;
static UINT PeriaptPrimitiveCount[3];
static int PeriaptPrimitiveStart[3];
static D3DState PeriaptLastRenderState; // TODO: ditch this?
// The overlay is a full-screen quad.
#define OVERLAY_VERTEX_COUNT  4
static struct PrimVertex OverlayVertex[3*OVERLAY_VERTEX_COUNT];
static UINT OverlayPrimitiveCount;

void DrawTransitionOverlay(IDirect3DDevice9* device) {
    // TODO: allow the alpha threshold (or at least changes to it) to
    //       be driven by script. For now, this will do to let me do
    //       the art bits.
    int gap = TRANSITION_GAP;
    float t = g_State.transitionProgress;
    int threshold = (int)(t*(255.0f+gap));
    int innerThreshold = threshold;
    if (innerThreshold<0) innerThreshold = 0;
    if (innerThreshold>255) innerThreshold = 255;
    int outerThreshold = threshold+gap;
    if (outerThreshold<0) outerThreshold = 0;
    if (outerThreshold>255) outerThreshold = 255;

    // Set up the overlay to be a full screen quad.
    OverlayPrimitiveCount = 2;
    D3DVIEWPORT9 viewport = {};
    device->GetViewport(&viewport);
    // TODO: the uv manipulation is unnecessary when we have a tiny gap!
    //       but if i want a larger gap, i shold make it spin anticlockwise.
    float du = -1.7f*t;
    float dv = -1.7f*t;
    for (int i=0; i<OVERLAY_VERTEX_COUNT; ++i) {
        if (i&1) {
            OverlayVertex[i].x = (float)viewport.Width+1.0f;
            OverlayVertex[i].u = 1.0f+du;
        } else {
            OverlayVertex[i].x = -1.0f;
            OverlayVertex[i].u = 0.0f+du;
        }
        if (i&2) {
            OverlayVertex[i].y = (float)viewport.Height+1.0f;
            OverlayVertex[i].v = 1.0f+dv;
        } else {
            OverlayVertex[i].y = -1.0f;
            OverlayVertex[i].v = 0.0f+dv;
        }
        OverlayVertex[i].z = 0.0f;
        OverlayVertex[i].w = 1.0f;
        OverlayVertex[i].diffuse = D3DCOLOR_COLORVALUE(1.0,1.0,1.0,1.0);
        OverlayVertex[i].specular = D3DCOLOR_COLORVALUE(0.0,0.0,0.0,1.0);
    }

    // TODO: we can reduce this to two calls, by clearing the stencil to 1,
    // and rendering the first pass with inverted alpha test and to set
    // stencil to zero. but only if we don't do any uv shenanigans.
    D3DState d3dState;
    BeginD3DStateChanges(device, &d3dState);

    // Don't z test or z write.
    d3dState.rs.zWriteEnable = FALSE;
    d3dState.rs.zFunc = D3DCMP_ALWAYS;
    // First pass: render opaquely.
    d3dState.rs.alphaBlendEnable = FALSE;
    d3dState.rs.alphaTestEnable = FALSE;
    d3dState.texture[0] = g_State.overlayTexture;
    ApplyD3DState(device, &d3dState);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, OverlayPrimitiveCount,
        OverlayVertex, sizeof(struct PrimVertex));

    // Okay, now revert to boring quad uvs again for the alpha test passes.
    for (int i=0; i<OVERLAY_VERTEX_COUNT; ++i) {
        if (i&1) {
            OverlayVertex[i].u = 1.0f;
        } else {
            OverlayVertex[i].u = 0.0f;
        }
        if (i&2) {
            OverlayVertex[i].v = 1.0f;
        } else {
            OverlayVertex[i].v = 0.0f;
        }
    }

    // Second pass: render the inner circle to stencil 2
    d3dState.rs.alphaTestEnable = TRUE;
    d3dState.rs.alphaRef = innerThreshold;
    d3dState.rs.alphaFunc = D3DCMP_LESSEQUAL;
    d3dState.rs.stencilEnable = TRUE;
    d3dState.rs.stencilMask = 0xFF;
    d3dState.rs.stencilWriteMask = 0xFF;
    d3dState.rs.stencilFunc = D3DCMP_ALWAYS;
    d3dState.rs.stencilRef = STENCILREF_REAL;
    d3dState.rs.stencilFail = D3DSTENCILOP_KEEP;
    d3dState.rs.stencilZFail = D3DSTENCILOP_KEEP;
    d3dState.rs.stencilPass = D3DSTENCILOP_REPLACE;
    d3dState.texture[0] = g_State.overlayTexture;
    ApplyD3DState(device, &d3dState);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, OverlayPrimitiveCount,
        OverlayVertex, sizeof(struct PrimVertex));
    // Third time's the charm: render the outer circle to stencil 1
    d3dState.rs.alphaRef = outerThreshold;
    d3dState.rs.alphaFunc = D3DCMP_GREATER;
    d3dState.rs.stencilRef = STENCILREF_DUAL;
    d3dState.texture[0] = g_State.overlayTexture;
    ApplyD3DState(device, &d3dState);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, OverlayPrimitiveCount,
        OverlayVertex, sizeof(struct PrimVertex));

    EndD3DStateChanges(device);
}

static void DrawPeriapt(IDirect3DDevice9 *device, PeriaptMode mode) {
    assert(g_State.texturesReady);
    assert(g_State.verticesReady);

    D3DState d3dState;
    BeginD3DStateChanges(device, &d3dState);

    // TODO: ditch all this! we can set the state correctly!!
    // yeaaaah, not the transforms huh
#if 0
    switch (mode) {
        case PERIAPT_TRANSITION_CRYSTAL:
        case PERIAPT_TRANSITION_CRYSTAL_Z_ONLY:
        case PERIAPT_TRANSITION_BODY_REAL:
        case PERIAPT_TRANSITION_BODY_DUAL:
        case PERIAPT_FACETS:
            // Use the last render state.
            if (PeriaptLastRenderState.valid) {
                d3dState = PeriaptLastRenderState;
            }
            break;
        case PERIAPT_NORMAL:
        case PERIAPT_FADE:
        case PERIAPT_ACTIVE:
        case PERIAPT_SAVE_STATE_ONLY:
            // Save the last render state.
            PeriaptLastRenderState = d3dState;
            break;
    }
#else
    // TODO: should be covered by the state loading above
    // Usual settings for a lit viewmodel mesh.
    d3dState.tss[0].colorOp = D3DTOP_MODULATE;
    d3dState.tss[0].colorArg1 = D3DTA_TEXTURE;
    d3dState.tss[0].colorArg2 = D3DTA_DIFFUSE;
    d3dState.tss[0].alphaOp = D3DTOP_MODULATE;
    d3dState.tss[0].alphaArg1 = D3DTA_TEXTURE;
    d3dState.tss[0].alphaArg2 = D3DTA_DIFFUSE;
    d3dState.tss[1].colorOp = D3DTOP_DISABLE;
    d3dState.tss[1].alphaOp = D3DTOP_DISABLE;
#endif

    bool drawBody = (mode==PERIAPT_NORMAL
        || mode==PERIAPT_FADE
        || mode==PERIAPT_ACTIVE
        || mode==PERIAPT_TRANSITION_BODY_REAL
        || mode==PERIAPT_TRANSITION_BODY_DUAL
        || mode==PERIAPT_TRANSITION_CRYSTAL
        || mode==PERIAPT_FACETS);
    bool drawCrystal = (mode==PERIAPT_NORMAL
        || mode==PERIAPT_FADE
        || mode==PERIAPT_ACTIVE
        || mode==PERIAPT_TRANSITION_CRYSTAL
        || mode==PERIAPT_TRANSITION_CRYSTAL_Z_ONLY
        || mode==PERIAPT_FACETS);

    if (drawBody) {
        // Set up stencil parameters.
        switch (mode) {
        case PERIAPT_TRANSITION_BODY_REAL:
            // Test: stencil == 2
            // Write: keep
            d3dState.rs.stencilEnable = TRUE;
            d3dState.rs.stencilMask = 0x03;
            d3dState.rs.stencilWriteMask = 0x03;
            d3dState.rs.stencilFunc = D3DCMP_EQUAL;
            d3dState.rs.stencilRef = STENCILREF_REAL;
            d3dState.rs.stencilFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilZFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilPass = D3DSTENCILOP_KEEP;
            break;
        case PERIAPT_TRANSITION_BODY_DUAL:
            // Test: stencil == 1
            // Write: keep
            d3dState.rs.stencilEnable = TRUE;
            d3dState.rs.stencilMask = 0x03;
            d3dState.rs.stencilWriteMask = 0x03;
            d3dState.rs.stencilFunc = D3DCMP_EQUAL;
            d3dState.rs.stencilRef = STENCILREF_DUAL;
            d3dState.rs.stencilFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilZFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilPass = D3DSTENCILOP_KEEP;
            break;
        case PERIAPT_TRANSITION_CRYSTAL:
            // We need to draw these parts before the crystal so that
            // they knock out parts of the crystal behind them with
            // its z test.
            //
            // Test: stencil != 0
            // Write: keep
            d3dState.rs.stencilEnable = TRUE;
            d3dState.rs.stencilMask = 0x03;
            d3dState.rs.stencilWriteMask = 0x03;
            d3dState.rs.stencilFunc = D3DCMP_NOTEQUAL;
            d3dState.rs.stencilRef = STENCILREF_ZERO;
            d3dState.rs.stencilFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilZFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilPass = D3DSTENCILOP_KEEP;
            break;
        case PERIAPT_FACETS:
            // We need to draw these parts before the crystal so that
            // they knock out parts of the crystal behind them with
            // its z test.
            // Write only to z-buffer
            d3dState.rs.stencilEnable = FALSE;
            d3dState.rs.alphaBlendEnable = TRUE;
            d3dState.rs.srcBlend = D3DBLEND_ZERO;
            d3dState.rs.destBlend = D3DBLEND_ONE;
            break;
        default:
            d3dState.rs.stencilEnable = FALSE;
            break;
        }

        // Part 1 is alpha-tested.
        d3dState.rs.alphaBlendEnable = FALSE;
        if (mode==PERIAPT_FACETS) d3dState.rs.alphaBlendEnable = TRUE;
        d3dState.rs.alphaTestEnable = TRUE;
        d3dState.rs.alphaRef = 127;
        d3dState.rs.alphaFunc = D3DCMP_GREATEREQUAL;
        d3dState.texture[0] = g_State.part2Texture;
        ApplyD3DState(device, &d3dState);
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
            PeriaptPrimitiveCount[PERIAPT_PART_2],
            (PeriaptVertex+PeriaptPrimitiveStart[PERIAPT_PART_2]),
            sizeof(struct PrimVertex));

        // Part 2 is opaque.
        d3dState.rs.alphaTestEnable = FALSE;
        if (mode==PERIAPT_FACETS) d3dState.rs.alphaBlendEnable = TRUE;
        d3dState.texture[0] = g_State.part1Texture;
        ApplyD3DState(device, &d3dState);
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
            PeriaptPrimitiveCount[PERIAPT_PART_1],
            (PeriaptVertex+PeriaptPrimitiveStart[PERIAPT_PART_1]),
            sizeof(struct PrimVertex));
    }

    if (drawCrystal) {
        // The crystal is opaque, but must render into the stencil buffer.
        switch (mode) {
        case PERIAPT_TRANSITION_CRYSTAL:
            // Test: stencil != 0
            // Write: Swap stencil values 1 and 2
            d3dState.rs.stencilEnable = TRUE;
            d3dState.rs.stencilMask = 0x03;
            d3dState.rs.stencilWriteMask = 0x03;
            d3dState.rs.stencilFunc = D3DCMP_NOTEQUAL;
            d3dState.rs.stencilRef = STENCILREF_ZERO;
            d3dState.rs.stencilFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilZFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilPass = D3DSTENCILOP_INVERT;
            d3dState.rs.alphaBlendEnable = FALSE;
            d3dState.rs.alphaTestEnable = FALSE;
            d3dState.texture[0] = g_State.crystalTexture;
            break;
        case PERIAPT_TRANSITION_CRYSTAL_Z_ONLY:
            // Stencil disabled
            // Write only to z-buffer
            d3dState.rs.stencilEnable = FALSE;
            d3dState.rs.alphaBlendEnable = TRUE;
            d3dState.rs.srcBlend = D3DBLEND_ZERO;
            d3dState.rs.destBlend = D3DBLEND_ONE;
            d3dState.rs.alphaTestEnable = FALSE;
            d3dState.texture[0] = g_State.crystalTexture;
            break;
        case PERIAPT_FADE:
        {
            // When fading the crystal, we must draw it twice. The first time
            // with alpha testing and setting the stencil; the second time with
            // inverse alpha testing and stencil disabled; this way the dual
            // view is limited to the areas where the first alpha test passes.
            // We can then modulate the dual view fading in or out by changing
            // the alpha test threshold.
            float t = g_State.fadeProgress;
            if (g_State.isFadingOut) t = 1.0f-t;
            int threshold = (int)(t*255.0f);
            // Test: always
            // Write: stencil 1
            d3dState.rs.stencilEnable = TRUE;
            d3dState.rs.stencilMask = 0x03;
            d3dState.rs.stencilWriteMask = 0x03;
            d3dState.rs.stencilFunc = D3DCMP_ALWAYS;
            d3dState.rs.stencilRef = STENCILREF_DUAL;
            d3dState.rs.stencilFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilZFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilPass = D3DSTENCILOP_REPLACE;
            d3dState.rs.alphaBlendEnable = FALSE;
            d3dState.rs.alphaTestEnable = TRUE;
            d3dState.rs.alphaRef = threshold;
            d3dState.rs.alphaFunc = D3DCMP_LESSEQUAL;
            d3dState.texture[0] = g_State.crystalTexture;
        } break;
        case PERIAPT_ACTIVE:
            // Test: always
            // Write: stencil 1
            d3dState.rs.stencilEnable = TRUE;
            d3dState.rs.stencilMask = 0x03;
            d3dState.rs.stencilWriteMask = 0x03;
            d3dState.rs.stencilFunc = D3DCMP_ALWAYS;
            d3dState.rs.stencilRef = STENCILREF_DUAL;
            d3dState.rs.stencilFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilZFail = D3DSTENCILOP_KEEP;
            d3dState.rs.stencilPass = D3DSTENCILOP_REPLACE;
            d3dState.rs.alphaBlendEnable = FALSE;
            d3dState.rs.alphaTestEnable = FALSE;
            d3dState.texture[0] = g_State.crystalTexture;
            break;
        case PERIAPT_FACETS:
            // Draw the crystal, but with facets texture.
            //
            // Stencil disabled
            d3dState.rs.stencilEnable = FALSE;
            d3dState.rs.alphaBlendEnable = TRUE;
            d3dState.rs.srcBlend = D3DBLEND_SRCCOLOR;
            d3dState.rs.destBlend = D3DBLEND_ONE;
            d3dState.rs.alphaTestEnable = FALSE;
            d3dState.texture[0] = g_State.facetsTexture;
            break;
        default:
            // Stencil disabled
            d3dState.rs.stencilEnable = FALSE;
            d3dState.rs.alphaBlendEnable = FALSE;
            d3dState.rs.alphaTestEnable = FALSE;
            d3dState.texture[0] = g_State.crystalTexture;
            break;
        }
        ApplyD3DState(device, &d3dState);

        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
            PeriaptPrimitiveCount[PERIAPT_CRYSTAL],
            (PeriaptVertex+PeriaptPrimitiveStart[PERIAPT_CRYSTAL]),
            sizeof(struct PrimVertex));

        // Draw the crystal again for fade mode, with inverted alpha test.
        if (mode==PERIAPT_FADE) {
            // Stencil disabled
            d3dState.rs.stencilEnable = FALSE;
            d3dState.rs.alphaFunc = D3DCMP_GREATER;
            ApplyD3DState(device, &d3dState);
            device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                PeriaptPrimitiveCount[PERIAPT_CRYSTAL],
                (PeriaptVertex+PeriaptPrimitiveStart[PERIAPT_CRYSTAL]),
                sizeof(struct PrimVertex));
        }
    }

    EndD3DStateChanges(device);
}

static void RenderWorld(IDirect3DDevice9* device, t2position* pos, double zoom,
                        RenderWorldMode which, DWORD stencilRef) {
    // We modify the pos parameter we're given (and later restore it) because
    // CoronaFrame (at least) stores a pointer to the loc! So if we passed
    // in a local variable, that would end up pointing to random stack gibberish.
    // It might not matter--I don't know--but safer not to risk it.
    t2position originalPos = *pos;
    if (which==RENDER_WORLD_DUAL) {
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

        // If the dual camera is out of this world, do nothing.
        if (!isCameraInWorld) {
            *pos = originalPos;
            return;
        }
    }
    if (stencilRef!=0) {
        D3DState d3dState;
        BeginD3DStateChanges(device, &d3dState);
        d3dState.rs.stencilEnable = TRUE;
        d3dState.rs.stencilMask = 0xFF;
        d3dState.rs.stencilWriteMask = 0xFF;
        d3dState.rs.stencilFunc = D3DCMP_EQUAL;
        d3dState.rs.stencilRef = stencilRef;
        d3dState.rs.stencilFail = D3DSTENCILOP_KEEP;
        d3dState.rs.stencilZFail = D3DSTENCILOP_KEEP;
        d3dState.rs.stencilPass = D3DSTENCILOP_KEEP;
        ApplyD3DState(device, &d3dState);
    }
    g_State.isRenderingDual = (which==RENDER_WORLD_DUAL);
    // Calling this again might have undesirable side effects; needs research.
    // Although right now I'm not seeing frobbiness being affected... not a
    // very conclusive test ofc.
    ORIGINAL_cam_render_scene(pos, zoom);
    g_State.isRenderingDual = false;
    if (stencilRef!=0) {
        EndD3DStateChanges(device);
    }
    if (which==RENDER_WORLD_DUAL) {
        // Restore the original position.
        *pos = originalPos;
    }
}


static bool DoTransition() {
    // TODO: check if it is permitted to transition right now!
    if (!g_State.isTransitioning) {
        g_State.transitionStartTime = g_State.frameTime;
        g_State.isTransitioning = true;
        return true;
    } else {
        return false;
    }
}


extern "C"
void __cdecl HOOK_cam_render_scene(t2position* pos, double zoom) {
    // Update frame counter and times.
    // TODO use actual game time, not this frames/60
    ++g_State.frame;

    g_State.frameTime = (float)((double)g_State.frame/60.0);

    // TODO: make sure that the periapt is only ever opaque and nonfunctional
    // before textures and vertices are ready?
    //
    // Reset necessary state at the start of the frame.
    g_State.didDrawPeriapt = false;


    #define LOOP_FADE_AND_TRANSITION 0

    #if !LOOP_FADE_AND_TRANSITION
    // TEMP: Just make it always active for now.
    g_State.isActive = true;
    #endif

    #if LOOP_FADE_AND_TRANSITION
    // TODO: animation loop time // TEMP variables for a repeated loop
    g_State.frameTime = fmodf(g_State.frameTime, (1.0f+FADE_DURATION+1.0f+TRANSITION_DURATION+1.0f+FADE_DURATION+1.0f));
    float fadeInStartTime = 1.0f;
    float transitionStartTime = fadeInStartTime+FADE_DURATION+1.0f;
    float fadeOutStartTime = transitionStartTime+TRANSITION_DURATION+1.0f;
    float fadeOutEndTime = fadeOutStartTime+FADE_DURATION;

    g_State.isActive = (g_State.frameTime>=fadeInStartTime
        && g_State.frameTime<fadeOutEndTime);

    // TODO: fade actually needs to be triggered by script
    if (g_State.frameTime>=fadeInStartTime
    && g_State.frameTime<(fadeInStartTime+0.25f)
    && !g_State.isFading) {
        g_State.fadeStartTime = g_State.frameTime;
        g_State.isFading = true;
        g_State.isFadingOut = false;
    }
    if (g_State.frameTime>=fadeOutStartTime
    && g_State.frameTime<(fadeOutStartTime+0.25f)
    && !g_State.isFading) {
        g_State.fadeStartTime = g_State.frameTime;
        g_State.isFading = true;
        g_State.isFadingOut = true;
    }
    #endif

    if (g_State.isFading) {
        g_State.fadeProgress =
            (g_State.frameTime-g_State.fadeStartTime)/FADE_DURATION;
        if (g_State.fadeProgress>1.0) {
            g_State.fadeProgress = 0.0;
            g_State.isFading = false;
        }
    } else {
        g_State.fadeProgress = 0.0f;
    }

    #if LOOP_FADE_AND_TRANSITION
    // TODO: transition actually needs to be triggered by script
    if (g_State.frameTime>=transitionStartTime
    && g_State.frameTime<(transitionStartTime+0.25f)
    && !g_State.isTransitioning) {
        g_State.transitionStartTime = g_State.frameTime;
        g_State.isTransitioning = true;
    }
    #endif

    if (g_State.isTransitioning) {
        g_State.transitionProgress =
            (g_State.frameTime-g_State.transitionStartTime)/TRANSITION_DURATION;
        g_State.isTransitionFirstHalf =
            (g_State.transitionProgress<TRANSITION_FIRSTHALF_END);
        if (g_State.transitionProgress>1.0) {
            g_State.transitionProgress = 0.0;
            g_State.isTransitioning = false;
            g_State.isTransitionFirstHalf = false;
        }
    } else {
        g_State.transitionProgress = 0.0f;
        g_State.isTransitionFirstHalf = false;
    }

    // Do a small zoom effect during transitions.
    if (g_State.isTransitioning) {
        // The pow() brings the peak of the effect forward in time.
        float t = powf(g_State.transitionProgress, 0.5f);
        // A gentle slope up to 1 and back down to 0:
        float curve = 0.5f-0.5f*cos(6.283185f*t);
        zoom = zoom*(1.0f+TRANSITION_ZOOM_FACTOR*curve);
    }

    IDirect3DDevice9* device = *t2_d3d9device_ptr;
    assert(device);
#if HOOKS_SPEW
    static bool spewed = false;
    if (! spewed) {
        spewed = true;
        printf("periapt: d3d9device = %08x\n", (unsigned int)(*t2_d3d9device_ptr));
    }
#endif

    if (g_State.isTransitioning) {
        // We don't want cam_render_scene to clear the stencil buffer later, so
        // we need to do it now.
        device->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 0, 0);

        DrawTransitionOverlay(device);
        if (g_State.isTransitionFirstHalf) {
            // Draw the periapt crystal now, inverting the inner/outer circles.
            DrawPeriapt(device, PERIAPT_TRANSITION_CRYSTAL);
        }

        g_State.dontClearTarget = true;
        g_State.dontClearStencil = true;
        RenderWorld(device, pos, zoom, RENDER_WORLD_REAL, STENCILREF_REAL);
        g_State.dontClearTarget = false;
        g_State.dontClearStencil = false;
    } else {
        // Render the real world normally.
        g_State.dontClearTarget = false;
        g_State.dontClearStencil = false;
        RenderWorld(device, pos, zoom, RENDER_WORLD_REAL, STENCILREF_ZERO);
    }

    // We only need to render the second pass if we're transitioning or the
    // periapt is up.
    if (g_Periapt.dualRender || g_State.isTransitioning) {
        // The original cam_render_scene will clear target+zbuffer+stencil
        // before drawing. We want to keep the previous scene render, so we
        // prevent clearing the target. And we want to keep what we've just
        // put in the stencil, so we prevent clearing the stencil too.
        g_State.dontClearTarget = true;
        g_State.dontClearStencil = true;
        RenderWorld(device, pos, zoom, RENDER_WORLD_DUAL, STENCILREF_DUAL);
        g_State.dontClearTarget = false;
        g_State.dontClearStencil = false;
    }

    if (g_State.didDrawPeriapt) {
        // Draw facets over the whole thing, including whatever is in
        // the crystal.
        if (g_State.texturesReady && g_State.verticesReady) {
            device->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
            DrawPeriapt(device, PERIAPT_FACETS);
        }
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
    // Just don't render overlays in the dual view, unless we're transitioning.
    if (g_State.isRenderingDual && !g_State.isTransitioning)
        return;

    g_State.isDrawingOverlays = true;
    ORIGINAL_dark_render_overlays();
    g_State.isDrawingOverlays = false;
}

extern "C"
void __cdecl HOOK_rendobj_render_object(t2id obj, UCHAR* clut, ULONG fragment) {
    // Check if we are rendering the periapt object.
    g_State.isDrawingPeriapt = false;
    if (g_Periapt.dualRender && g_State.isDrawingOverlays) {
        const char* name = t2_modelname_Get(obj);
        g_State.isDrawingPeriapt = (name && (stricmp(name, "periaptv") == 0));
    }

    if (g_State.isDrawingPeriapt) {
        g_State.didDrawPeriapt = true;

        IDirect3DDevice9* device = *t2_d3d9device_ptr;
        if (g_State.isTransitioning
        && g_State.isTransitionFirstHalf) {
            // Draw the periapt now, because the dual one might get completely
            // clipped. That would only require us to draw the dual periapt, but
            // to ensure both periapts match up, we draw them both here during
            // the first half of the transition.
            DrawPeriapt(device, PERIAPT_TRANSITION_CRYSTAL_Z_ONLY);
            if (g_State.isRenderingDual) {
                DrawPeriapt(device, PERIAPT_TRANSITION_BODY_DUAL);
            } else {
                DrawPeriapt(device, PERIAPT_TRANSITION_BODY_REAL);
                // Also need to call the original to ensure this frame's periapt
                // render state gets saved.
                ORIGINAL_rendobj_render_object(obj, clut, fragment);
            }
        } else {
            if (g_State.isRenderingDual) {
                // Skip the viewmodel in the dual pass.
            } else {
                ORIGINAL_rendobj_render_object(obj, clut, fragment);
            }
        }
    } else {
        // Render other objects normally.
        ORIGINAL_rendobj_render_object(obj, clut, fragment);
    }
}

extern "C"
void __cdecl HOOK_mm_hardware_render(t2mmsmodel *m) {
    if (g_State.isDrawingOverlays
    && g_State.isDrawingPeriapt
    && g_Periapt.dualRender) {
        char *base = ((char *)m) + m->smatr_off;
        // Find the textures for all parts of the periapt.
        g_State.crystalTexture = NULL;
        g_State.facetsTexture = NULL;
        g_State.overlayTexture = NULL;
        g_State.part1Texture = NULL;
        g_State.part2Texture = NULL;
        for (int i=0; i<m->smatrs; ++i) {
            char *name;
            unsigned int handle;
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
                int i = ((t2material*)handle)->cache_index;
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
                    } else if (stricmp(name, "pericry3.png")==0) {
                        g_State.facetsTexture = tex;
                    } else if (stricmp(name, "peri1.png")==0) {
                        g_State.part1Texture = tex;
                    } else if (stricmp(name, "peri2.png")==0) {
                        g_State.part2Texture = tex;
                    }
                }
            }
        }
        PeriaptVertexIndex = 0;
    }

    // We don't want to do any drawing into the stencil until all textures
    // are ready and loaded into the cache.
    g_State.texturesReady =
        (g_State.crystalTexture && g_State.facetsTexture
        && g_State.overlayTexture && g_State.part1Texture
        && g_State.part2Texture);
    // We would reset g_State.verticesReady here too, but for the first half
    // of the transition, we need to use the vertices from the previous frame.
    g_State.partCount = 0;

    ORIGINAL_mm_hardware_render(m);
}

extern "C"
int __cdecl HOOK_mDrawTriangleLists(IDirect3DDevice9 *device, D3DPRIMITIVETYPE PrimitiveType,
    UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) {

    // If we're not drawing the viewmodel, just do the original draw and stop.
    if (!g_State.isDrawingOverlays
    || !g_State.isDrawingPeriapt
    || !g_State.texturesReady
    || !g_Periapt.dualRender) {
        return device->DrawPrimitiveUP(PrimitiveType, PrimitiveCount,
            pVertexStreamZeroData, VertexStreamZeroStride);
    }

    // If we're drawing the dual pass of the world, skip the periapt.
    if (g_State.isRenderingDual) {
        return S_OK;
    }

    // From this point, we know we're rending the periapt, and its in the real.

    // Just some sanity checks.
    assert(PrimitiveType==D3DPT_TRIANGLELIST);
    assert(3*PrimitiveCount<=PERIAPT_VERTEX_COUNT);
    assert(VertexStreamZeroStride==sizeof(struct PrimVertex));

    // When transitioning, we never want to save dual vertices, because they're
    // on the other side of the world!. But we still want to count parts, so
    // that during the first half we know when to draw the periapt.
    int isLastPart = (g_State.partCount==PERIAPT_PART_COUNT-1);
    if (!g_State.isRenderingDual) {
        // Save the transformed vertices for all the parts, so we can draw them in
        // the order that we need.
        IDirect3DBaseTexture9 *texture;
        device->GetTexture(0, &texture);
        int partIndex = PERIAPT_IGNORE;
        if (texture==g_State.crystalTexture) {
            partIndex = PERIAPT_CRYSTAL;
        } else if (texture==g_State.facetsTexture) {
            // Ignore it.
        } else if (texture==g_State.overlayTexture) {
            // Ignore it.
        } else if (texture==g_State.part1Texture) {
            partIndex = PERIAPT_PART_1;
        } else if (texture==g_State.part2Texture) {
            partIndex = PERIAPT_PART_2;
        }
        if (partIndex!=PERIAPT_IGNORE) {
            assert((PeriaptVertexIndex+3*PrimitiveCount)<=PERIAPT_VERTEX_COUNT);
            memcpy(&PeriaptVertex[PeriaptVertexIndex], pVertexStreamZeroData,
                3*PrimitiveCount*sizeof(struct PrimVertex));
            PeriaptPrimitiveCount[partIndex] = PrimitiveCount;
            PeriaptPrimitiveStart[partIndex] = PeriaptVertexIndex;
            PeriaptVertexIndex += 3*PrimitiveCount;
        }
        if (isLastPart) {
            if (!g_State.verticesReady) {
                g_State.verticesReady = true;
                printf("Vertices are ready\n");
            }
        }
    }
    ++g_State.partCount;

    // We don't know what order the draw calls for the periapt are issued in,
    // so we do nothing until the last one, then we do all the drawing we need.
    if (!isLastPart) {
        return S_OK;
    }

    // if (!g_State.verticesReady) {
    //     printf("Vertices not ready ?????? : skipping periapt primitive draw\n");
    //     // ?????????????
    //     return S_OK;
    // }

/*
    OKAY, issues:
    - if the teleport is immediate, the periapt doesn't render in the outer
      ring. the only way we might make that happen is by reusing last frame's
      periapt vertices (not ideal, but the 1 frame jumpback might not be a big
      deal?)
    - if the teleport is not immediate, then .... this makes the game logic and
      teleport testing MUCH MUCH harder, because then the player may have moved
      into an impossible position. NO! **TELEPORT MUST BE IMMEDIATE**
    - also my rewrite to get the transition working has broken the crystal
      going opaque, so fix that up.
    - will need a "panic button" flag so that if the dual position moves into
      solid while the transition is ongoing, it will have to jump thetransition
      to done. shouldnt happen often, if the timing of the transition is
      reasonable and i implement lookahead/larger sphere testing of the dual
      position.
    - transition should start more from the "heart" of the gem
    - transition area should not be solid red (what should it be though?)
    - when i have transition art i like, figure out the clip zones so that we
      can then clip both inner and outer circle renders to minimise overhead
    - also no need for the clip zone to be configurable from outside the osm,
      (though being able to turn it off is still useful for testing)
*/

    if (g_State.isTransitioning && g_State.isTransitionFirstHalf) {
        // The software view clipping means that if the dual periapt is not
        // on screen, this function won't even be called! But we need to
        // render the periapt. So in the first half transition, we drew
        // last frame's periapt in rendobj_render_obj() already.
        if (g_State.isRenderingDual) {
            // Ignore.
        } else {
            // Save this frame's state for next frame.
            DrawPeriapt(device, PERIAPT_SAVE_STATE_ONLY);
        }
    } else {
        if (!g_State.isRenderingDual) {
            if (g_State.isFading) {
                DrawPeriapt(device, PERIAPT_FADE);
            } else if (g_State.isActive) {
                DrawPeriapt(device, PERIAPT_ACTIVE);
            } else {
                DrawPeriapt(device, PERIAPT_NORMAL);
            }
        }
    }

    return S_OK;
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

    if (g_State.isRenderingDual
    && !g_State.isTransitioning
    && g_Periapt.dualCull) {
        // Adjust the portal clipping rectangle for the second render pass.
        //
        // For the first pass l,t = 0,0 and r,b = w,h (screen dimensions);
        // but for the second pass we want to only include portals that would
        // be rendered in the periapt viewmodel, or in the transition area.
        //
        // Clip values to encompass the periapt (outwith transitions):
        float clipLeft = 0.51f;
        float clipRight = 1.0f;
        float clipTop = 0.31f;
        float clipBottom = 1.0f;
        l = (int)(w*clipLeft);
        r = (int)(w*clipRight);
        t = (int)(h*clipTop);
        b = (int)(h*clipBottom);
    }

    clip->l = l<<16;
    clip->t = t<<16;
    clip->r = r<<16;
    clip->b = b<<16;
    clip->tl = clip->l + clip->t;
    clip->tr = clip->r - clip->t;
    clip->bl = clip->l - clip->b;
    clip->br = clip->r + clip->b;
}

/***************************************************************************/
/*** Hooking and unhooking ***/

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
        (unsigned int)GameInfoTable.cam_render_scene,
        (unsigned int)&TRAMPOLINE_cam_render_scene,
        (unsigned int)&BYPASS_cam_render_scene,
        GameInfoTable.cam_render_scene_preamble);
    hooks_spew("Hooking cD8Renderer::Clear...\n");
    install_hook(&hooked_cD8Renderer_Clear,
        (unsigned int)GameInfoTable.cD8Renderer_Clear,
        (unsigned int)&TRAMPOLINE_cD8Renderer_Clear,
        (unsigned int)&BYPASS_cD8Renderer_Clear,
        GameInfoTable.cD8Renderer_Clear_preamble);
    hooks_spew("Hooking dark_render_overlays...\n");
    install_hook(&hooked_dark_render_overlays,
        (unsigned int)GameInfoTable.dark_render_overlays,
        (unsigned int)&TRAMPOLINE_dark_render_overlays,
        (unsigned int)&BYPASS_dark_render_overlays,
        GameInfoTable.dark_render_overlays_preamble);
    hooks_spew("Hooking rendobj_render_object...\n");
    install_hook(&hooked_rendobj_render_object,
        (unsigned int)GameInfoTable.rendobj_render_object,
        (unsigned int)&TRAMPOLINE_rendobj_render_object,
        (unsigned int)&BYPASS_rendobj_render_object,
        GameInfoTable.rendobj_render_object_preamble);
    hooks_spew("Hooking explore_portals...\n");
    install_hook(&hooked_explore_portals,
        (unsigned int)GameInfoTable.explore_portals,
        (unsigned int)&TRAMPOLINE_explore_portals,
        (unsigned int)&BYPASS_explore_portals,
        GameInfoTable.explore_portals_preamble);
    hooks_spew("Hooking initialize_first_region_clip...\n");
    install_hook(&hooked_initialize_first_region_clip,
        (unsigned int)GameInfoTable.initialize_first_region_clip,
        (unsigned int)&TRAMPOLINE_initialize_first_region_clip,
        (unsigned int)&BYPASS_initialize_first_region_clip,
        GameInfoTable.initialize_first_region_clip_preamble);
    hooks_spew("Hooking mm_hardware_render...\n");
    install_hook(&hooked_mm_hardware_render,
        (unsigned int)GameInfoTable.mm_hardware_render,
        (unsigned int)&TRAMPOLINE_mm_hardware_render,
        (unsigned int)&BYPASS_mm_hardware_render,
        GameInfoTable.mm_hardware_render_preamble);
    hooks_spew("Hooking mDrawTriangleLists...\n");
    install_hook(&hooked_mDrawTriangleLists,
        (unsigned int)GameInfoTable.mDrawTriangleLists,
        (unsigned int)&TRAMPOLINE_mDrawTriangleLists,
        (unsigned int)&BYPASS_mDrawTriangleLists,
        GameInfoTable.mDrawTriangleLists_preamble);
}

void remove_all_hooks() {
    hooks_spew("Unhooking cam_render_scene...\n");
    remove_hook(&hooked_cam_render_scene,
        (unsigned int)GameInfoTable.cam_render_scene,
        (unsigned int)&TRAMPOLINE_cam_render_scene,
        (unsigned int)&BYPASS_cam_render_scene,
        GameInfoTable.cam_render_scene_preamble);
    hooks_spew("Unhooking cD8Renderer::Clear...\n");
    remove_hook(&hooked_cD8Renderer_Clear,
        (unsigned int)GameInfoTable.cD8Renderer_Clear,
        (unsigned int)&TRAMPOLINE_cD8Renderer_Clear,
        (unsigned int)&BYPASS_cD8Renderer_Clear,
        GameInfoTable.cD8Renderer_Clear_preamble);
    hooks_spew("Unhooking dark_render_overlays...\n");
    remove_hook(&hooked_dark_render_overlays,
        (unsigned int)GameInfoTable.dark_render_overlays,
        (unsigned int)&TRAMPOLINE_dark_render_overlays,
        (unsigned int)&BYPASS_dark_render_overlays,
        GameInfoTable.dark_render_overlays_preamble);
    hooks_spew("Unhooking rendobj_render_object...\n");
    remove_hook(&hooked_rendobj_render_object,
        (unsigned int)GameInfoTable.rendobj_render_object,
        (unsigned int)&TRAMPOLINE_rendobj_render_object,
        (unsigned int)&BYPASS_rendobj_render_object,
        GameInfoTable.rendobj_render_object_preamble);
    hooks_spew("Unhooking explore_portals...\n");
    remove_hook(&hooked_explore_portals,
        (unsigned int)GameInfoTable.explore_portals,
        (unsigned int)&TRAMPOLINE_explore_portals,
        (unsigned int)&BYPASS_explore_portals,
        GameInfoTable.explore_portals_preamble);
    hooks_spew("Unhooking initialize_first_region_clip...\n");
    remove_hook(&hooked_initialize_first_region_clip,
        (unsigned int)GameInfoTable.initialize_first_region_clip,
        (unsigned int)&TRAMPOLINE_initialize_first_region_clip,
        (unsigned int)&BYPASS_initialize_first_region_clip,
        GameInfoTable.initialize_first_region_clip_preamble);
    hooks_spew("Unhooking mm_hardware_render...\n");
    remove_hook(&hooked_mm_hardware_render,
        (unsigned int)GameInfoTable.mm_hardware_render,
        (unsigned int)&TRAMPOLINE_mm_hardware_render,
        (unsigned int)&BYPASS_mm_hardware_render,
        GameInfoTable.mm_hardware_render_preamble);
    hooks_spew("Unhooking mDrawTriangleLists...\n");
    remove_hook(&hooked_mDrawTriangleLists,
        (unsigned int)GameInfoTable.mDrawTriangleLists,
        (unsigned int)&TRAMPOLINE_mDrawTriangleLists,
        (unsigned int)&BYPASS_mDrawTriangleLists,
        GameInfoTable.mDrawTriangleLists_preamble);
}

/***************************************************************************/
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
    const char *message = pMsg->message;

    if (stricmp(message, "Sim") == 0) {
        BOOL fStarting = static_cast<sSimMsg*>(pMsg)->fStarting;
        printf(PREFIX "%s: fStarting=%s\n", message, (fStarting ? "true" : "false"));
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
    else if (stricmp(message, "DarkGameModeChange") == 0) {
        BOOL fEntering = static_cast<sDarkGameModeScrMsg*>(pMsg)->fEntering;
        printf(PREFIX "%s: fEntering=%s\n", message, (fEntering ? "true" : "false"));
    }
    else if (stricmp(message, "BeginScript") == 0) {
        printf(PREFIX "%s\n", message);
        // TODO: load settings from saved data (if any)
        g_Periapt.dualRender = false;
        g_Periapt.dualOffset.x = 0.0f;
        g_Periapt.dualOffset.y = 0.0f;
        g_Periapt.dualOffset.z = 1.0f;
        g_Periapt.dualCull = false;
        g_Periapt.depthCull = false;
        g_Periapt.depthCullDistance = 256.0f;
    }
    else if (stricmp(message, "EndScript") == 0) {
        printf(PREFIX "%s\n", message);
    }
    else if (stricmp(message, "TurnOn") == 0) {
        printf(PREFIX "%s\n", message);
        enable_hooks();
    }
    else if (stricmp(message, "TurnOff") == 0) {
        printf(PREFIX "%s\n", message);
        disable_hooks();
    }
    else if (stricmp(message, "Translocate") == 0) {
        printf(PREFIX "%s\n", message);
        bool ok = DoTransition();
        if (pReply) *pReply = cMultiParm(ok);
    }
    else if (stricmp(message, "SetDualRender") == 0) {
        bool v = static_cast<bool>(pMsg->data);
        printf(PREFIX "%s(%s)\n", message, (v?"true":"false"));
        g_Periapt.dualRender = v;
    }
    else if (stricmp(message, "SetDualOffset") == 0) {
        const mxs_vector *v = static_cast<const mxs_vector *>(pMsg->data);
        if (v) {
            printf(PREFIX "%s(<%0.3f,%0.3f,%0.3f>)\n", message, v->x, v->y, v->z);
            g_Periapt.dualOffset.x = v->x;
            g_Periapt.dualOffset.y = v->y;
            g_Periapt.dualOffset.z = v->z;
        } else {
            printf(PREFIX "%s(<not a vector>)\n", message);
        }
    }
    else if (stricmp(message, "SetDualCull") == 0) {
        bool v = static_cast<bool>(pMsg->data);
        printf(PREFIX "%s(%s)\n", message, (v?"true":"false"));
        g_Periapt.dualCull = v;
    }
    else if (stricmp(message, "SetDepthCull") == 0) {
        bool v = static_cast<bool>(pMsg->data);
        printf(PREFIX "%s(%s)\n", message, (v?"true":"false"));
        g_Periapt.depthCull = v;
    }
    else if (stricmp(message, "SetDepthCullDistance") == 0) {
        float d = static_cast<float>(pMsg->data);
        printf(PREFIX "%s(%0.3f)\n", message, d);
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
    static BOOL didAllocConsole = false;
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
        BOOL shouldAllocConsole = (!isEditor || !isIdentified);
        BOOL shouldRedirectStdout = (isEditor || shouldAllocConsole);
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
