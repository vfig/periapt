/*************
 * LGS Links
 */

#ifndef _LG_LINKS_H
#define _LG_LINKS_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <lg/objstd.h>
#include <lg/iiddef.h>

#include <lg/types.h>
#include <lg/defs.h>

#ifdef __GNUC__
#pragma interface "lg/links.h"
#endif

extern  const GUID  IID_ILinkQuery;
interface ILinkQuery : IUnknown
{
	STDMETHOD_(int,Done)(void) const PURE;
	STDMETHOD_(long,Link)(sLink *) const PURE;
	STDMETHOD_(long,ID)(void) const PURE;
	// The pointer should not be freed.
	STDMETHOD_(void*,Data)(void) const PURE;
	STDMETHOD_(long,Next)(void) PURE;
	STDMETHOD_(ILinkQuery*,Inverse)(void) PURE;
};
DEFINE_IIDSTRUCT(ILinkQuery,IID_ILinkQuery)

struct sRelationListenMsg
{
	int event;
	long lLink;
	int object;
};
typedef void (__stdcall *RelListenFunc)(sRelationListenMsg*,void*);

struct sRelationDesc
{
	char szName[45];
};

struct sRelationDataDesc
{
	char szTypeName[32];
	unsigned int uiTypeSize;
};

extern  const GUID  IID_IRelation;
interface IRelation : IUnknown
{
	// This function returns information about the relation
	STDMETHOD_(const sRelationDesc*,Describe)(void) const PURE;
	// The functions below get (and set?) the ID of the relation
	STDMETHOD_(short,GetID)(void) const PURE;
	STDMETHOD_(long,SetID)(short) PURE;
	// This function returns the data of the relation
	STDMETHOD_(const sRelationDataDesc*,DescribeData)(void) const PURE;
	// This function returns the corresponding inverse relation (~Name)
	STDMETHOD_(IRelation*,Inverse)(void) PURE;
	// Add relations?
	STDMETHOD_(long,Add)(int,int) PURE;
	// The contents of the data pointer are copied. (Be sure the memory it points to is large enough to be read.)
	// Wasn't originally const, I changed it to allow the use of string literals. 
	STDMETHOD_(long,AddFull)(int,int,const void*) PURE;
	STDMETHOD_(long,Remove)(long) PURE;
	// The function below retrieves link information, given the link's (not the relation's!) ID.
	STDMETHOD_(int,Get)(long,sLink *) const PURE;
	STDMETHOD_(long,SetData)(long,void *) PURE;
	// The pointer should not be freed
	STDMETHOD_(void*,GetData)(long) PURE;
	STDMETHOD_(ILinkQuery*,Query)(int,int) const PURE;
	STDMETHOD_(long,Notify)(unsigned long,void *) PURE;	
	STDMETHOD_(long,Listen)(unsigned long,RelListenFunc,void*) PURE;
	STDMETHOD_(long,GetSingleLink)(int,int) PURE;
	STDMETHOD_(int,AnyLinks)(int,int) PURE;
};
DEFINE_IIDSTRUCT(IRelation,IID_IRelation)


///////////
// linkset
///////////
inline linkset::~linkset()
{
	if (query) query->Release();
}
inline link* linkset::Link(link& __l) const
{
	if (!query) return NULL;
	__l = query->ID();
	return &__l;
}
inline void linkset::NextLink()
{
	if (query) query->Next();
}
inline true_bool* linkset::AnyLinksLeft(true_bool& __p) const
{
	__p = (query && !query->Done());
	return &__p;
}


#endif // _LG_LINKS_H
