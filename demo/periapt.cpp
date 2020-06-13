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

class cScr_Readmem : public cScript
{
public:
	virtual ~cScr_Readmem() { }
	cScr_Readmem(const char* pszName, int iHostObjId)
		: cScript(pszName,iHostObjId)
	{ }

	STDMETHOD_(long,ReceiveMessage)(sScrMsg*,sMultiParm*,eScrTraceAction);

public:
	static IScript* __cdecl ScriptFactory(const char* pszName, int iHostObjId);
};

/*** Script implementations ***/

long cScr_Readmem::ReceiveMessage(sScrMsg* pMsg, sMultiParm* pReply, eScrTraceAction eTrace)
{
	long iRet = cScript::ReceiveMessage(pMsg, pReply, eTrace);

	try
	{
		string sOutput = pMsg->message;
		sOutput += "(";
		sOutput += static_cast<const char*>(pMsg->data);
		sOutput += ",";
		sOutput += static_cast<const char*>(pMsg->data2);
		sOutput += ",";
		sOutput += static_cast<const char*>(pMsg->data2);
		sOutput += ")";

		int iTime = -1001;
		IPropertySrv* pProps = static_cast<IPropertySrv*>(g_pScriptManager->GetService(IID_IPropertyScriptService));
		if (pProps)
		{
			if (pProps->Possessed(m_iObjId, "ScriptTiming"))
			{
				cMultiParm mpTiming;
				pProps->Get(mpTiming, m_iObjId, "ScriptTiming", NULL);
				if (static_cast<int>(mpTiming) > 0)
				{
					iTime = mpTiming;
				}
			}
			pProps->Release();
		}

		IDarkUISrv* pUI = static_cast<IDarkUISrv*>(g_pScriptManager->GetService(IID_IDarkUIScriptService));
		if (pUI)
		{
			pUI->TextMessage(sOutput.c_str(), 0, iTime);
			pUI->Release();
		}
	}
	catch (exception& err)
	{
		// Don't pass exceptions out of the module.
		if (g_pfnMPrintf)
			g_pfnMPrintf("Error! %s\n", err.what());
	}

	return iRet;
}

/*** Script Factories ***/

IScript* cScr_Readmem::ScriptFactory(const char* pszName, int iHostObjId)
{
	if (stricmp(pszName,"Readmem") != 0)
		return NULL;

	// Use a static string, so I don't have to make a copy.
	cScr_Readmem* pscrRet = new(nothrow) cScr_Readmem("Readmem", iHostObjId);
	return static_cast<IScript*>(pscrRet);
}

const sScrClassDesc cScriptModule::sm_ScriptsArray[] = {
	{ "periapt", "Readmem", "CustomScript", cScr_Readmem::ScriptFactory },
};
const unsigned int cScriptModule::sm_ScriptsArraySize = sizeof(sm_ScriptsArray)/sizeof(sm_ScriptsArray[0]);


/* -------------------------------------------------------- */


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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
    	// Since dromed already has a console, we don't want to allocate one,
    	// not unless we're running in the game itself, I guess?
        // AllocConsole();
    	// But I guess we still redirect stdout so printf can be seen in
    	// dromed's monolog.
        freopen("CONOUT$", "w", stdout);
        printf("DLL_PROCESS_ATTACH\n");
        // If you need to know the base address of the process your injected:   
        printf("base address: 0x%X\n", (DWORD)GetModuleHandle(NULL));

		// # 0x286960 - _cam_render_scene in Dromed ND 1.26
		// # 0x1bc7a0 - _cam_render_scene in Thief2 ND 1.26
		// # 0x1bd820 - _cam_render_scene in Thief2 ND 1.27
		readMem(0x286960, 0x20);

        break;
    case DLL_PROCESS_DETACH:
        printf("DLL_PROCESS_DETACH\n");
        // FreeConsole();
        break;
    }
    return true;
}
