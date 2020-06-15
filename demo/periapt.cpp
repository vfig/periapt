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

#include "t2types.h"

using namespace std;

// FIXME: temp
void readMem(void *addr, DWORD lenxx)
{
    HANDLE hProcess = GetCurrentProcess();
    // HMODULE hModule = GetModuleHandle(NULL);
    // TODO: we probably can't see printfs, right?
    // if (g_pfnMPrintf) g_pfnMPrintf(...);

    int len = 32; // ignore whatever we were told.
    UCHAR bytes[32];

    SIZE_T bytesRead = 0;
    BOOL ok = ReadProcessMemory(hProcess, (LPCVOID)(addr), bytes, len, &bytesRead);
    if (ok) {
        printf("Bytes at %08x:\n", (unsigned int)addr);
        for (int i=0; i<len; ++i) {
            if (i%16 == 0) printf(" ");
            printf(" %02X", bytes[i]);
            if (i%16 == 15) printf("\n");
        }
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

/*** Function info ***/

enum ExeFunction {
    ExeFunction_cam_render_scene = 0,

    ExeFunctionCount,
};
struct ExeFunctionInfo {
    DWORD offset;
    DWORD prologueSize;
};
static const ExeFunctionInfo PerIdentityExeFunctionInfoTable[ExeIdentityCount][ExeFunctionCount] = {
    // ExeThief_v126
    {
        { 0x001bc7a0UL, 9 }, // ExeFunction_cam_render_scene
    },
    // ExeDromEd_v126
    {
        { 0x00286960UL, 9 }, // ExeFunction_cam_render_scene
    },
    // ExeThief_v127
    {
        { 0x001bd820UL, 9 }, // ExeFunction_cam_render_scene
    },
    // ExeDromEd_v127
    {
        { 0x002895c0UL, 9 }, // ExeFunction_cam_render_scene
    },
};
static const ExeFunctionInfo *ExeFunctionInfoTable;

/*** Hooks and crooks ***/

extern "C" {
// These are defined in bypass.s:
extern uint8_t bypass_enable;
extern void __cdecl CALL_cam_render_scene(void* pos, double zoom);
extern const void *BYPS_cam_render_scene;
extern const void *TRAM_cam_render_scene;
}

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

// FIXME: temp, just so I can print the address in install_hook/remove_hook below
extern "C" {
extern void __cdecl CALL_cam_render_scene(void* pos, double zoom);
}

void install_hook(bool *hooked, uint32_t target, uint32_t bypass, uint32_t trampoline, uint32_t size) {
    if (! *hooked) {
        *hooked = true;
printf("hooking target %08x, bypass %08x, tramp %08x, size %u\n", target, bypass, trampoline, size);
        DWORD targetProtection, bypassProtection;
        VirtualProtect((void *)target, size, PAGE_EXECUTE_READWRITE, &targetProtection);
        VirtualProtect((void *)bypass, size+5, PAGE_EXECUTE_READWRITE, &bypassProtection);

readMem((void *)target, 32);
readMem((void *)CALL_cam_render_scene, 32);
readMem((void *)bypass, 32);
readMem((void *)trampoline, 32);

printf("memcpy %08x, %08x, %u\n", bypass, target, size);
        memcpy((void *)bypass, (void *)target, size);
printf("patch_jmp %08x, %08x\n", bypass+size, target+size);
        patch_jmp(bypass+size, target+size);
printf("patch_jmp %08x, %08x\n", target, trampoline);
        patch_jmp(target, trampoline);
        VirtualProtect((void *)target, size, targetProtection, NULL);
        VirtualProtect((void *)bypass, size+5, bypassProtection, NULL);

readMem((void *)target, 32);
readMem((void *)CALL_cam_render_scene, 32);
readMem((void *)bypass, 32);
readMem((void *)trampoline, 32);

printf("hook complete\n");
    }
}

void remove_hook(bool *hooked, uint32_t target, uint32_t bypass, uint32_t trampoline, uint32_t size) {
    (void)trampoline; // Unused
    if (*hooked) {
        *hooked = false;

printf("unhooking target %08x, bypass %08x, tramp %08x, size %u\n", target, bypass, trampoline, size);
        DWORD targetProtection, bypassProtection;
        VirtualProtect((void *)target, size, PAGE_EXECUTE_READWRITE, &targetProtection);
        // VirtualProtect((void *)bypass, size+5, PAGE_EXECUTE_READWRITE, &bypassProtection);

readMem((void *)target, 32);
readMem((void *)CALL_cam_render_scene, 32);
readMem((void *)bypass, 32);
readMem((void *)trampoline, 32);

printf("memcpy %08x, %08x, %u\n", target, bypass, size);
        memcpy((void *)target, (void *)bypass, size);

readMem((void *)target, 32);
readMem((void *)CALL_cam_render_scene, 32);
readMem((void *)bypass, 32);
readMem((void *)trampoline, 32);

        VirtualProtect((void *)target, size, targetProtection, NULL);
        // VirtualProtect((void *)bypass, size+5, bypassProtection, NULL);

printf("unhook complete\n");
    }
}

// TODO: I don't think we want to hook and unhook many parts individually,
// so change this to use a single flag for if all hooks are installed or not.
bool hooked_cam_render_scene;
extern "C" void __cdecl HOOK_cam_render_scene(t2position* pos, double zoom) {
    // reverse the camera Z for a laugh:
    t2position newpos = *pos;
    newpos.fac.z = ~newpos.fac.z;
    double newzoom = 2.0 * zoom;
    CALL_cam_render_scene(&newpos, newzoom);

    // so, what breaks if we call it twice, huh?
    // nothing immediate!
    CALL_cam_render_scene(pos, zoom);
}

void install_all_hooks() {
    const ExeFunctionInfo *info = &ExeFunctionInfoTable[ExeFunction_cam_render_scene];
    DWORD baseAddress = (DWORD)GetModuleHandle(NULL);
    install_hook(&hooked_cam_render_scene, baseAddress+info->offset,
        (uint32_t)&BYPS_cam_render_scene, (uint32_t)&TRAM_cam_render_scene, info->prologueSize);
}

void remove_all_hooks() {
    const ExeFunctionInfo *info = &ExeFunctionInfoTable[ExeFunction_cam_render_scene];
    DWORD baseAddress = (DWORD)GetModuleHandle(NULL);
    remove_hook(&hooked_cam_render_scene, baseAddress+info->offset,
        (uint32_t)&BYPS_cam_render_scene, (uint32_t)&TRAM_cam_render_scene, info->prologueSize);
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
            // Assign things that depend on identity:
            ExeFunctionInfoTable = PerIdentityExeFunctionInfoTable[identity];
        } else {
            printf(PREFIX "Cannot identify exe; must not continue!\n");
            return false;
        }

        printf("CALL_cam_render_scene: %08x\n", (uint32_t)(void *)CALL_cam_render_scene);
        printf("BYPS_cam_render_scene: %08x\n", (uint32_t)&BYPS_cam_render_scene);
        printf("TRAM_cam_render_scene: %08x\n", (uint32_t)&TRAM_cam_render_scene);

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
