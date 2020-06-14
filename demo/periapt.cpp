/******************************************************************************
 *    Demo.cc
 *
 *    This file is part of Object Script Module
 *    Copyright (C) 2004 Tom N Harris <telliamed@whoopdedo.cjb.net>
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
#include "Script.h"
#include "ScriptModule.h"

#include <lg/scrservices.h>

#include <cstring>
#include <new>
#include <exception>
#include <string>
#include <strings.h>

using namespace std;

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
struct ExeSignature {
    const char *name;
    const char *version;
    DWORD offset;
    DWORD size;
    UCHAR bytes[ExeSignatureBytesMax];
    CHAR fixups[ExeSignatureFixupsMax];
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
        { 5, 17, -1 },
    },
    {
        "DromEd", "ND 1.26", 0x00286960UL, 31,
        {
            0x83,0xec,0x28,0xdd,0x05,0xa8,0xdf,0x4a,
            0x00,0x53,0x56,0xdd,0x54,0x24,0x24,0xdd,
            0x05,0xb0,0xdf,0x4a,0x00,0x8b,0xf0,0x8b,
            0x06,0xdd,0x54,0x24,0x1c,0xd9,0x05,
        },
        { 5, 17, -1 },
    },
    {
        "Thief2", "ND 1.27", 0x001bd820UL, 31,
        {
            0x83,0xec,0x28,0xdd,0x05,0xd8,0xee,0x38,
            0x00,0x53,0x56,0xdd,0x54,0x24,0x24,0xdd,
            0x05,0xe0,0xee,0x38,0x00,0x8b,0xf0,0x8b,
            0x06,0xdd,0x54,0x24,0x1c,0xd9,0x05,
        },
        { 5, 17, -1 },
    },
    {
        "DromEd", "ND 1.27", 0x002895c0UL, 31,
        {
            0x83,0xec,0x28,0xdd,0x05,0xa8,0x1f,0x4b,
            0x00,0x53,0x56,0xdd,0x54,0x24,0x24,0xdd,
            0x05,0xb0,0x1f,0x4b,0x00,0x8b,0xf0,0x8b,
            0x06,0xdd,0x54,0x24,0x1c,0xd9,0x05,
        },
        { 5, 17, -1 },
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
        CHAR o = sig->fixups[i];
        if (o < 0) break;
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

/*** Function patching ***/

void ReadFunctionMemory(void *address, BYTE* buffer, DWORD size) {
    DWORD originalProtection;
    VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &originalProtection);
    ReadProcessMemory(GetCurrentProcess(), (LPVOID)address, buffer, size, NULL);
    VirtualProtect(address, size, originalProtection, NULL);
}

void WriteFunctionMemory(void* address, BYTE* buffer, DWORD size) {
    DWORD originalProtection;
    VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &originalProtection);
    WriteProcessMemory(GetCurrentProcess(), (LPVOID)address, buffer, size, NULL);
    VirtualProtect(address, size, originalProtection, NULL);
}

#define TRAMPOLINE_SIZE 16
struct Trampoline {
    bool active;
    int length;
    BYTE bytes[TRAMPOLINE_SIZE];
};
static Trampoline TrampolineTable[ExeFunctionCount] = {};

void ActivateTrampoline(ExeFunction fn) {
    DWORD baseAddress = (DWORD)GetModuleHandle(NULL);
    const ExeFunctionInfo *info = &ExeFunctionInfoTable[fn];
    Trampoline *trampoline = &TrampolineTable[fn];
#ifndef NDEBUG
    // Make sure the trampoline can hold the prologue!
    // FIXME: will need space for an absolute jump! But for now we're not
    //        doing a full trampoline, just copying bytes in and out.
    if (info->prologueSize < TRAMPOLINE_SIZE) {
    }
#endif
    if (! trampoline->active) {
        DWORD addr = baseAddress + info->offset;
        trampoline->length = info->prologueSize;
        ReadFunctionMemory((LPVOID)addr, trampoline->bytes, trampoline->length);
        BYTE hack[] = { 0xc3 };
        WriteFunctionMemory((LPVOID)addr, hack, 1);
        trampoline->active = true;
    }
}

void DeactivateTrampoline(ExeFunction fn) {
    DWORD baseAddress = (DWORD)GetModuleHandle(NULL);
    const ExeFunctionInfo *info = &ExeFunctionInfoTable[fn];
    Trampoline *trampoline = &TrampolineTable[fn];
#ifndef NDEBUG
    // Make sure the trampoline can hold the prologue!
    // FIXME: will need space for an absolute jump! But for now we're not
    //        doing a full trampoline, just copying bytes in and out.
    if (info->prologueSize < TRAMPOLINE_SIZE) {
    }
#endif
    if (trampoline->active) {
        DWORD addr = baseAddress + info->offset;
        WriteFunctionMemory((LPVOID)addr, trampoline->bytes, trampoline->length);
        trampoline->active = false;
    }
}

void DeactivateAllTrampolines() {
    for (int i=0; i<ExeFunctionCount; ++i) {
        DeactivateTrampoline(static_cast<ExeFunction>(i));
    }
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

    if (_stricmp(pMsg->message, "TurnOn") == 0) {
        ActivateTrampoline(ExeFunction_cam_render_scene);
    } else if (_stricmp(pMsg->message, "TurnOff") == 0) {
        DeactivateTrampoline(ExeFunction_cam_render_scene);
    }

    return iRet;
}

/*** Script Factories ***/

IScript* cScr_PeriaptControl::ScriptFactory(const char* pszName, int iHostObjId)
{
    if (_stricmp(pszName,"PeriaptControl") != 0)
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
        printf(PREFIX "Base address: 0x%X\n", (unsigned long)GetModuleHandle(NULL));
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
    } break;
    case DLL_PROCESS_DETACH: {
        printf(PREFIX "DLL_PROCESS_DETACH\n");
        DeactivateAllTrampolines();
        if (didAllocConsole) {
            FreeConsole();
        }
    } break;
    }
    return true;
}
