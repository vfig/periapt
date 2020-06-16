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

struct GameInfo {
    void *cam_render_scene;
    DWORD cam_render_scene_preamble;
    void *graphics_info_ptr;
    DWORD ofs_d3d9device;
};

static const GameInfo PerIdentityGameTable[ExeIdentityCount] = {
    // ExeThief_v126
    {
        (void *)0x001bc7a0UL, 9,    // cam_render_scene
        (void *)0x005d8d40UL, 0x3C, // graphics_info_ptr, ofs_d3d9device
    },
    // ExeDromEd_v126
    {
        (void *)0x00286960UL, 9,    // cam_render_scene
        (void *)0x016e878cUL, 0x3C, // graphics_info_ptr, ofs_d3d9device
    },
    // ExeThief_v127
    {
        (void *)0x001bd820UL, 9,    // cam_render_scene
        (void *)0x005d9d88UL, 0x3C, // graphics_info_ptr, ofs_d3d9device
    },
    // ExeDromEd_v127
    {
        (void *)0x002895c0UL, 9,    // cam_render_scene
        (void *)0x016ec920UL, 0x3C, // graphics_info_ptr, ofs_d3d9device
    },
};
static GameInfo GameInfoTable = {};

/*** Hooks and crooks ***/

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
printf("  offset: %08x (%d)\n", offset, offset);
printf("  memcpy %08x, %08x, %u\n", jmp_address, (uint32_t)&jmp, 1);
    memcpy((void *)jmp_address, &jmp, 1);
printf("  memcpy %08x, %08x, %u\n", jmp_address+1, (uint32_t)&offset, 4);
    memcpy((void *)(jmp_address+1), &offset, 4);
}

void install_hook(bool *hooked, uint32_t target, uint32_t trampoline, uint32_t bypass, uint32_t size) {
    if (! *hooked) {
        *hooked = true;
printf("hooking target %08x, trampoline %08x, bypass %08x, size %u\n", target, trampoline, bypass, size);
        DWORD targetProtection, trampolineProtection;
        VirtualProtect((void *)target, size, PAGE_EXECUTE_READWRITE, &targetProtection);
        VirtualProtect((void *)trampoline, size+5, PAGE_EXECUTE_READWRITE, &trampolineProtection);

readMem((void *)target, 32);
readMem((void *)ORIGINAL_cam_render_scene, 32);
readMem((void *)trampoline, 32);
readMem((void *)bypass, 32);

printf("memcpy %08x, %08x, %u\n", trampoline, target, size);
        memcpy((void *)trampoline, (void *)target, size);
printf("patch_jmp %08x, %08x\n", trampoline+size, target+size);
        patch_jmp(trampoline+size, target+size);
printf("patch_jmp %08x, %08x\n", target, bypass);
        patch_jmp(target, bypass);
        VirtualProtect((void *)target, size, targetProtection, NULL);
        VirtualProtect((void *)trampoline, size+5, trampolineProtection, NULL);

readMem((void *)target, 32);
readMem((void *)ORIGINAL_cam_render_scene, 32);
readMem((void *)trampoline, 32);
readMem((void *)bypass, 32);

printf("hook complete\n");
    }
}

void remove_hook(bool *hooked, uint32_t target, uint32_t trampoline, uint32_t bypass, uint32_t size) {
    (void)bypass; // Unused
    if (*hooked) {
        *hooked = false;

printf("unhooking target %08x, trampoline %08x, bypass %08x, size %u\n", target, trampoline, bypass, size);
        DWORD targetProtection; // trampolineProtection;
        VirtualProtect((void *)target, size, PAGE_EXECUTE_READWRITE, &targetProtection);
        // VirtualProtect((void *)trampoline, size+5, PAGE_EXECUTE_READWRITE, &trampolineProtection);

readMem((void *)target, 32);
readMem((void *)ORIGINAL_cam_render_scene, 32);
readMem((void *)trampoline, 32);
readMem((void *)bypass, 32);

printf("memcpy %08x, %08x, %u\n", target, trampoline, size);
        memcpy((void *)target, (void *)trampoline, size);

readMem((void *)target, 32);
readMem((void *)ORIGINAL_cam_render_scene, 32);
readMem((void *)trampoline, 32);
readMem((void *)bypass, 32);

        VirtualProtect((void *)target, size, targetProtection, NULL);
        // VirtualProtect((void *)trampoline, size+5, trampolineProtection, NULL);

printf("unhook complete\n");
    }
}

// TODO: I don't think we want to hook and unhook many parts individually,
// so change this to use a single flag for if all hooks are installed or not.
bool hooked_cam_render_scene;

static IDirect3DDevice9** p_d3d9device;

extern "C" void __cdecl HOOK_cam_render_scene(t2position* pos, double zoom) {
    if (! p_d3d9device) {
        // just test that it all works....
        do {
            LPVOID graphics_info_ptr = GameInfoTable.graphics_info_ptr;
            printf("graphics_info_ptr is %08x\n", (unsigned int)graphics_info_ptr);
            if (! graphics_info_ptr) break;
            readMem((void *)graphics_info_ptr, 0x4);

            LPVOID graphics_info = *((LPVOID*)graphics_info_ptr);
            printf("graphics_info is %08x\n", (unsigned int)graphics_info);
            if (! graphics_info) break;
            readMem((void *)graphics_info, 0x40);

            LPVOID d3d9device_ptr = ((BYTE*)graphics_info + GameInfoTable.ofs_d3d9device);
            printf("d3d9device_ptr is %08x\n", (unsigned int)d3d9device_ptr);
            if (! d3d9device_ptr) break;
            readMem((void *)d3d9device_ptr, 0x4);

            LPVOID d3d9device = *((LPVOID*)d3d9device_ptr);
            printf("d3d9device is %08x\n", (unsigned int)d3d9device);
            if (! d3d9device) break;
            readMem((void *)d3d9device, 0x20);

            LPVOID vtable = *((LPVOID*)d3d9device);
            printf("vtable is %08x\n", (unsigned int)vtable);
            if (! vtable) break;
            readMem((void *)vtable, 0x80);
        } while(0);

        // Grab the pointer
        if (GameInfoTable.graphics_info_ptr) {
            p_d3d9device = (IDirect3DDevice9**)(*((BYTE**)GameInfoTable.graphics_info_ptr) + GameInfoTable.ofs_d3d9device);
            printf("p_d3d9device is %08x\n", (unsigned int)p_d3d9device);
        }
    }

    // Render the scene normally.
    ORIGINAL_cam_render_scene(pos, zoom);

    if (p_d3d9device) {
        // Render an SS1-style rear-view mirror.
        IDirect3DDevice9* device = *p_d3d9device;
        D3DVIEWPORT9 viewport = {};
        device->GetViewport(&viewport);

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

        // TODO: also use the stencil test, later maybe. Don't forget
        // to clear the stencil, eh?
        // D3DRect clearRect = {
        //     viewport.X,
        //     viewport.Y,
        //     viewport.X+viewport.Width,
        //     viewport.Y+viewport.Height
        // };
        // DWORD flags = D3DCLEAR_TARGET;
        // D3DCOLOR color = D3DCOLOR_RGBA(255, 0, 0, 255);
        // float z = 0;
        // DWORD stencil = 0;
        // device->Clear(1, &clearRect, flags, color, z, stencil);

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

        // Calling this again might have undesirable side effects; needs research.
        // Although right now I'm not seeing frobbiness being affected... not a
        // very conclusive test ofc.
        ORIGINAL_cam_render_scene(pos, zoom);

        *pos = originalPos;
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    }
}

void install_all_hooks() {
    install_hook(&hooked_cam_render_scene,
        (uint32_t)GameInfoTable.cam_render_scene,
        (uint32_t)&TRAMPOLINE_cam_render_scene,
        (uint32_t)&BYPASS_cam_render_scene,
        GameInfoTable.cam_render_scene_preamble);
}

void remove_all_hooks() {
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

/*** Function patching ***/

#include <stdio.h>
#include <windows.h>

#define PREFIX "periapt: "

static void fixup_ptr(LPVOID& ptr, DWORD baseAddress) {
    ptr = (void *)((DWORD)ptr + baseAddress);
}

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
            // Grab the game info and fixup pointers
            GameInfoTable = PerIdentityGameTable[identity];

            printf("raw GameInfoTable.cam_render_scene: %08x\n", (unsigned int)GameInfoTable.cam_render_scene);
            printf("raw GameInfoTable.graphics_info_ptr: %08x\n", (unsigned int)GameInfoTable.graphics_info_ptr);
            DWORD baseAddress = (DWORD)GetModuleHandle(NULL);
            fixup_ptr(GameInfoTable.cam_render_scene, baseAddress);
            fixup_ptr(GameInfoTable.graphics_info_ptr, baseAddress);
            printf("fixed GameInfoTable.cam_render_scene: %08x\n", (unsigned int)GameInfoTable.cam_render_scene);
            printf("fixed GameInfoTable.graphics_info_ptr: %08x\n", (unsigned int)GameInfoTable.graphics_info_ptr);
        } else {
            printf(PREFIX "Cannot identify exe; must not continue!\n");
            return false;
        }

        printf("ORIGINAL_cam_render_scene: %08x\n", (uint32_t)(void *)ORIGINAL_cam_render_scene);
        printf("TRAMPOLINE_cam_render_scene: %08x\n", (uint32_t)&TRAMPOLINE_cam_render_scene);
        printf("BYPASS_cam_render_scene: %08x\n", (uint32_t)&BYPASS_cam_render_scene);
        printf("HOOK_cam_render_scene: %08x\n", (uint32_t)&HOOK_cam_render_scene);

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
