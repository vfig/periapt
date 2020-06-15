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


/*** Script class declarations (this will usually be in a header file) ***/

class cScr_Echo : public cScript
{
public:
	virtual ~cScr_Echo() { }
	cScr_Echo(const char* pszName, int iHostObjId)
		: cScript(pszName,iHostObjId)
	{ }

	STDMETHOD_(long,ReceiveMessage)(sScrMsg*,sMultiParm*,eScrTraceAction);

public:
	static IScript* __cdecl ScriptFactory(const char* pszName, int iHostObjId);
};

/*** Script implementations ***/

long cScr_Echo::ReceiveMessage(sScrMsg* pMsg, sMultiParm* pReply, eScrTraceAction eTrace)
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

IScript* cScr_Echo::ScriptFactory(const char* pszName, int iHostObjId)
{
	if (stricmp(pszName,"Echo") != 0)
		return NULL;

	// Use a static string, so I don't have to make a copy.
	cScr_Echo* pscrRet = new(nothrow) cScr_Echo("Echo", iHostObjId);
	return static_cast<IScript*>(pscrRet);
}

const sScrClassDesc cScriptModule::sm_ScriptsArray[] = {
	{ "echo", "Echo", "CustomScript", cScr_Echo::ScriptFactory },
};
const unsigned int cScriptModule::sm_ScriptsArraySize = sizeof(sm_ScriptsArray)/sizeof(sm_ScriptsArray[0]);
