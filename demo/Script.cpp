/******************************************************************************
 *    Script.cc
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

#include <cstring>

using namespace std;


cScript::~cScript()
{
}

cScript::cScript(const char* pszName, int iHostObjId)
	    : m_iRef(1), m_szName(pszName), m_iObjId(iHostObjId)
{
}

HRESULT cScript::QueryInterface(REFIID riid, void** ppout)
{
	if (riid == IID_IUnknown)
		*ppout = static_cast<IUnknown*>(this);
	else if (riid == IID_IScript)
		*ppout = static_cast<IScript*>(this);
	else
		return E_NOINTERFACE;
	static_cast<IUnknown*>(*ppout)->AddRef();
	return S_OK;
}

ULONG cScript::AddRef(void)
{
	return ++m_iRef;
}

ULONG cScript::Release(void)
{
	if (m_iRef)
	{
		if (--m_iRef == 0)
			delete this;
	}
	return m_iRef;
}

const char* cScript::GetClassName(void)
{
	// Name MUST match the one in the list.
	// Still, we don't want to toss a NULL pointer around, do we?
	return (m_szName) ? m_szName : "cScript";
}

long cScript::ReceiveMessage(sScrMsg* pMsg, sMultiParm* pReply, eScrTraceAction eTrace)
{
	long iRet = 0;
	if (!stricmp(pMsg->message, "ScriptPtrQuery"))
	{
		iRet = ScriptPtrQuery(static_cast<sPtrQueryMsg*>(pMsg));
	}
	return iRet;
}


long cScript::ScriptPtrQuery(sPtrQueryMsg* pMsg)
{
	// Check class name 
	if (!stricmp(pMsg->pszDestClass, GetClassName()))
	{
		*(pMsg->pScriptReceptacle) = reinterpret_cast<void*>(this);
		return 0;
	}
	return 1;
}


