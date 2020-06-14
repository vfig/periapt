/************************************************
 * LGS Auto-Release wrapper for Script interface
 */

#ifndef _LG_INTERFACE_H
#define _LG_INTERFACE_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <exception>
#include <cstdio>

#include <lg/iiddef.h>
#include <lg/scrmanagers.h>
//#include <lg/scrservices.h>

#ifdef __GNUC__
#pragma interface "lg/interface.h"
#endif

class null_pointer : public std::exception
{
public:
	null_pointer() throw() { }
	virtual ~null_pointer() throw() { }

	virtual const char* what() const throw()
	{ return "null pointer"; }
};

class no_interface : public std::exception
{
	const char* ifname;
	char* buf;
public:
	no_interface() throw() : ifname(NULL), buf(NULL) { }
	no_interface(const char* nm) : ifname(nm), buf(NULL) { }
	virtual ~no_interface() throw()
	{
		if (buf)
			delete[] buf;
	}

	virtual const char* what() throw()
	{
		buf = new char[strlen(ifname) + 27];
		sprintf(buf, "interface not available (%s)", ifname);
		return buf;
	}
};

/**************************************
 * Helper class for interface pointers.
 */
template<class _IFace, typename _IID = IID_Def<_IFace> >
class SInterface
{
	_IFace* m_pIFace;

public:
	~SInterface() throw()
	{
		if (m_pIFace)
			m_pIFace->Release();
	}
	explicit SInterface(_IFace* __p = NULL) throw()
		: m_pIFace(__p)
	{
		if (m_pIFace)
			m_pIFace->AddRef();
	}
	SInterface(SInterface& __x) throw()
		: m_pIFace(__x.m_pIFace)
	{
		if (m_pIFace)
			m_pIFace->AddRef();
	}
	template<class _ExtIFace>
	SInterface(SInterface<_ExtIFace>& __x) throw()
	{
		m_pIFace = static_cast<_IFace*>(__x.m_pIFace);
		if (m_pIFace)
			m_pIFace->AddRef();
	}
	SInterface(IScriptMan* pSM) throw(/*no_interface*/)
		: m_pIFace(NULL)
	{
		if (E_NOINTERFACE == pSM->QueryInterface(_IID::iid(), reinterpret_cast<void**>(&m_pIFace)))
			throw no_interface(_IID::name());
	}

	SInterface& operator=(SInterface& __x) throw()
	{
		m_pIFace = __x.m_pIFace;
		if (m_pIFace)
			m_pIFace->AddRef();
		return *this;
	}
	SInterface& operator=(_IFace* __p) throw()
	{
		m_pIFace = __p;
		return *this;
	}

	operator bool () const throw()
	{
		return m_pIFace != NULL;
	}

	operator _IFace* () const throw()
	{
		return m_pIFace;
	}

	_IFace* get() const throw()
	{
		return m_pIFace;
	}
	_IFace& operator*() const throw()
	{
		//if (!m_pIFace)
		//	throw null_pointer();
		return *m_pIFace;
	}
	_IFace* operator->() const throw()
	{
		//if (!m_pIFace)
		//	throw null_pointer();
		return m_pIFace;
	}
};

/******************************************************
 * Helper class for script service interface pointers.
 */
template<class _IFace, typename _IID = IID_Def<_IFace> >
class SService
{
	_IFace* m_pIFace;

public:
	~SService() throw()
	{
		if (m_pIFace)
			m_pIFace->Release();
	}
	explicit SService(_IFace* __p = NULL) throw()
		: m_pIFace(__p)
	{
		if (m_pIFace)
			m_pIFace->AddRef();
	}
	SService(SService& __x) throw()
		: m_pIFace(__x.m_pIFace)
	{
		if (m_pIFace)
			m_pIFace->AddRef();
	}
	template<class _ExtIFace>
	SService(SService<_ExtIFace>& __x) throw()
	{
		m_pIFace = static_cast<_IFace*>(__x.m_pIFace);
		if (m_pIFace)
			m_pIFace->AddRef();
	}
	SService(IScriptMan* pSM) throw(/*no_interface*/)
	{
		m_pIFace = static_cast<_IFace*>(pSM->GetService(_IID::iid()));
		if (!m_pIFace)
			throw no_interface(_IID::name());
	}

	SService& operator=(SService& __x) throw()
	{
		m_pIFace = __x.m_pIFace;
		if (m_pIFace)
			m_pIFace->AddRef();
		return *this;
	}
	SService& operator=(_IFace* __p) throw()
	{
		m_pIFace = __p;
		return *this;
	}

	operator bool () const throw()
	{
		return m_pIFace != NULL;
	}

	operator _IFace* () const throw()
	{
		return m_pIFace;
	}

	_IFace* get() const throw()
	{
		return m_pIFace;
	}
	_IFace& operator*() const throw()
	{
		//if (!m_pIFace)
		//	throw null_pointer();
		return *m_pIFace;
	}
	_IFace* operator->() const throw()
	{
		//if (!m_pIFace)
		//	throw null_pointer();
		return m_pIFace;
	}
};

#endif // _LG_INTERFACE_H
