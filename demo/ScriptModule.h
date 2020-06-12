/******************************************************************************
 *    ScriptModule.h
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

#ifdef __BORLANDC__
#include <rpc.h>
#endif
#include <objbase.h>

#include <lg/script.h>
#include <lg/scrmanagers.h>

extern IMalloc *g_pMalloc;
extern IScriptMan *g_pScriptManager;

typedef int (__cdecl *MPrintfProc)(const char*, ...);
extern MPrintfProc g_pfnMPrintf;

class cScriptModule : public IScriptModule
{
public:
	// IUnknown
	STDMETHOD(QueryInterface)(REFIID,void**);
	STDMETHOD_(ULONG,AddRef)(void);
	STDMETHOD_(ULONG,Release)(void);
	// IScriptModule
	STDMETHOD_(const char*,GetName)(void);
	STDMETHOD_(const sScrClassDesc*,GetFirstClass)(tScrIter*);
	STDMETHOD_(const sScrClassDesc*,GetNextClass)(tScrIter*);
	STDMETHOD_(void,EndClassIter)(tScrIter*);

private:
	int m_iRef;

public:
	virtual ~cScriptModule();
	cScriptModule();
	cScriptModule(const char* pszName);

	void SetName(const char* pszName);

private:
	const sScrClassDesc* GetScript(unsigned int i);
	
	char* m_pszName;

	static const sScrClassDesc sm_ScriptsArray[];
	static const unsigned int sm_ScriptsArraySize;
};
