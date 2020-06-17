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

#include <cstring>
#include <new>
#include <exception>
#include <string>
#include <strings.h>
#include <d3d9.h>

#include "t2types.h"
#include "bypass.h"

using namespace std;

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

static void fixup_ptr(LPVOID& ptr, DWORD baseAddress) {
    ptr = (void *)((DWORD)ptr + baseAddress);
}

struct GameInfo {
    void *cam_render_scene;
    DWORD cam_render_scene_preamble;
    void *cD8Renderer_Clear;
    DWORD cD8Renderer_Clear_preamble;
    void *d3d9device_ptr;
};

static GameInfo GameInfoTable = {};

static const GameInfo PerIdentityGameTable[ExeIdentityCount] = {
    // ExeThief_v126
    {
        (void *)0x001bc7a0UL, 9,    // cam_render_scene
        (void *)0x0020ce80UL, 6,    // cD8Renderer_Clear
        (void *)0x005d8118UL,       // d3d9device_ptr
    },
    // ExeDromEd_v126
    {
        (void *)0x00286960UL, 9,    // cam_render_scene
        (void *)0x002e62a0UL, 6,    // cD8Renderer_Clear
        (void *)0x016e7b50UL,       // d3d9device_ptr
    },
    // ExeThief_v127
    {
        (void *)0x001bd820UL, 9,    // cam_render_scene
        (void *)0x0020dff0UL, 6,    // cD8Renderer_Clear
        (void *)0x005d915cUL,       // d3d9device_ptr
    },
    // ExeDromEd_v127
    {
        (void *)0x002895c0UL, 9,    // cam_render_scene
        (void *)0x002e8e60UL, 6,    // cD8Renderer_Clear
        (void *)0x016ebce0UL,       // d3d9device_ptr
    },
};

void LoadGameInfoTable(ExeIdentity identity) {
    GameInfoTable = PerIdentityGameTable[identity];
    DWORD baseAddress = (DWORD)GetModuleHandle(NULL);
    fixup_ptr(GameInfoTable.cam_render_scene, baseAddress);
    fixup_ptr(GameInfoTable.cD8Renderer_Clear, baseAddress);
    fixup_ptr(GameInfoTable.d3d9device_ptr, baseAddress);
}

/*** Hooks and crooks ***/

#define HOOKS_SPEW 1

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

static IDirect3DDevice9** p_d3d9device;
static bool prevent_target_stencil_clear;

extern "C"
void __cdecl HOOK_cam_render_scene(t2position* pos, double zoom) {
    if (! p_d3d9device) {
        if (GameInfoTable.d3d9device_ptr) {
            p_d3d9device = (IDirect3DDevice9**)GameInfoTable.d3d9device_ptr;
            printf("p_d3d9device is %08x\n", (unsigned int)p_d3d9device);
            if (p_d3d9device) {
                printf("d3d9device is %08x\n", (unsigned int)(*p_d3d9device));
            }
        }
    }

    // Render the scene normally.
    ORIGINAL_cam_render_scene(pos, zoom);

    if (p_d3d9device) {
        // Render an SS1-style rear-view mirror.
        IDirect3DDevice9* device = *p_d3d9device;
        D3DVIEWPORT9 viewport = {};
        device->GetViewport(&viewport);

#define USE_SCISSOR 0
#define USE_STENCIL 1
#if USE_SCISSOR
        // rear-view mirror will be the top third (horizontally) and
        // quarter (vertically) of the screen.
        RECT rect = {
            (long)viewport.X+(long)viewport.Width/3,
            (long)viewport.Y+0,
            (long)viewport.X+(2*(long)viewport.Width)/3,
            (long)viewport.Y+(long)viewport.Height/4,
        };
        // // Centered rect, half the screen size:
        // RECT rect = {
        //     (long)viewport.X+(1*(long)viewport.Width)/4,
        //     (long)viewport.Y+(1*(long)viewport.Height)/4,
        //     (long)viewport.X+(3*(long)viewport.Width)/4,
        //     (long)viewport.Y+(3*(long)viewport.Height)/4,
        // };

        // FIXME: we should probably save and restore any state
        // that we fiddle with, for safety. But not for MAD SCIENCE!!
        device->SetScissorRect(&rect);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
#endif // USE_SCISSOR
#if USE_STENCIL
        D3DRECT viewportRect = {
            (long)viewport.X,
            (long)viewport.Y,
            (long)viewport.X+(long)viewport.Width,
            (long)viewport.Y+(long)viewport.Height,
        };
        D3DRECT periaptRect = {
            (long)viewport.X+(long)viewport.Width/3,
            (long)viewport.Y+0,
            (long)viewport.X+(2*(long)viewport.Width)/3,
            (long)viewport.Y+(long)viewport.Height/4,
        };
        // device->Clear(1, &viewportRect, D3DCLEAR_TARGET, D3DCOLOR_RGBA(255,0,0,255), 0, 0);
        // device->Clear(1, &periaptRect, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0,255,255,255), 0, 0);
        device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        device->Clear(1, &viewportRect, D3DCLEAR_STENCIL, 0, 0, 0);
        device->Clear(1, &periaptRect, D3DCLEAR_STENCIL, 0, 0, 1);
        // Do some really, really hacky 'rounded' corners so the
        // difference from the scissor is clear:
        long cornerWidth = 30;
        long cornerHeight = 30;
        for (int y=0; y<2; ++y) {
            for (int x=0; x<2; ++x) {
                D3DRECT cornerRect = {
                    ((x == 0) ? periaptRect.x1 : (periaptRect.x2 - cornerWidth)),
                    ((y == 0) ? periaptRect.y1 : (periaptRect.y2 - cornerHeight)),
                    ((x == 0) ? (periaptRect.x1 + cornerWidth) : periaptRect.x2),
                    ((y == 0) ? (periaptRect.y1 + cornerHeight) : periaptRect.y2),
                };
                device->Clear(1, &cornerRect, D3DCLEAR_STENCIL, 0, 0, 0);
            }
        }
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
        prevent_target_stencil_clear = true;
#endif // USE_STENCIL

        // And render the reverse view.

        // We modify the pos parameter we're given (and later restore it) because
        // CoronaFrame (at least) stores a pointer to the loc! So if we passed
        // in a local variable, that would end up pointing to random stack gibberish.
        // It might not matter--I don't know--but safer not to risk it.
        t2position originalPos = *pos;

        // pos->fac.y = ~pos->fac.y; // reverses the pitch, but not in a nice way.
        pos->fac.z += T2_ANGLE_PI; // 180 degree rotation to 'behind me'.

        // // Move the camera a touch since the view is at the top of the screen.
        // pos->loc.vec.z += 1.0; 
        // // If we move the location, then we ought to cancel the cell+hint metadata:
        // pos->loc.cell = -1;
        // pos->loc.hint = -1;

        // PROBLEM: this ends up calling device->Clear() again and clearing
        // the whole stencil buffer! Blargh.

        // Calling this again might have undesirable side effects; needs research.
        // Although right now I'm not seeing frobbiness being affected... not a
        // very conclusive test ofc.
        ORIGINAL_cam_render_scene(pos, zoom);

        *pos = originalPos;

#if USE_STENCIL
        prevent_target_stencil_clear = false;
        device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
#endif
#if USE_SCISSOR
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
#endif
    }
}

extern "C"
void __stdcall HOOK_cD8Renderer_Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    if (prevent_target_stencil_clear) {
        Flags &= ~(D3DCLEAR_TARGET | D3DCLEAR_STENCIL);
    }
    ORIGINAL_cD8Renderer_Clear(Count, pRects, Flags, Color, Z, Stencil);
}

// TODO: I don't think we want to hook and unhook many parts individually,
// so change this to use a single flag for if all hooks are installed or not.
bool hooked_cam_render_scene;
bool hooked_cD8Renderer_Clear;

void install_all_hooks() {
    install_hook(&hooked_cam_render_scene,
        (uint32_t)GameInfoTable.cam_render_scene,
        (uint32_t)&TRAMPOLINE_cam_render_scene,
        (uint32_t)&BYPASS_cam_render_scene,
        GameInfoTable.cam_render_scene_preamble);
    install_hook(&hooked_cD8Renderer_Clear,
        (uint32_t)GameInfoTable.cD8Renderer_Clear,
        (uint32_t)&TRAMPOLINE_cD8Renderer_Clear,
        (uint32_t)&BYPASS_cD8Renderer_Clear,
        GameInfoTable.cD8Renderer_Clear_preamble);
}

void remove_all_hooks() {
    remove_hook(&hooked_cD8Renderer_Clear,
        (uint32_t)GameInfoTable.cD8Renderer_Clear,
        (uint32_t)&TRAMPOLINE_cD8Renderer_Clear,
        (uint32_t)&BYPASS_cD8Renderer_Clear,
        GameInfoTable.cD8Renderer_Clear_preamble);
    remove_hook(&hooked_cam_render_scene,
        (uint32_t)GameInfoTable.cam_render_scene,
        (uint32_t)&TRAMPOLINE_cam_render_scene,
        (uint32_t)&BYPASS_cam_render_scene,
        GameInfoTable.cam_render_scene_preamble);
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
        printf("Sim: fStarting=%s\n", (fStarting ? "true" : "false"));
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
        printf("DarkGameModeChange: fEntering=%s\n", (fEntering ? "true" : "false"));
    }
    if (stricmp(pMsg->message, "BeginScript") == 0) {
        printf("BeginScript\n");
    }
    if (stricmp(pMsg->message, "EndScript") == 0) {
        printf("EndScript\n");
    }
    else if (stricmp(pMsg->message, "TurnOn") == 0) {
        enable_hooks();
    }
    else if (stricmp(pMsg->message, "TurnOff") == 0) {
        disable_hooks();
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

#define PREFIX "periapt: "

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
            int top = 480;
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
        // DeactivateAllTrampolines();
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
