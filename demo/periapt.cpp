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

using namespace std;

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

    if (stricmp(pMsg->message, "TurnOn") == 0) {
    } else if (stricmp(pMsg->message, "TurnOff") == 0) {
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


/* -------------------------------------------------------- */

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
    DWORD offset;
    DWORD size;
    UCHAR bytes[ExeSignatureBytesMax];
    CHAR fixups[ExeSignatureFixupsMax];
};
typedef void (*FnPtr)(void);
struct ExeFunctionPointers {
    FnPtr cam_render_scene;
};
struct ExeInfo {
    const char *name;
    const char *version;
    ExeSignature signature;
    ExeFunctionPointers functions;
};
static const ExeInfo ExeInfoTable[ExeIdentityCount] = {
    {
        "Thief2", "ND 1.26",
        {   // Signature
            0x001bc7a0UL, 31,
            {
            0x83,0xec,0x28,0xdd,0x05,0xd8,0xde,0x38,
            0x00,0x53,0x56,0xdd,0x54,0x24,0x24,0xdd,
            0x05,0xe0,0xde,0x38,0x00,0x8b,0xf0,0x8b,
            0x06,0xdd,0x54,0x24,0x1c,0xd9,0x05,
            },
            { 5, 17, -1 },
        },
    },
    {
        "DromEd", "ND 1.26",
        {   // Signature
            0x00286960UL, 31,
            {
                0x83,0xec,0x28,0xdd,0x05,0xa8,0xdf,0x4a,
                0x00,0x53,0x56,0xdd,0x54,0x24,0x24,0xdd,
                0x05,0xb0,0xdf,0x4a,0x00,0x8b,0xf0,0x8b,
                0x06,0xdd,0x54,0x24,0x1c,0xd9,0x05,
            },
            { 5, 17, -1 },
        },
    },
    {
        "Thief2", "ND 1.27",
        {   // Signature
            0x001bd820UL, 31,
            {
                0x83,0xec,0x28,0xdd,0x05,0xd8,0xee,0x38,
                0x00,0x53,0x56,0xdd,0x54,0x24,0x24,0xdd,
                0x05,0xe0,0xee,0x38,0x00,0x8b,0xf0,0x8b,
                0x06,0xdd,0x54,0x24,0x1c,0xd9,0x05,
            },
            { 5, 17, -1 },
        },
    },
    {
        "DromEd", "ND 1.27",
        {   // Signature
            0x002895c0UL, 31,
            {
                0x83,0xec,0x28,0xdd,0x05,0xa8,0x1f,0x4b,
                0x00,0x53,0x56,0xdd,0x54,0x24,0x24,0xdd,
                0x05,0xb0,0x1f,0x4b,0x00,0x8b,0xf0,0x8b,
                0x06,0xdd,0x54,0x24,0x1c,0xd9,0x05,
            },
            { 5, 17, -1 },
        },
    },
};

BOOL DoesSignatureMatch(const ExeSignature *sig) {
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
        const ExeSignature *sig = &(ExeInfoTable[i].signature);
        if (DoesSignatureMatch(sig)) {
            return static_cast<ExeIdentity>(i);
        }
    }
    return ExeIdentityUnknown;
}

static void printLastError()
{
    DWORD err = GetLastError();
    char buffer[1024];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, 0, buffer, 1024, NULL);
    printf("(%lu) %s\n", err, buffer);
}

void readMem(DWORD offset, DWORD len)
{
    HANDLE hProcess = GetCurrentProcess();
    HMODULE hModule = GetModuleHandle(NULL);
    // TODO: we probably can't see printfs, right?
    // if (g_pfnMPrintf) g_pfnMPrintf(...);
    printf("Module base address: %08x\n", (DWORD)hModule);
    DWORD baseAddress = (DWORD)hModule;

    len = 32; // ignore whatever we were told.
    UCHAR bytes[32];

    SIZE_T bytesRead = 0;
    BOOL ok = ReadProcessMemory(hProcess, (LPCVOID)(baseAddress+offset), bytes, len, &bytesRead);
    if (ok) {
        printf("Bytes at %08x:\n", offset);
        for (int i=0; i<len; ++i) {
            if (i%16 == 0) printf(" ");
            printf(" %02X", bytes[i]);
            if (i%16 == 15) printf("\n");
        }
        printf("\n");
    } else {
        printf("ReadProcessMemory failed (%lu bytes read)!\n", (unsigned long)bytesRead);
        printLastError();
        return;
    }
}


/* -------------------------------------------------------- */


#include <stdio.h>

#define PREFIX "periapt: "

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    static BOOL didAllocConsole = false;
    switch (reason) {
    case DLL_PROCESS_ATTACH: {
        // Try to identify this exe.
        ExeIdentity identity = IdentifyExe();
        BOOL isEditor, isIdentified;
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
            didAllocConsole = (BOOL)AllocConsole();
        }
        if (shouldRedirectStdout) {
            freopen("CONOUT$", "w", stdout);
        }
        // Now that we (hopefully) have somewhere for output to go,
        // we can print some useful messages.
        printf(PREFIX "DLL_PROCESS_ATTACH\n");
        printf(PREFIX "Base address: 0x%X\n", (unsigned long)GetModuleHandle(NULL));
        // Display the results of identifying the exe.
        if (isIdentified) {
            const ExeInfo *info = &ExeInfoTable[identity];
            printf(PREFIX "Identified exe as %s %s (%s)\n",
                info->name, info->version, (isEditor ? "EDITOR" : "GAME"));
        } else {
            printf(PREFIX "Cannot identify exe; must not continue!\n");
            return false;
        }

        // # 0x286960 - _cam_render_scene in Dromed ND 1.26
        // # 0x1bc7a0 - _cam_render_scene in Thief2 ND 1.26
        // # 0x1bd820 - _cam_render_scene in Thief2 ND 1.27
        readMem(0x286960, 0x20);
    } break;
    case DLL_PROCESS_DETACH: {
        printf(PREFIX "DLL_PROCESS_DETACH\n");
        if (didAllocConsole) {
            FreeConsole();
        }
    } break;
    }
    return true;
}
