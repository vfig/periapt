/************************
 * LGS Definitions
 */

#ifndef _LG_DEFS_H
#define _LG_DEFS_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <lg/types.h>


enum eAIMode
{
	kAIM_Asleep,
	kAIM_SuperEfficient,
	kAIM_Efficient,
	kAIM_Normal,
	kAIM_Combat,
	kAIM_Dead,
	kAIM_Num
};

enum eAIScriptAlertLevel
{
	kNoAlert,
	kLowAlert,
	kModerateAlert,
	kHighAlert
};

enum eAIAwareLevel
{
	kNoAwareness,
	kLowAwareness,
	kModerateAwareness,
	kHighAwareness
};

enum eAIScriptSpeed
{
	kSlow,
	kNormalSpeed,
	kFast
};

enum eAIAction
{
	kAINoAction,
	kAIGoto,
	kAIFrob,
	kAIManeuver
};

enum eAIActionPriority
{
	kLowPriorityAction,
	kNormalPriorityAction,
	kHighPriorityAction
};

enum eAIActionResult
{
	kActionDone,
	kActionFailed,
	kActionNotAttempted
};

enum eAIConverseState
{
	kAIConverseStateZero
};

enum eAIConvEventKind
{
	kMotion,
	kSchema,
	kTweqAll,
	kVoiceover,
	kEnd
};

enum eAIResult
{
	kAIResultZero
};

enum eAmbientFlags
{
	kAmbientFlagEnvironmental = 0x01,
	kAmbientFlagNoSharpCurve = 0x02,
	kAmbientFlagTurnedOff = 0x04,
	kAmbientFlagRemoveProp = 0x08,
	kAmbientFlagMusic = 0x10,
	kAmbientFlagSynch = 0x20,
	kAmbientFlagNoFade = 0x40,
	kAmbientFlagDestroyObj = 0x80,
	kAmbientFlagDoAutoOff = 0x100
};

enum eConstraintKind
{
	kNullConstraint,
	kConstrainBefore,
	kConstrainAfter
};

enum eContainsEvent
{
	kContainQueryAdd,
	kContainQueryCombine,
	kContainAdd,
	kContainRemove,
	kContainCombine
};

enum eContainType
{
	kContainTypeAlternate = -3,
	kContainTypeHand = -2,
	kContainTypeBelt = -1,
	kContainTypeGeneric = 0
};

enum eCritMsgNotification
{
	kCritMsgEnter,
	kCritMsgExit,
	kCritMsgIgnoring,
	kCritMsgTerminating,
	kCritMsgDebugging
};

enum eDoorState
{
	kDoorStateClosed,
	kDoorStateOpen,
	kDoorStateClosing,
	kDoorStateOpening,
	kDoorStateHalted
};

enum eEnvSoundLoc
{
	kEnvSoundOnObj,
	kEnvSoundAtObjLoc,
	kEnvSoundAmbient
};

enum eFrobLoc
{
	kFrobLocWorld,
	kFrobLocInv,
	kFrobLocTool,
	kFrobLocNone,
	kFrobNumActions = kFrobLocNone
};

enum eInventoryType
{
	kJunk,
	kItem,
	kWeapon
};

enum eKeyUse
{
	kKeyUseDefault,
	kKeyUseOpen,
	kKeyUseClose,
	kKeyUseCheck
};

enum ePhysAxes
{
	NoAxes,
	XAxis = 0x01,
	YAxis = 0x02,
	ZAxis = 0x04,
	NegXAxis = 0x08,
	NegYAxis = 0x10,
	NegZAxis = 0x20
};

enum ePhysForceType
{
	kPFT_Gravity,
	kPFT_Wind,
	kPFT_Current,
	kPFT_Collision
};

enum ePhysModelType
{
	kPMT_OBB,
	kPMT_Sphere,
	kPMT_SphereHat,
	kPMT_NumTypes,
	kPMT_Invalid = -128
};
/* Where is this used?
enum ePhysModelType
{
	kPMT_Sphere,
	kPMT_BSP,
	kPMT_Point,
	kPMT_OBB,
	kPMT_SphereHat,
	kPMT_NumTypes,
	kPMT_Invalid = -128
};
*/

enum eSlayResult
{
	kSlayResultNormal,
	kSlayResultNoEffect,
	kSlayResultTerminate,
	kSlayResultDestroy
};

enum eSoundNetwork
{
	kSoundNetwork0
};

enum eSoundSpecial
{
	kSoundNormal,
	kSoundLoop
};

enum eTrapFlags
{
	kTrapFlagOnce = 1,
	kTrapFlagInvert = 2,
	kTrapFlagNoOn = 4,
	kTrapFlagNoOff = 8
};

enum eTweqAction
{
	kTweqDoDefault,
	kTweqDoActivate,
	kTweqDoHalt,
	kTweqDoReset,
	kTweqDoContinue,
	kTweqDoForward,
	kTweqDoReverse
};

enum eTweqFlag
{
	kTweqFlagOn = 1,
	kTweqFlagReverse = 2,
	kTweqFlagReSynch = 4,
	kTweqFlagGoEdge = 8,
	kTweqFlagLapOne = 16
};

enum eTweqType
{
	kTweqTypeScale,
	kTweqTypeRotate,
	kTweqTypeJoints,
	kTweqTypeModels,
	kTweqTypeDelete,
	kTweqTypeEmitter,
	kTweqTypeFlicker,
	kTweqTypeLock,
	kTweqTypeAll,
	kTweqTypeNull
};

enum eTweqOperation
{
	kTweqOpKillAll,
	kTweqOpRemoveTweq,
	kTweqOpHaltTweq,
	kTweqOpStatusQuo,
	kTweqOpSlayAll,
	kTweqOpFrameEvent
};

enum eTweqDirection
{
	kTweqDirForward,
	kTweqDirReverse
};

enum goaltype
{
	kGoalNone,
	kGoalTake,
	kGoalSlay,
	kGoalLoot,
	kGoalGoTo
};


#endif // _LG_DEFS_H
