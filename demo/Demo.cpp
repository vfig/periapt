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

class cScr_Parrot : public cScript
{
public:
	virtual ~cScr_Parrot() { }
	cScr_Parrot(const char* pszName, int iHostObjId)
		: cScript(pszName,iHostObjId)
	{ }

	STDMETHOD_(long,ReceiveMessage)(sScrMsg*,sMultiParm*,eScrTraceAction);

public:
	static IScript* __cdecl ScriptFactory(const char* pszName, int iHostObjId);
};

class cScr_SlowRelay : public cScript
{
public:
	virtual ~cScr_SlowRelay() { }
	cScr_SlowRelay(const char* pszName, int iHostObjId)
		: cScript(pszName,iHostObjId)
	{ }

	STDMETHOD_(long,ReceiveMessage)(sScrMsg*,sMultiParm*,eScrTraceAction);

public:
	static IScript* __cdecl ScriptFactory(const char* pszName, int iHostObjId);

private:
	void SetTimer(void);
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

long cScr_Parrot::ReceiveMessage(sScrMsg* pMsg, sMultiParm* pReply, eScrTraceAction eTrace)
{
	long iRet = cScript::ReceiveMessage(pMsg, pReply, eTrace);

	if (pMsg->from != 0)
	{
		g_pScriptManager->PostMessage2(m_iObjId, pMsg->from, pMsg->message, pMsg->data, pMsg->data2, pMsg->data3);
	}

	return iRet;
}

void cScr_SlowRelay::SetTimer(void)
{
	unsigned long iTime = 1000;
	IPropertySrv* pProps = static_cast<IPropertySrv*>(g_pScriptManager->GetService(IID_IPropertyScriptService));
	if (pProps)
	{
		if (pProps->Possessed(m_iObjId, "ScriptTiming"))
		{
			cMultiParm mpTime;
			pProps->Get(mpTime, m_iObjId, "ScriptTiming", NULL);
			iTime = (static_cast<int>(mpTime) > 100) ? static_cast<int>(mpTime) : 100;
		}
		pProps->Release();
	}
	
	tScrTimer hDelay = g_pScriptManager->SetTimedMessage2(m_iObjId, "Delay", iTime, kSTM_OneShot, cMultiParm());
	cMultiParm mpDelay = reinterpret_cast<int>(hDelay);
	sScrDatumTag dataInfo;
	dataInfo.objId = m_iObjId;
	dataInfo.pszClass = m_szName;
	dataInfo.pszName = "Timer";
	g_pScriptManager->SetScriptData(&dataInfo, &mpDelay);
}

long cScr_SlowRelay::ReceiveMessage(sScrMsg* pMsg, sMultiParm* pReply, eScrTraceAction eTrace)
{
	long iRet = 0;
	cMultiParm mpData;
	sScrDatumTag dataInfo;
	dataInfo.objId = m_iObjId;
	dataInfo.pszClass = m_szName;

	if (!stricmp(pMsg->message, "Timer"))
	{
		if (!stricmp(static_cast<sScrTimerMsg*>(pMsg)->name, "Delay"))
		{
			ILinkSrv* pLS = static_cast<ILinkSrv*>(g_pScriptManager->GetService(IID_ILinkScriptService));
			ILinkToolsSrv* pLTS = static_cast<ILinkToolsSrv*>(g_pScriptManager->GetService(IID_ILinkToolsScriptService));
			if (pLS && pLTS)
			{
				pLS->BroadcastOnAllLinks(m_iObjId, "TurnOn", pLTS->LinkKindNamed("ControlDevice"));
				pLTS->Release();
				pLS->Release();
			}

			dataInfo.pszName = "Timer";
			g_pScriptManager->ClearScriptData(&dataInfo, &mpData);

			dataInfo.pszName = "Count";
			if (g_pScriptManager->IsScriptDataSet(&dataInfo))
			{
				g_pScriptManager->GetScriptData(&dataInfo, &mpData);
				int iCount = static_cast<int>(mpData) - 1;
				if (iCount > 0)
				{
					mpData = iCount;
					g_pScriptManager->SetScriptData(&dataInfo, &mpData);
					SetTimer();
				}
				else
				{
					g_pScriptManager->ClearScriptData(&dataInfo, &mpData);
				}
			}
		}
	}
	else if (!stricmp(pMsg->message, "TurnOn"))
	{
		int iCount = 0;
		dataInfo.pszName = "Count";
		if (g_pScriptManager->IsScriptDataSet(&dataInfo))
		{
			g_pScriptManager->GetScriptData(&dataInfo, &mpData);
			iCount = static_cast<int>(mpData);
		}
		mpData = iCount + 1;
		g_pScriptManager->SetScriptData(&dataInfo, &mpData);

		dataInfo.pszName = "Timer";
		if (!g_pScriptManager->IsScriptDataSet(&dataInfo))
			SetTimer();
	}
	else if (!stricmp(pMsg->message, "TurnOff"))
	{
		dataInfo.pszName = "Count";
		if (g_pScriptManager->IsScriptDataSet(&dataInfo))
		{
			g_pScriptManager->GetScriptData(&dataInfo, &mpData);
			int iCount = static_cast<int>(mpData) - 1;
			if (iCount > 0)
			{
				mpData = iCount;
				g_pScriptManager->SetScriptData(&dataInfo, &mpData);
			}
			else
			{
				g_pScriptManager->ClearScriptData(&dataInfo, &mpData);
				dataInfo.pszName = "Timer";
				if (g_pScriptManager->IsScriptDataSet(&dataInfo))
				{
					g_pScriptManager->GetScriptData(&dataInfo, &mpData);
					tScrTimer hDelay = reinterpret_cast<tScrTimer>(static_cast<int>(mpData));
					g_pScriptManager->KillTimedMessage(hDelay);
					g_pScriptManager->ClearScriptData(&dataInfo, &mpData);
				}
			}
		}
	}
	else
		iRet = cScript::ReceiveMessage(pMsg, pReply, eTrace);

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

IScript* cScr_Parrot::ScriptFactory(const char* pszName, int iHostObjId)
{
	if (stricmp(pszName,"Parrot") != 0)
		return NULL;

	// Use a static string, so I don't have to make a copy.
	cScr_Parrot* pscrRet = new(nothrow) cScr_Parrot("Parrot", iHostObjId);
	return static_cast<IScript*>(pscrRet);
}

IScript* cScr_SlowRelay::ScriptFactory(const char* pszName, int iHostObjId)
{
	if (stricmp(pszName,"SlowRelay") != 0)
		return NULL;

	// Use a static string, so I don't have to make a copy.
	cScr_SlowRelay* pscrRet = new(nothrow) cScr_SlowRelay("SlowRelay", iHostObjId);
	return static_cast<IScript*>(pscrRet);
}

const sScrClassDesc cScriptModule::sm_ScriptsArray[] = {
	{ "Demo", "Echo", "CustomScript", cScr_Echo::ScriptFactory },
	{ "Demo", "Parrot", "CustomScript", cScr_Parrot::ScriptFactory },
	{ "Demo", "SlowRelay", "CustomScript", cScr_SlowRelay::ScriptFactory }
};
const unsigned int cScriptModule::sm_ScriptsArraySize = sizeof(sm_ScriptsArray)/sizeof(sm_ScriptsArray[0]);
