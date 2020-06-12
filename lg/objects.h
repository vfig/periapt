/**********************
 * LGS Objects &c.
 */

#ifndef _LG_OBJECTS_H
#define _LG_OBJECTS_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <lg/objstd.h>
#include <lg/iiddef.h>

#include <lg/types.h>
#include <lg/defs.h>


interface IProperty;
interface IIntProperty;
interface ITagFile;


extern  const GUID  IID_IObjectQuery;
interface IObjectQuery : IUnknown
{
	STDMETHOD_(int,Done)(void) PURE;
	STDMETHOD_(int,Object)(void) PURE;
	STDMETHOD_(long,Next)(void) PURE;
};
DEFINE_IIDSTRUCT(IObjectQuery,IID_IObjectQuery)

extern  const GUID  IID_IObjectSystem;
struct sObjListenerDesc
{
};
struct sObjPartitionFilter
{
};
interface IObjectSystem : IUnknown
{
	STDMETHOD_(int,Create)(int,unsigned long) PURE;
	STDMETHOD_(int,BeginCreate)(int,unsigned long) PURE;
	STDMETHOD_(long,EndCreate)(int) PURE;
	STDMETHOD_(long,Destroy)(int) PURE;
	STDMETHOD_(long,CloneObject)(int,int) PURE;
	STDMETHOD_(long,Reset)(void) PURE;
	STDMETHOD_(int,Exists)(int) PURE;
	STDMETHOD_(long,NameObject)(int,const char*) PURE;
	STDMETHOD_(const char*,GetName)(int) PURE;
	STDMETHOD_(int,GetObjectNamed)(const char*) PURE;
	STDMETHOD_(long,SetObjTransience)(int,int) PURE;
	STDMETHOD_(int,ObjIsTransient)(int) PURE;
	STDMETHOD_(long,SetObjImmobility)(int,int) PURE;
	STDMETHOD_(int,ObjIsImmobile)(int) PURE;
	STDMETHOD_(IObjectQuery*,Iter)(unsigned long) PURE;
	STDMETHOD_(IProperty*,GetCoreProperty)(unsigned long) PURE;
	STDMETHOD_(long,DatabaseNotify)(unsigned long,void*) PURE;
	STDMETHOD_(long,f17)(void) PURE;
	STDMETHOD_(int,Listen)(sObjListenerDesc*) PURE;
	STDMETHOD_(long,Unlisten)(int) PURE;
	STDMETHOD_(int,MaxObjID)(void) PURE;
	STDMETHOD_(int,MinObjID)(void) PURE;
	STDMETHOD_(int,ActiveObjects)(void) PURE;
	STDMETHOD_(long,Lock)(void) PURE;
	STDMETHOD_(long,Unlock)(void) PURE;
	STDMETHOD_(int,IsObjSavePartition)(int,unsigned long) PURE;
	STDMETHOD_(int,IsObjLoadPartition)(int,unsigned long) PURE;
	STDMETHOD_(long,ObjDefaultPartition)(int) PURE;
	STDMETHOD_(int,RemapOnLoad)(int) PURE;
	STDMETHOD_(long,SetSubPartitionFilter)(unsigned long,const sObjPartitionFilter*) PURE;
};
DEFINE_IIDSTRUCT(IObjectSystem,IID_IObjectSystem)
 
 /* 
interface IObjectNetworking : IUnknown
{
	STDMETHOD_(void,ClearTables)(void) PURE;
	STDMETHOD_(void,ObjRegisterProxy)(int,short,int) PURE;
	STDMETHOD_(int,ObjGetProxy)(int,short) PURE;
	STDMETHOD_(void,ObjDeleteProxy)(int) PURE;
	STDMETHOD_(int,ObjIsProxy)(int) PURE;	
	STDMETHOD_(int,ObjHostedHere)(int) PURE;
	STDMETHOD_(int,ObjLocalOnly)(int) PURE;
	STDMETHOD_(int,ObjHostPlayer)(int) PURE;
	STDMETHOD_(short,ObjHostObjID)(int) PURE;
	STDMETHOD_(void,ObjHostInfo)(int,int*,short*) PURE;
	STDMETHOD_(void,ObjTakeOver)(int) PURE;
	STDMETHOD_(void,ObjGiveTo)(int,int,int) PURE;
	STDMETHOD_(void,ObjGiveWithoutObjID)(int,int) PURE;
	STDMETHOD_(void,StartBeginCreate)(int,int) PURE;
	STDMETHOD_(void,FinishBeginCreate)(int) PURE;
	STDMETHOD_(void,StartEndCreate)(int) PURE;
	STDMETHOD_(void,FinishEndCreate)(int) PURE;
	STDMETHOD_(void,StartDestroy)(int) PURE;
	STDMETHOD_(void,FinishDestroy)(int) PURE;
};
*/

extern  const GUID  IID_IDonorCache;
struct sDonorCacheParams
{
};
struct sDonorCacheStats
{
};
interface IDonorCache : IUnknown
{
	STDMETHOD_(unsigned int,NewTrait)(const sTraitDesc*) PURE;
	STDMETHOD_(long,SetDonor)(int,unsigned long,int,int) PURE;
	STDMETHOD_(int,GetDonor)(int,unsigned long,int*,int*) PURE;
	STDMETHOD_(long,Clear)(void) PURE;
	STDMETHOD_(long,Flush)(int,unsigned long) PURE;
	STDMETHOD_(long,FlushObjSet)(IObjectQuery*,unsigned long) PURE;
	STDMETHOD_(long,SetParams)(const sDonorCacheParams*) PURE;
	STDMETHOD_(long,GetParams)(sDonorCacheParams*) PURE;
	STDMETHOD_(long,GetTotalStats)(sDonorCacheStats*) PURE;
	STDMETHOD_(long,GetStatsByTrait)(unsigned long,sDonorCacheStats*) PURE;
	STDMETHOD_(long,GetByTraitName)(const char*,sDonorCacheStats*) PURE;
	STDMETHOD_(long,GetMaxID)(void) PURE;
	STDMETHOD_(long,GetCacheEntries)(void) PURE;
};
DEFINE_IIDSTRUCT(IDonorCache,IID_IDonorCache)

extern  const GUID  IID_IDonorQuery;
interface IDonorQuery : IUnknown
{
	STDMETHOD_(int,Done)(void) PURE;
	STDMETHOD_(int,Object)(void) PURE;
	STDMETHOD_(long,Next)(void) PURE;
	STDMETHOD_(long,Priority)(void) PURE;
};
DEFINE_IIDSTRUCT(IDonorQuery,IID_IDonorQuery)

extern  const GUID  IID_IDamageModel;
struct sChainedEvent
{
};
struct sDamage
{
};
struct sImpact
{
};
struct sDamageMsg
{
};
interface IDamageModel : IUnknown
{
	STDMETHOD_(unsigned long,HandleImpact)(int,int,sImpact*,sChainedEvent*) PURE;
	STDMETHOD_(unsigned long,DamageObject)(int,int,sDamage*,sChainedEvent*,int) PURE;
	STDMETHOD_(unsigned long,SlayObject)(int,int,sChainedEvent*) PURE;
	STDMETHOD_(unsigned long,TerminateObject)(int,sChainedEvent*) PURE;
	STDMETHOD_(unsigned long,ResurrectObject)(int, int, sChainedEvent*) PURE;
	STDMETHOD_(long,Filter)(unsigned long (__stdcall *)(int,int,sDamage*,void*),void*) PURE;
	STDMETHOD_(long,Listen)(unsigned long,unsigned long (__stdcall *)(const sDamageMsg*,void*),void*) PURE;
	STDMETHOD_(long,Init)(void) PURE;
	STDMETHOD_(long,End)(void) PURE;
};
DEFINE_IIDSTRUCT(IDamageModel,IID_IDamageModel)

extern  const GUID  IID_IContainSys;
struct sContainIter
{
	int zero;
	eContainType ContainType;
	long Link;
	int Object;
	short unknown;
	short count;
};
interface IContainSys : IUnknown
{
	STDMETHOD_(long,Listen)(int,int (__cdecl *)(eContainsEvent,int,int,int,void*),void*) PURE;
	STDMETHOD_(int,CheckCallback)(int,eContainsEvent,int,int,int) PURE;
	STDMETHOD_(long,Add)(int,int,int,unsigned int) PURE;
	STDMETHOD_(long,Remove)(int,int) PURE;
	STDMETHOD_(long,SetContainType)(int,int,int) PURE;
	STDMETHOD_(int,IsHeld)(int,int) PURE;
	STDMETHOD_(int,Contains)(int,int) PURE;
	STDMETHOD_(int,GetContainer)(int) PURE;
	STDMETHOD_(long,MoveAllContents)(int,int,int) PURE;
	STDMETHOD_(long,CombineTry)(int,int,int) PURE;
	STDMETHOD_(int,CombineCount)(int,int) PURE;
	STDMETHOD_(long,CombineAdd)(int,int,int,unsigned long) PURE;
	STDMETHOD_(int,CanCombine)(int,int,int) PURE;
	STDMETHOD_(int,CanCombineContainer)(int,int,int) PURE;
	STDMETHOD_(int,RemoveFromStack)(int,unsigned long,unsigned long) PURE;
	STDMETHOD_(int,StackAdd)(int,int,unsigned long) PURE;
	STDMETHOD_(IIntProperty*,StackCountProp)(void) PURE;
	STDMETHOD_(sContainIter*,IterStart)(int) PURE;
	STDMETHOD_(sContainIter*,IterStartType)(int,int,int) PURE;
	STDMETHOD_(sContainIter*,IterStartInherits)(int) PURE;
	STDMETHOD_(sContainIter*,IterStartInheritsType)(int,int,int) PURE;
	STDMETHOD_(int,IterNext)(sContainIter*) PURE;
	STDMETHOD_(void,IterEnd)(sContainIter*) PURE;
	STDMETHOD_(long,DatabaseMessage)(unsigned long,IUnknown*) PURE;
};
DEFINE_IIDSTRUCT(IContainSys,IID_IContainSys)

extern  const GUID  IID_IInventory;
/*
enum eWhichInvObj
{
	kWhichInvObj0
};
*/
typedef int eWhichInvObj;
enum eCycleDirection
{
	kForward = 1,
	kBackward = -1
};
interface  IInventory : IUnknown
{
	STDMETHOD_(eInventoryType,GetType)(int) PURE;
	STDMETHOD_(long,Add)(int) PURE;
	STDMETHOD_(long,Remove)(int) PURE;
	STDMETHOD_(long,Select)(int) PURE;
	STDMETHOD_(int,Selection)(eWhichInvObj) PURE;
	STDMETHOD_(long,ClearSelection)(eWhichInvObj) PURE;
	STDMETHOD_(int,CycleSelection)(eWhichInvObj,eCycleDirection) PURE;
	STDMETHOD_(long,Wield)(eWhichInvObj) PURE;
	STDMETHOD_(int,WieldedObj)(void) PURE;
	STDMETHOD_(int,WieldingJunk)(void) PURE;
	STDMETHOD_(long,DatabaseNotify)(unsigned long,ITagFile*) PURE;
};
DEFINE_IIDSTRUCT(IInventory,IID_IInventory)

extern  const GUID  IID_IKeySys;
interface IKeySys : IUnknown
{
	STDMETHOD_(int,TryToUseKey)(int,int,eKeyUse);
};
DEFINE_IIDSTRUCT(IKeySys,IID_IKeySys)

extern  const GUID  IID_ITrait;
// cBaseTrait, cNullTrait, cCachedTrait, cInheritedTrait, cUninheritedTrait, cPredicateTrait,
interface ITrait : IUnknown
{
	STDMETHOD_(const sTraitDesc*,Describe)(void) PURE;
	STDMETHOD_(int,PossessedBy)(int) PURE;
	STDMETHOD_(int,IntrinsicTo)(int) PURE;
	STDMETHOD_(long,Touch)(int) PURE;
	STDMETHOD_(int,GetDonor)(int) PURE;
	STDMETHOD_(IObjectQuery*,GetAllDonors)(int) PURE;
	STDMETHOD_(IObjectQuery*,GetAllHeirs)(int,unsigned long) PURE;
};
DEFINE_IIDSTRUCT(ITrait,IID_ITrait)

// Worth a shot
extern  const GUID  IID_IAvatar;
interface IAvatar : IUnknown
{
	virtual unsigned long f0(void) = 0;
	STDMETHOD_(int,PlayerObjID)(void) PURE;
	STDMETHOD_(void,WriteTagInfo)(ITagFile *) PURE;
};
DEFINE_IIDSTRUCT(IAvatar,IID_IAvatar)

#endif // _LG_OBJECTS_H
