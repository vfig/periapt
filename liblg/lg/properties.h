/**************************
 * LGS Property Interfaces
 */

#ifndef _LG_PROPERTIES_H
#define _LG_PROPERTIES_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <lg/objstd.h>
#include <lg/types.h>


struct sPropertyDesc
{
	char	szName[32];
	char*	pszParentMenu;
	char*	pszMenuLabel;
};

struct sPropertyTypeDesc
{
	char	szTypeName[32];
	int		iTypeSize;
};

struct sPropertyObjIter
{
	void* _a;
	unsigned long _b;
	void* _c;	// cObjPropDatumHashTable
};

struct sPropertyListenMsg
{
	long havent_a_clue;
};

DECLARE_HANDLE(PropListenerHandle);
DECLARE_HANDLE(PropListenerData);

typedef void (__stdcall *PropListenFunc)(sPropertyListenMsg*,PropListenerData);

extern  const GUID  IID_IProperty;
interface IProperty : IUnknown
{
	STDMETHOD_(const sPropertyDesc*,Describe)(void) const PURE;
	STDMETHOD_(int,GetID)(void) const PURE;
	STDMETHOD_(const sPropertyTypeDesc*,DescribeType)(void) const PURE;
	STDMETHOD_(long,Create)(int) PURE;
	STDMETHOD_(long,Copy)(int,int) PURE;
	STDMETHOD_(long,Delete)(int) PURE;
	STDMETHOD_(int,IsRelevant)(int) const PURE;
	STDMETHOD_(int,IsSimplyRelevant)(int) const PURE;
	STDMETHOD_(int,Touch)(int) PURE;
	STDMETHOD_(void,Notify)(unsigned long,void*) PURE;
	STDMETHOD_(PropListenerHandle,Listen)(unsigned long,PropListenFunc,PropListenerData) PURE;
	STDMETHOD_(long,Unlisten)(PropListenerHandle) PURE;
	STDMETHOD_(void,IterStart)(sPropertyObjIter*) const PURE;
	STDMETHOD_(int,IterNext)(sPropertyObjIter*,int*) const PURE;
	STDMETHOD_(void,IterStop)(sPropertyObjIter*) const PURE;
};
DEFINE_IIDSTRUCT(IProperty,IID_IProperty)

interface IGenericProperty : IProperty
{
	STDMETHOD_(int,Set)(int,void*) PURE;
	STDMETHOD_(int,TouchValue)(int,void*) PURE;
	STDMETHOD_(int,Get)(int,void**) const PURE;
	STDMETHOD_(int,GetSimple)(int,void**) const PURE;
	STDMETHOD_(int,IterNextValue)(sPropertyObjIter*,int*,void**) const PURE;
};

extern  const GUID  IID_IIntProperty;
interface IIntProperty : IProperty
{
	STDMETHOD_(int,Set)(int,int) PURE;
	STDMETHOD_(int,TouchValue)(int,int) PURE;
	STDMETHOD_(int,Get)(int,int*) PURE;
	STDMETHOD_(int,GetSimple)(int,int*) PURE;
	STDMETHOD_(int,IterNextValue)(sPropertyObjIter*,int*,int*) const PURE;
};

extern  const GUID  IID_IBoolProperty;
#define IBoolProperty IIntProperty

extern  const GUID  IID_IFloatProperty;
interface IFloatProperty : IProperty
{
	STDMETHOD_(int,Set)(int,float) PURE;
	STDMETHOD_(int,TouchValue)(int,float) PURE;
	STDMETHOD_(int,Get)(int,float*) PURE;
	STDMETHOD_(int,GetSimple)(int,float*) PURE;
	STDMETHOD_(int,IterNextValue)(sPropertyObjIter*,int*,float*) const PURE;
};

extern  const GUID  IID_IStringProperty;
interface IStringProperty : IProperty
{
	STDMETHOD_(int,Set)(int,const char*) PURE;
	STDMETHOD_(int,TouchValue)(int,const char*) PURE;
	STDMETHOD_(int,Get)(int,const char* *) PURE;
	STDMETHOD_(int,GetSimple)(int,const char**) PURE;
	STDMETHOD_(int,IterNextValue)(sPropertyObjIter*,int*,const char* *) const PURE;
};

extern  const GUID  IID_IVectorProperty;
interface IVectorProperty : IProperty
{
	STDMETHOD_(int,Set)(int,const mxs_vector*) PURE;
	STDMETHOD_(int,TouchValue)(int,const mxs_vector*) PURE;
	STDMETHOD_(int,Get)(int,mxs_vector**) PURE;
	STDMETHOD_(int,GetSimple)(int,mxs_vector**) PURE;
	STDMETHOD_(int,IterNextValue)(sPropertyObjIter*,int*,mxs_vector**) const PURE;
};

extern  const GUID  IID_ILabelProperty;

extern  const GUID  IID_IInvRenderProperty;

extern  const GUID  IID_IInvStringProperty;

extern  const GUID  IID_IBrushProperty;

extern  const GUID  IID_IGlobalIDProperty;

extern  const GUID  IID_ITweqSimpleProperty;

extern  const GUID  IID_ITweqSimpleStateProperty;

extern  const GUID  IID_ITweqVectorProperty;

extern  const GUID  IID_ITweqVectorStateProperty;

// Specific properties.

extern  const GUID  IID_IMovingTerrainProperty;

extern  const GUID  IID_IPhysStateProperty;

extern  const GUID  IID_IRendFlashProperty;

extern  const GUID  IID_IScriptProperty;


#endif // _LG_PROPERTIES_H
