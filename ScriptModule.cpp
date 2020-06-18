/******************************************************************************
 *    ScriptModule.cc
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

#include "ScriptModule.h"

#include <cstring>

#include <lg/scrmanagers.h>
#include <lg/malloc.h>

IMalloc *g_pMalloc = NULL;
IScriptMan *g_pScriptManager = NULL;
MPrintfProc g_pfnMPrintf = NULL;


cScriptModule::~cScriptModule()
{
	if (m_pszName)
		delete[] m_pszName;
}

cScriptModule::cScriptModule()
	    : m_iRef(1), m_pszName(NULL)
{
}

cScriptModule::cScriptModule(const char* pszName)
	    : m_iRef(1), m_pszName(NULL)
{
	SetName(pszName);
}

HRESULT cScriptModule::QueryInterface(REFIID riid, void** ppout)
{
	if (riid == IID_IUnknown)
		*ppout = static_cast<IUnknown*>(this);
	else if (riid == IID_IScriptModule)
		*ppout = static_cast<IScriptModule*>(this);
	else
		return E_NOINTERFACE;
	reinterpret_cast<IUnknown*>(*ppout)->AddRef();
	return S_OK;
}

ULONG cScriptModule::AddRef(void) 
{
	return ++m_iRef;
}

ULONG cScriptModule::Release(void)
{
	// This object is static, so we don't delete it.
	if (m_iRef)
		m_iRef--;
	return m_iRef;
}

void cScriptModule::SetName(const char* pszName)
{
	if (m_pszName)
		delete[] m_pszName;
	if (pszName)
	{
		m_pszName = new char[strlen(pszName)];
		strcpy(m_pszName, pszName);
	}
	else
		m_pszName = NULL;
}

const char* cScriptModule::GetName(void)
{
	return m_pszName;
}

const sScrClassDesc* cScriptModule::GetScript(unsigned int i)
{
	if (i < sm_ScriptsArraySize)
		return &sm_ScriptsArray[i];
	else
		return NULL;
}

const sScrClassDesc* cScriptModule::GetFirstClass(tScrIter* pIterParam)
{
	*reinterpret_cast<unsigned int*>(pIterParam) = 0;
	return GetScript(0);
}

const sScrClassDesc* cScriptModule::GetNextClass(tScrIter* pIterParam) 
{
	const sScrClassDesc *pRet;
	register unsigned int index = *reinterpret_cast<unsigned int*>(pIterParam);
	pRet = GetScript(++index);
	*reinterpret_cast<unsigned int*>(pIterParam) = index;
	return pRet;
}

void cScriptModule::EndClassIter(tScrIter*)
{
	// Nothing to do here
}


cScriptModule  g_ScriptModule;

extern "C" __declspec(dllexport) __stdcall 
int ScriptModuleInit (const char* pszName, 
                      IScriptMan* pScriptMan,
                      MPrintfProc pfnMPrintf,
                      IMalloc* pMalloc,
                      IScriptModule** pOutInterface)
{
	*pOutInterface = NULL;

	g_pScriptManager = pScriptMan;
#ifdef _DEBUG
	pMalloc->QueryInterface(IID_IDebugMalloc, reinterpret_cast<void**>(&g_pMalloc));
	if (!g_pMalloc)
		g_pMalloc = pMalloc;
#else
	g_pMalloc = pMalloc;
#endif

	g_pfnMPrintf = reinterpret_cast<MPrintfProc>(pfnMPrintf);

	if (!g_pScriptManager || !g_pMalloc)
		return 0;

	g_ScriptModule.SetName(pszName);
	g_ScriptModule.QueryInterface(IID_IScriptModule, reinterpret_cast<void**>(pOutInterface));

	return 1;
}

