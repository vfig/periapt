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
// Data to be accessed:
IDirect3DDevice9 **t2_d3d9device_ptr;
void *t2_modelnameprop_ptr;
t2position *t2_portal_camera_pos_ptr;
int *t2_mmd_version_ptr;
void **t2_mmd_smatrs_ptr;

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
    DWORD mm_setup_material;
    DWORD mm_setup_material_preamble;
    DWORD mm_hardware_render;
    DWORD mm_hardware_render_preamble;
    DWORD mDrawTriangleLists;
    DWORD mDrawTriangleLists_preamble;
    DWORD mDrawTriangleLists_resume;
    // Functions to be called:
    DWORD ObjPosGet;
    DWORD ObjPosSetLocation;
    // Data to be accessed:
    DWORD d3d9device_ptr;
    DWORD modelnameprop;
    DWORD portal_camera_pos;
    DWORD mmd_version;
    DWORD mmd_smatrs;
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
        0, 0, /* TODO */    // mm_setup_material
        0, 0, /* TODO */    // mm_hardware_render
        0, 0, /* TODO */    // mDrawTriangleLists
        0,                  // mDrawTriangleLists_resume
        0,                  // ObjPosGet
        0,                  // ObjPosSetLocation
        0x005d8118UL,       // d3d9device_ptr
        0,                  // modelnameprop
        0,                  // portal_camera_pos
        0,                  // mmd_version
        0,                  // mmd_smatrs
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
        0, 0, /* TODO */    // mm_setup_material
        0, 0, /* TODO */    // mm_hardware_render
        0, 0, /* TODO */    // mDrawTriangleLists
        0,                  // mDrawTriangleLists_resume
        0,                  // ObjPosGet
        0,                  // ObjPosSetLocation
        0x016e7b50UL,       // d3d9device_ptr
        0,                  // modelnameprop
        0,                  // portal_camera_pos
        0,                  // mmd_version
        0,                  // mmd_smatrs
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
        0, 0, /* TODO */    // mm_setup_material
        0, 0, /* TODO */    // mm_hardware_render
        0, 0, /* TODO */    // mDrawTriangleLists
        0,                  // mDrawTriangleLists_resume
        0,                  // ObjPosGet
        0,                  // ObjPosSetLocation
        0x005d915cUL,       // d3d9device_ptr
        0x005ce4d8UL,       // modelnameprop
        0x00460bf0UL,       // portal_camera_pos
        0,                  // mmd_version
        0,                  // mmd_smatrs
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
        0x002e9be0UL, 7,    // mm_setup_material
        0x002e9ab0UL, 8,    // mm_hardware_render
//        0x002e7990UL, 7,    // mDrawTriangleLists
        0x002e7a38UL, 6,    // mDrawTriangleLists
        0x002e7a40UL,       // mDrawTriangleLists_resume
        0x001e4680UL,       // ObjPosGet
        0x001e49e0UL,       // ObjPosSetLocation
        0x016ebce0UL,       // d3d9device_ptr
        0x016e0f84UL,       // modelnameprop
        0x0140216cUL,       // portal_camera_pos
        0x0172d8b8UL,       // mmd_version
        0x0172d8b4UL,       // mmd_smatrs
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
    fixup_addr(&GameInfoTable.mm_setup_material, base);
    fixup_addr(&GameInfoTable.mm_hardware_render, base);
    fixup_addr(&GameInfoTable.mDrawTriangleLists, base);
    fixup_addr(&GameInfoTable.mDrawTriangleLists_resume, base);
    fixup_addr(&GameInfoTable.ObjPosGet, base);
    fixup_addr(&GameInfoTable.ObjPosSetLocation, base);
    fixup_addr(&GameInfoTable.d3d9device_ptr, base);
    fixup_addr(&GameInfoTable.modelnameprop, base);
    fixup_addr(&GameInfoTable.portal_camera_pos, base);
    fixup_addr(&GameInfoTable.mmd_version, base);
    fixup_addr(&GameInfoTable.mmd_smatrs, base);

    t2_ObjPosGet = (t2position*(*)(t2id))GameInfoTable.ObjPosGet;
    ADDR_ObjPosSetLocation = GameInfoTable.ObjPosSetLocation;
    t2_d3d9device_ptr = (IDirect3DDevice9**)GameInfoTable.d3d9device_ptr;
    t2_modelnameprop_ptr = (void*)GameInfoTable.modelnameprop;
    t2_portal_camera_pos_ptr = (t2position*)GameInfoTable.portal_camera_pos;
    t2_mmd_version_ptr = (int*)GameInfoTable.mmd_version;
    t2_mmd_smatrs_ptr = (void**)GameInfoTable.mmd_smatrs;
    RESUME_initialize_first_region_clip = GameInfoTable.initialize_first_region_clip_resume;
    RESUME_mDrawTriangleLists = GameInfoTable.mDrawTriangleLists_resume;

#if HOOKS_SPEW
    printf("periapt: cam_render_scene = %08x\n", (unsigned int)GameInfoTable.cam_render_scene);
    printf("periapt: cD8Renderer_Clear = %08x\n", (unsigned int)GameInfoTable.cD8Renderer_Clear);
    printf("periapt: dark_render_overlays = %08x\n", (unsigned int)GameInfoTable.dark_render_overlays);
    printf("periapt: rendobj_render_object = %08x\n", (unsigned int)GameInfoTable.rendobj_render_object);
    printf("periapt: explore_portals = %08x\n", (unsigned int)GameInfoTable.explore_portals);
    printf("periapt: initialize_first_region_clip = %08x\n", (unsigned int)GameInfoTable.initialize_first_region_clip);
    printf("periapt: mm_setup_material = %08x\n", (unsigned int)GameInfoTable.mm_setup_material);
    printf("periapt: mm_hardware_render = %08x\n", (unsigned int)GameInfoTable.mm_hardware_render);
    printf("periapt: mDrawTriangleLists = %08x\n", (unsigned int)GameInfoTable.mDrawTriangleLists);
    printf("periapt: mDrawTriangleLists_resume = %08x\n", (unsigned int)GameInfoTable.mDrawTriangleLists_resume);
    printf("periapt: t2_ObjPosGet = %08x\n", (unsigned int)t2_ObjPosGet);
    printf("periapt: ADDR_ObjPosSetLocation = %08x\n", (unsigned int)ADDR_ObjPosSetLocation);
    printf("periapt: CALL_ObjPosSetLocation = %08x\n", (unsigned int)CALL_ObjPosSetLocation);
    printf("periapt: t2_d3d9device_ptr = %08x\n", (unsigned int)t2_d3d9device_ptr);
    printf("periapt: t2_modelnameprop_ptr = %08x\n", (unsigned int)t2_modelnameprop_ptr);
    printf("periapt: t2_portal_camera_pos_ptr = %08x\n", (unsigned int)t2_portal_camera_pos_ptr);
    printf("periapt: t2_mmd_version_ptr = %08x\n", (unsigned int)t2_mmd_version_ptr);
    printf("periapt: t2_mmd_smatrs_ptr = %08x\n", (unsigned int)t2_mmd_smatrs_ptr);
    printf("periapt: RESUME_initialize_first_region_clip = %08x\n", (unsigned int)RESUME_initialize_first_region_clip);
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
    bool isRenderingDual;
    bool dontClearTargetOrStencil;
    bool isDrawingPeriapt;
    uint32_t stencilOpCounter; // 0: ignore; 1+: read op bit and decrement.
    uint32_t stencilOp; // bit==0: erase stencil; bit==1: draw into stencil
} g_State = {};

extern "C"
void __cdecl HOOK_cam_render_scene(t2position* pos, double zoom) {
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
        D3DVIEWPORT9 viewport = {};
        device->GetViewport(&viewport);
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
        g_State.dontClearTargetOrStencil = true;

        // And render the reverse view.

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

        // Calling this again might have undesirable side effects; needs research.
        // Although right now I'm not seeing frobbiness being affected... not a
        // very conclusive test ofc.
        g_State.isRenderingDual = true;
        ORIGINAL_cam_render_scene(pos, zoom);
        g_State.isRenderingDual = false;

        *pos = originalPos;

        g_State.dontClearTargetOrStencil = false;
        device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    }
}

extern "C"
void __stdcall HOOK_cD8Renderer_Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    if (g_State.dontClearTargetOrStencil) {
        Flags &= ~(D3DCLEAR_TARGET | D3DCLEAR_STENCIL);
    }
    ORIGINAL_cD8Renderer_Clear(Count, pRects, Flags, Color, Z, Stencil);
}

static void spew_obj_location(t2id obj, const char *msg) {
    t2position pos = *(t2_ObjPosGet(obj));
    printf("obj %d: %0.2f, %0.2f, %0.2f (%s)\n", obj, pos.loc.vec.x, pos.loc.vec.y, pos.loc.vec.z, msg);
}

extern "C"
void __cdecl HOOK_dark_render_overlays(void) {
    ORIGINAL_dark_render_overlays();

/*
    // Okay, now we want to render an object! How about, um, id... 16? That's the stone head!
    // We'll move it to 0,0,0 and back again!
    t2id obj = 16;
    t2position pos = *(t2_ObjPosGet(obj));
    t2location loc = {{ 0.0, 0.0, 0.0 }, -1, -1};

    static bool first_time = true;
    if (first_time) {
        first_time = false;
        spew_obj_location(obj, "Starting");
        CALL_ObjPosSetLocation(obj, &loc);
        spew_obj_location(obj, "Moved");
        // FIXME: here
        // Okay, so this doesn't seem to be rendering anything at all?
        // Or if it is, _I_ can't see it! Phooey!
        ORIGINAL_rendobj_render_object(obj, NULL, 0);
        CALL_ObjPosSetLocation(obj, &pos.loc);
        spew_obj_location(obj, "Returned");
    }
*/
}

extern "C"
void __cdecl HOOK_rendobj_render_object(t2id obj, UCHAR* clut, ULONG fragment) {
    // TODO: can probably simplify this by _not_ testing the model name, but
    //       just evaluating EVERY mesh drawn by dark_render_overlays.
    bool isPeriapt = false;
    if (g_Periapt.dualRender) {
        const char* name = t2_modelname_Get(obj);
        // printf("rendobj_render_object(%d) [%s]\n", obj, (name ? name : "null"));
        isPeriapt = (name && (stricmp(name, "periaptv") == 0));
    }
    g_State.isDrawingPeriapt = isPeriapt;

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
        if (cell_near_dist > g_Periapt.depthCullDistance) return;
    }

    ORIGINAL_explore_portals(cell);
}

extern "C"
void __cdecl HOOK_initialize_first_region_clip(int w, int h, t2clipdata *clip) {
    int l=0, r=w, t=0, b=h;

    if (g_Periapt.dualCull) {
        if (g_State.isRenderingDual) {
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

// TODO: can drop this one, i think
extern "C"
void __cdecl HOOK_mm_setup_material(int index) {
    /*
    int version = *t2_mmd_version_ptr;
    if (version==1) {
        t2smatr_v1 *mmd_smatrs = *((t2smatr_v1 **)t2_mmd_smatrs_ptr);
        char *name = mmd_smatrs[index].name;
        uint32_t handle = mmd_smatrs[index].handle;
        printf("mm_setup_material(%d): '%16s' 0x%08X\n", index, name, handle); 
    } else if (version==2) {
        t2smatr_v2 *mmd_smatrs = *((t2smatr_v2 **)t2_mmd_smatrs_ptr);
        char *name = mmd_smatrs[index].name;
        uint32_t handle = mmd_smatrs[index].handle;
        printf("mm_setup_material(%d): '%16s' 0x%08X\n", index, name, handle); 
    } else {
        printf("mm_setup_material(%d): bad version %d\n", index, version); 
    }
    */

    ORIGINAL_mm_setup_material(index);
}

extern "C"
void __cdecl HOOK_mm_hardware_render(t2mmsmodel *m) {
    if (g_Periapt.dualRender
    && !g_State.isRenderingDual
    && g_State.isDrawingPeriapt) {
        char *base = ((char *)m) + m->smatr_off;
        uint32_t size = (m->version==1)? sizeof(t2smatr_v1) : sizeof(t2smatr_v2);
        // Find which materials (if any) is the special one, and write a 1
        // bit into the stencil op for it.
        g_State.stencilOp = 0;
        g_State.stencilOpCounter = 0;
        for (int i=0; i<m->smatrs; ++i) {
            char *name = (base+i*size);
            ++g_State.stencilOpCounter;
            if (stricmp(name, "pericry1.png")==0) {
                g_State.stencilOp |= (1UL << i);
            }
        }
    }
    
    ORIGINAL_mm_hardware_render(m);
}

extern "C"
int __cdecl HOOK_mDrawTriangleLists(IDirect3DDevice9 *device, D3DPRIMITIVETYPE PrimitiveType,
    UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) {

    if (g_State.stencilOpCounter) {
        // Get the stencil op for this draw
        uint32_t op = g_State.stencilOp & 1;
        g_State.stencilOp >>= 1;
        // Apply it
        device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
        device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
        device->SetRenderState(D3DRS_STENCILREF, 0x1);
        device->SetRenderState(D3DRS_STENCILMASK, 0xffffffff);
        device->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff);
        device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
        device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
        device->SetRenderState(D3DRS_STENCILPASS, op ? D3DSTENCILOP_REPLACE : D3DSTENCILOP_ZERO);
        HRESULT result = device->DrawPrimitiveUP(PrimitiveType, PrimitiveCount,
            pVertexStreamZeroData, VertexStreamZeroStride);
        // Disable stencil ops when the last one was done.
        if (--g_State.stencilOpCounter==0) {
            device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        }
        return result;
    } else {
        return device->DrawPrimitiveUP(PrimitiveType, PrimitiveCount,
            pVertexStreamZeroData, VertexStreamZeroStride);
    }
}

// TODO: I don't think we want to hook and unhook many parts individually,
// so change this to use a single flag for if all hooks are installed or not.
bool hooked_cam_render_scene;
bool hooked_cD8Renderer_Clear;
bool hooked_dark_render_overlays;
bool hooked_rendobj_render_object;
bool hooked_explore_portals;
bool hooked_initialize_first_region_clip;
bool hooked_mm_setup_material;
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
    hooks_spew("Hooking mm_setup_material...\n");
    install_hook(&hooked_mm_setup_material,
        (uint32_t)GameInfoTable.mm_setup_material,
        (uint32_t)&TRAMPOLINE_mm_setup_material,
        (uint32_t)&BYPASS_mm_setup_material,
        GameInfoTable.mm_setup_material_preamble);
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
    hooks_spew("Unhooking mm_setup_material...\n");
    remove_hook(&hooked_mm_setup_material,
        (uint32_t)GameInfoTable.mm_setup_material,
        (uint32_t)&TRAMPOLINE_mm_setup_material,
        (uint32_t)&BYPASS_mm_setup_material,
        GameInfoTable.mm_setup_material_preamble);
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
