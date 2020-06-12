/**********************
 * LGS Script Services
 */

#ifndef _LG_SCRSERVICES_H
#define _LG_SCRSERVICES_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <lg/objstd.h>
#include <lg/iiddef.h>

#include <lg/types.h>
#include <lg/defs.h>


interface IScriptServiceBase : IUnknown
{
	STDMETHOD_(void,Init)(void) PURE;
	STDMETHOD_(void,End)(void) PURE;
};

extern  const GUID  IID_IActReactScriptService;
interface IActReactSrv : IScriptServiceBase
{
/*** React - Execute a reaction response.
 *  	= long - 
 *  	: reaction_kind - ID of the reaction to execute.
 *  	: float - Intensity of the reaction.
 *  	: object - The source object.
 *  	: object - The destination object.
 *  	: cMultiParm & - Argument for the reaction.
 *  	: ...
 */
	STDMETHOD_(long,React)(reaction_kind,float,object,object,cMultiParm &,cMultiParm &,cMultiParm &,cMultiParm &,cMultiParm &,cMultiParm &,cMultiParm &,cMultiParm &) PURE;
#if (_DARKGAME == 1)
/*** Stimulate - Stimulate an object.
 *  	= long - 
 *  	: object - The object to be stimulated.
 *  	: object - The stimulus object.
 *  	: float - The intensity of the stimulus.
 */
	STDMETHOD_(long,Stimulate)(object,object,float) PURE;
#else
/*** Stimulate - Stimulate an object.
 *  	= long - 
 *  	: object - The object to be stimulated.
 *  	: object - The stimulus object.
 *  	: float - The intensity of the stimulus.
 *  	: object - The source of the stimulus.
 */
	STDMETHOD_(long,Stimulate)(object,object,float,object) PURE;
#endif
/*** GetReactionNamed - Return the reaction ID given its name.
 *  	= int - ID of the reaction.
 *  	: const char * - The reaction name.
 */
	STDMETHOD_(int,GetReactionNamed)(const char *) PURE;
/*** GetReactionName - Return the name of a reaction given its ID.
 *  	= cScrStr - Name of the reaction.
 *  	: long - The reaction ID.
 */
	STDMETHOD_(cScrStr*,GetReactionName)(cScrStr &,long) PURE;
/*** SubscribeToStimulus - Register an object to receive stimulus messages of a particular type.
 *  	= long - 
 *  	: object - The object that will receive the messages.
 *  	: object - The type of stimulus.
 */
	STDMETHOD_(long,SubscribeToStimulus)(object,object) PURE;
/*** UnsubscribeToStimulus - Stop listening for stimulus messages.
 *  	= long - 
 *  	: object - The object that was previously registered.
 *  	: object - The type of stimulus.
 */
	STDMETHOD_(long,UnsubscribeToStimulus)(object,object) PURE;
/*** BeginContact - Initiate a stimulus contact on an object.
 *  	= long - 
 *  	: object - The stimulus object.
 *  	: object - The destination object.
 */
	STDMETHOD_(long,BeginContact)(object,object) PURE;
/*** EndContact - Terminate stimulus contact on an object.
 *  	= long - 
 *  	: object - The stimulus object.
 *  	: object - The destination object.
 */
	STDMETHOD_(long,EndContact)(object,object) PURE;
/*** SetSingleSensorContact - Briefly touch an object.
 *  	= long - 
 *  	: object - The stimulus object.
 *  	: object - The destination object.
 */
	STDMETHOD_(long,SetSingleSensorContact)(object,object) PURE;
};
DEFINE_IIDSTRUCT(IActReactSrv,IID_IActReactScriptService)

extern  const GUID  IID_IAIScriptService;
interface IAIScrSrv : IScriptServiceBase
{
/*** MakeGotoObjLoc - Instruct an AI to go to the current location of an object.
 *  	= true_bool - 
 *  	: int - Object ID of the AI.
 *  	: const object - The target object.
 *  	: eAIScriptSpeed - How quickly the AI will move.
 *  	: eAIActionPriority - Priority level the action will be evaluated at.
 *  	: const cMultiParm & - Arbitrary data that will be passed with the ObjActResult message.
 */
	STDMETHOD_(true_bool*,MakeGotoObjLoc)(true_bool &,int,const object &,eAIScriptSpeed,eAIActionPriority,const cMultiParm &) PURE;
/*** MakeFrobObj - Instruct an AI to frob an object with another object.
 *  	= true_bool - 
 *  	: int - Object ID of the AI.
 *  	: const object - The target object.
 *  	: const object - Object that the target will be frobbed with.
 *  	: eAIActionPriority - Priority level the action will be evaluated at.
 *  	: const cMultiParm & - Arbitrary data that will be passed with the ObjActResult message.
 */
	STDMETHOD_(true_bool*,MakeFrobObj)(true_bool &,int,const object &,const object &,eAIActionPriority,const cMultiParm &) PURE;
/*** MakeFrobObj - Instruct an AI to frob an object.
 *  	= true_bool - 
 *  	: int - Object ID of the AI.
 *  	: const object - The target object.
 *  	: eAIActionPriority - Priority level the action will be evaluated at.
 *  	: const cMultiParm & - Arbitrary data that will be passed with the ObjActResult message.
 */
	STDMETHOD_(true_bool*,MakeFrobObj)(true_bool &,int,const object &,eAIActionPriority,const cMultiParm &) PURE;
/*** GetAlertLevel - Return the current alert level of the AI.
 *  	= eAIScriptAlertLevel - The current level.
 *  	: int - Object ID of the AI.
 */
	STDMETHOD_(eAIScriptAlertLevel,GetAlertLevel)(int) PURE;
/*** SetMinimumAlert - Restrict the alert level of the AI.
 *  	: int - Object ID of the AI.
 *  	: eAIScriptAlertLevel - The lowest alert level allowable for the AI.
 */
	STDMETHOD_(void,SetMinimumAlert)(int,eAIScriptAlertLevel) PURE;
/*** ClearGoals - Make the AI forget what it's doing.
 *  	: int - Object ID of the AI.
 */
	STDMETHOD_(void,ClearGoals)(int) PURE;
/*** SetScriptFlags - 
 *  	: int - Object ID of the AI.
 *  	: int - 
 */
	STDMETHOD_(void,SetScriptFlags)(int,int) PURE;
/*** ClearAlertness - Reset the alert level to it's lowest allowable value.
 *  	: int - Object ID of the AI.
 */
	STDMETHOD_(void,ClearAlertness)(int) PURE;
/*** Signal - Send a SignalAI message with a given string to the AI.
 *  	: int - Object ID of the AI.
 *  	: const cScrStr & - A string that will be sent with the signal.
 */
	STDMETHOD_(void,Signal)(int,const cScrStr  &) PURE;
/*** StartConversation - Run a conversation pseudo-script.
 *  	= true_bool - 
 *  	: int - The object with the conversation script.
 */
	STDMETHOD_(true_bool*,StartConversation)(true_bool &,int) PURE;
};
DEFINE_IIDSTRUCT(IAIScrSrv,IID_IAIScriptService)

extern  const GUID  IID_IAnimTextureScriptService;
interface IAnimTextureSrv : IScriptServiceBase
{
/*** ChangeTexture - Swap one texture on nearby polygons.
 *  	= long - 
 *  	: object - The host object.
 *  	: const char * - The path to the texture family of the old texture. May be NULL.
 *  	: const char * - The name of the old texture. If the path is NULL, this must be a full path.
 *  	: const char * - The path to the texture family of the new texture. May be NULL.
 *  	: const char * - The name of the new texture. If the path is NULL, this must be a full path.
 */
	STDMETHOD_(long,ChangeTexture)(object,const char *,const char *,const char *,const char *) PURE;
};
DEFINE_IIDSTRUCT(IAnimTextureSrv,IID_IAnimTextureScriptService)

extern  const GUID  IID_IBowScriptService;
interface IBowSrv : IScriptServiceBase
{
/*** Equip - Set the bow as the current weapon.
 *  	= long - 
 */
	STDMETHOD_(long,Equip)(void) PURE;
/*** UnEquip - Clear the current weapon if it was the bow.
 *  	= long - 
 */
	STDMETHOD_(long,UnEquip)(void) PURE;
/*** IsEquipped - Check if the bow is the current weapon.
 *  	= int - Non-zero if the bow is the current weapon.
 */
	STDMETHOD_(int,IsEquipped)(void) PURE;
/*** StartAttack - Begin drawing the bow.
 *  	= long - 
 */
	STDMETHOD_(long,StartAttack)(void) PURE;
/*** FinishAttack - Release the bow.
 *  	= long - 
 */
	STDMETHOD_(long,FinishAttack)(void) PURE;
/*** AbortAttack - Relax the bow without firing an arrow.
 *  	= long - 
 */
	STDMETHOD_(long,AbortAttack)(void) PURE;
/*** SetArrow - Make an object the current projectile for the bow.
 *  	= int - 
 *  	: object - The object that will be fired from the bow.
 */
	STDMETHOD_(int,SetArrow)(object) PURE;
};
DEFINE_IIDSTRUCT(IBowSrv,IID_IBowScriptService)

#if (_DARKGAME == 2)
extern  const GUID  IID_ICameraScriptService;
interface ICameraSrv : IScriptServiceBase
{
/*** StaticAttach - Attach camera to object. Do not allow freelook.
 *  	= long - 
 *  	: object - Object to attach to.
 */
	STDMETHOD_(long,StaticAttach)(object) PURE;
/*** DynamicAttach - Attach camera to object. Allow freelook.
 *  	= long - 
 *  	: object - Object to attach to.
 */
	STDMETHOD_(long,DynamicAttach)(object) PURE;
/*** CameraReturn - Return the camera to Player if it's still attached to the object.
 *  	= long - 
 *  	: object - Object to detach from. Does nothing if camera is attached to a different object.
 */
	STDMETHOD_(long,CameraReturn)(object) PURE;
/*** ForceCameraReturn - Return the camera to Player unconditionally.
 *  	= long - 
 */
	STDMETHOD_(long,ForceCameraReturn)(void) PURE;
};
DEFINE_IIDSTRUCT(ICameraSrv,IID_ICameraScriptService)
#endif

extern  const GUID  IID_IContainerScriptService;
interface IContainSrv : IScriptServiceBase
{
/*** Add - Move an object into a container.
 *  	= long - 
 *  	: object - The object to be contained.
 *  	: object - The container.
 *  	: int - The contain type.
 *  	: int - Combine the object if set to 1.
 */
	STDMETHOD_(long,Add)(object,object,int,int) PURE;
/*** Remove - Take an object out of a container.
 *  	= long - 
 *  	: object - The object that was contained.
 *  	: object - The container.
 */
	STDMETHOD_(long,Remove)(object,object) PURE;
/*** MoveAllContents - Move all the objects in one container into another.
 *  	= long - 
 *  	: object - The container to take the objects from.
 *  	: object - The container to place the objects in.
 *  	: int - Attempt to combine similar objects if set to 1.
 */
	STDMETHOD_(long,MoveAllContents)(object,object,int) PURE;
#if (_DARKGAME != 1)
/*** StackAdd - Modify the stack count of an object.
 *  	= long - 
 *  	: object - The object to add to.
 *  	: int - Amount to add. May be negative.
 */
	STDMETHOD_(long,StackAdd)(object,int) PURE;
/*** IsHeld - Check whether an object is contained by another.
 *  	= int - Returns the contain type or MAXLONG if the object is not contained.
 *  	: object - The container to check.
 *  	: object - The possibly contained object.
 */
	STDMETHOD_(int,IsHeld)(object,object) PURE;
#endif
};
DEFINE_IIDSTRUCT(IContainSrv,IID_IContainerScriptService)

extern  const GUID  IID_IDamageScriptService;
interface IDamageSrv : IScriptServiceBase
{
/*** Damage - Cause damage to an object.
 *  	= long - 
 *  	: object - Victim
 *  	: object - Culprit
 *  	: int - Intensity of damage
 *  	: int - Type of damage
 */
	STDMETHOD_(long,Damage)(object,object,int,int) PURE;
/*** Slay - Slay an object.
 *  	= long - 
 *  	: object - Victim
 *  	: object - Culprit
 */
	STDMETHOD_(long,Slay)(object,object) PURE;
/*** Resurrect - Reverse the effects of slaying an object.
 *  	= long - 
 *  	: object - Un-Victim
 *  	: object - Culprit
 */
	STDMETHOD_(long,Resurrect)(object,object) PURE;
};
DEFINE_IIDSTRUCT(IDamageSrv,IID_IDamageScriptService)

extern  const GUID  IID_IDarkGameScriptService;
interface IDarkGameSrv : IScriptServiceBase
{
/*** KillPlayer - Garrett dies. Ignored if no_endgame is set.
 *  	= long - 
 */
	STDMETHOD_(long,KillPlayer)(void) PURE;
/*** EndMission - Immediately stop the current mission. Ignored if no_endgame is set.
 *  	= long - 
 */
	STDMETHOD_(long,EndMission)(void) PURE;
/*** FadeToBlack - Gradually fade the screen. Ignored if no_endgame is set.
 *  	= long - 
 *  	: float - Speed of the fade, in seconds. If negative, the screen will immediately return to full visibility.
 */
	STDMETHOD_(long,FadeToBlack)(float) PURE;
#if (_DARKGAME == 2)
/*** FoundObject - Rings the secret bell.
 *  	= long - 
 *  	: int - object ID. (Will only work once per object?)
 */
	STDMETHOD_(long,FoundObject)(int) PURE;
/*** ConfigIsDefined - Test if a config variable has been set.
 *  	= int - Is non-zero if the variable has been set.
 *  	: const char * - The config variable to test for.
 */
	STDMETHOD_(int,ConfigIsDefined)(const char *) PURE;
/*** ConfigGetInt - Return the value of a config variable interpreted as an integer.
 *  	= int - 
 *  	: const char * - The config variable to retrieve.
 *  	: int * - Address of a variable that will recieve the value.
 */
	STDMETHOD_(int,ConfigGetInt)(const char *,int *) PURE;
/*** ConfigGetFloat - Return the value of a config variable interpreted as an floating-point number.
 *  	= int - 
 *  	: const char * - The config variable to retrieve.
 *  	: float * - Address of a variable that will recieve the value.
 */
	STDMETHOD_(int,ConfigGetFloat)(const char *,float *) PURE;
/*** BindingGetFloat - Return the floating-point value of a key-binding variable.
 *  	= float - The value of the variable.
 *  	: const char * - The name of the variable.
 */
	STDMETHOD_(float,BindingGetFloat)(const char *) PURE;
/*** GetAutomapLocationVisited - Return whether a region of the auto-map is marked as visited.
 *  	= int - Non-zero if the region was visited.
 *  	: int - The page number of the location.
 *  	: int - The room number of the location.
 */
	STDMETHOD_(int,GetAutomapLocationVisited)(int,int) PURE;
/*** SetAutomapLocationVisited -Mark a region of the auto-map as having been visited.
 *  	= long - 
 *  	: int - The page number of the location.
 *  	: int - The room number of the location.
 */
	STDMETHOD_(long,SetAutomapLocationVisited)(int,int) PURE;
#endif
};
DEFINE_IIDSTRUCT(IDarkGameSrv,IID_IDarkGameScriptService)

extern  const GUID  IID_IDarkUIScriptService;
interface IDarkUISrv : IScriptServiceBase
{
/*** TextMessage - Display a string on the screen.
 *  	= long - 
 *  	: const char * - The message to display.
 *  	: int - The color of the text. Equivalent to Win32 COLORREF. If this is zero, uses the default color, which is white.
 *  	: int - The maximum time, in milliseconds, to display the message. Passing -1001 will use the default time of 5 secs. (Actual time may be shorter if another call to TextMessage is made.)
 */
	STDMETHOD_(long,TextMessage)(const char *,int,int) PURE;
/*** ReadBook - Enter book mode.
 *  	= long - 
 *  	: const char * - The resource name of the book.
 *  	: const char * - The background image that will be shown.
 */
	STDMETHOD_(long,ReadBook)(const char *,const char *) PURE;
/*** InvItem - Get the currently selected inventory item.
 *  	= object - The inventory item.
 */
	STDMETHOD_(object*,InvItem)(object &) PURE;
/*** InvWeapon - Get the currently selected weapon.
 *  	= object - Object ID of the weapon.
 */
	STDMETHOD_(object*,InvWeapon)(object &) PURE;
/*** InvSelect - Set an object to be the active inventory selection.
 *  	= long - 
 *  	: object - Object ID of the item.
 */
	STDMETHOD_(long,InvSelect)(object) PURE;
/*** IsCommandBound - Test if a command string is bound to a key sequence.
 *  	= true_bool - Non-zero if the command is bound. Aggregate return.
 *  	: const cScrStr & - The command string to check for.
 */
	STDMETHOD_(true_bool*,IsCommandBound)(true_bool &,const cScrStr &) PURE;
/*** DescribeKeyBinding - Return the key-binding for a particular command.
 *  	= cScrStr - The keys bound to this command. Aggregate return.
 *  	: const cScrStr & - The command string.
 */
	STDMETHOD_(cScrStr*,DescribeKeyBinding)(cScrStr &,const cScrStr &) PURE;
};
DEFINE_IIDSTRUCT(IDarkUISrv,IID_IDarkUIScriptService)

extern  const GUID  IID_IDataScriptService;
interface IDataSrv : IScriptServiceBase
{
/*** GetString - Retrieve a string from a resource file.
 *  	= cScrStr - Aggregate return. Caller should free.
 *  	: const char * - The name of the resource file.
 *  	: const char * - The string name to get.
 *  	: const char * - unused
 *  	: const char * - The resource directory to look in. ("Books" is not a normal resource, use "..\Books\file" for the filename and "strings" here.)
 */
	STDMETHOD_(cScrStr*,GetString)(cScrStr &,const char *,const char *,const char *,const char *) PURE;
/*** GetObjString - Retrieve one of the standard object strings. Uses the GameName property.
 *  	= cScrStr - Aggregate return. Caller should free.
 *  	: int - The object ID to get the string of.
 *  	: const char * - The resource name to look in. "objnames" or "objdescs"
 */
	STDMETHOD_(cScrStr*,GetObjString)(cScrStr &,int,const char *) PURE;
/*** DirectRand - Return a (psuedo) random number in the range [0,32767]
 *  	= int - A randomly generated number.
 */
	STDMETHOD_(int,DirectRand)(void) PURE;
/*** RandInt - Return a (psuedo) random number in a given range. The range is inclusive on both ends.
 *  	= int - A randomly generated number.
 *  	: int - The low end of the range. The generated number will not be less than this.
 *  	: int - The high end of the range. The generated number will not be greater than this.
 */
	STDMETHOD_(int,RandInt)(int,int) PURE;
/*** RandFlt0to1 - Return a (psuedo) random floating-point number in the range [0,1].
 *  	= float - A randomly generated number.
 */
	STDMETHOD_(float,RandFlt0to1)(void) PURE;
/*** RandFltNeg1to1 - Return a (psuedo) random floating-point number in the range [-1,1).
 *  	= float - A randomly generated number.
 */
	STDMETHOD_(float,RandFltNeg1to1)(void) PURE;
};
DEFINE_IIDSTRUCT(IDataSrv,IID_IDataScriptService)

extern  const GUID  IID_IDebugScriptService;
interface IDebugScrSrv : IScriptServiceBase
{
/*** MPrint - Send string(s) to the mono.
 *  	= long - 
 *  	: cScrStr - A string to print.
 *  	: ... 
 */
	STDMETHOD_(long,MPrint)(const cScrStr &,const cScrStr &,const cScrStr &,const cScrStr &,const cScrStr &,const cScrStr &,const cScrStr &,const cScrStr &) PURE;
/*** Command - Execute commands.
 *  	= long - 
 *  	: cScrStr - Command string. A space will be inserted between this and the rest of the arguments.
 *  	: cScrStr - Command argument.
 *  	: ... 
 */
	STDMETHOD_(long,Command)(const cScrStr &,const cScrStr &,const cScrStr &,const cScrStr &,const cScrStr &,const cScrStr &,const cScrStr &,const cScrStr &) PURE;
/*** Break - Raise a debugger interrupt.
 *  	= long - 
 */
	STDMETHOD_(long,Break)(void) PURE;
};
DEFINE_IIDSTRUCT(IDebugScrSrv,IID_IDebugScriptService)

extern  const GUID  IID_IDoorScriptService;
interface IDoorSrv : IScriptServiceBase
{
/*** CloseDoor - Closes a door.
 *  	= int - 
 *  	: object - The door to close.
 */
	STDMETHOD_(int,CloseDoor)(object) PURE;
/*** OpenDoor - Open a door.
 *  	= int - 
 *  	: object - The door to open.
 */
	STDMETHOD_(int,OpenDoor)(object) PURE;
/*** GetDoorState - Return whether a door is open or closed (or something else).
 *  	= int - The state of the door. See eDoorState in lg/defs.h.
 *  	: object - The door.
 */
	STDMETHOD_(int,GetDoorState)(object) PURE;
/*** ToggleDoor - Switch the state of a door.
 *  	= long - 
 *  	: object - The door to open/close.
 */
	STDMETHOD_(long,ToggleDoor)(object) PURE;
#if (_DARKGAME == 2)
/*** SetBlocking - Set whether a door can block sound.
 *  	= long - 
 *  	: object - The door to change.
 *  	: int - The blocking state. Zero to not block sound, non-zero to block.
 */
	STDMETHOD_(long,SetBlocking)(object,int) PURE;
/*** GetSoundBlocking - Return if a door blocks sound.
 *  	= int - Non-zero if the door blocks sound.
 *  	: object - The door to check..
 */
	STDMETHOD_(int,GetSoundBlocking)(object) PURE;
#endif
};
DEFINE_IIDSTRUCT(IDoorSrv,IID_IDoorScriptService)

extern  const GUID  IID_IDrkInvScriptService;
enum eDrkInvCap
{
	kDrkInvCapCycle,
	kDrkInvCapWorldFrob,
	kDrkInvCapWorldFocus,
	kDrkInvCapInvFrob
};
enum eDrkInvControl
{
	kDrkInvControlOn,
	kDrkInvControlOff,
	kDrkInvControlToggle
};
interface IDarkInvSrv : IScriptServiceBase
{
/*** CapabilityControl - 
 *  	: eDrkInvCap - 
 *  	: eDrkInvControl - 
 */
	STDMETHOD_(void,CapabilityControl)(eDrkInvCap,eDrkInvControl) PURE;
/*** AddSpeedControl - Change the speed of Player.
 *  	: char * - ID of the control. Speed controls can be stacked and removed out-of-order.
 *  	: float - Amount to multiply the speed by.
 *  	: float - Multiplier for turning speed. Appears to be ineffective.
 */
	STDMETHOD_(void,AddSpeedControl)(const char *,float,float) PURE;
/*** RemoveSpeedControl - Remove a previously applied speed control.
 *  	: char * - ID of the control.
 */
	STDMETHOD_(void,RemoveSpeedControl)(char *) PURE;
};
DEFINE_IIDSTRUCT(IDarkInvSrv,IID_IDrkInvScriptService)

extern  const GUID  IID_IDrkPowerupsScriptService;
interface IDarkPowerupsSrv : IScriptServiceBase
{
/*** TriggerWorldFlash - Create a blinding flash.
 *  	: object - The flash will occur at the location of this object. Any AI with a line-of-sight to the object will be blinded. A RenderFlash link on the object's archetype determines how the flash will appear on screen.
 */
	STDMETHOD_(void,TriggerWorldFlash)(object) PURE;
/*** ObjTryDeploy - 
 *  	= int - 
 *  	: object - 
 *  	: object - 
 */
	STDMETHOD_(int,ObjTryDeploy)(object,object) PURE;
/*** CleanseBlood - Slay nearby objects with Prox\Blood
 *  	: object - Culprit object.
 *  	: float - The radius around the object that will be cleaned.
 */
	STDMETHOD_(void,CleanseBlood)(object,float) PURE;
};
DEFINE_IIDSTRUCT(IDarkPowerupsSrv,IID_IDrkPowerupsScriptService)

extern  const GUID  IID_IKeyScriptService;
interface IKeySrv : IScriptServiceBase
{
/*** TryToUseKey - Use or test a key on an object.
 *  	= int - Non-zero if the operation was successful.
 *  	: const object & - The key object.
 *  	: const object & - The object to use the key on.
 *  	: eKeyUse - The operation to attempt.
 */
	STDMETHOD_(int,TryToUseKey)(const object &,const object &,eKeyUse) PURE;
};
DEFINE_IIDSTRUCT(IKeySrv,IID_IKeyScriptService)

extern  const GUID  IID_ILightScriptService;
interface ILightScrSrv : IScriptServiceBase
{
/*** Set - Set the mode and min/max values of the light.
 *  	: const object & - The light object.
 *  	: int - The light mode.
 *  	: float - Minimum brightness of the light.
 *  	: float - Maximum brightness of the light.
 */
	STDMETHOD_(void,Set)(const object &,int,float,float) PURE;
/*** SetMode - Set the current mode of the light.
 *  	: const object & - The light object.
 *  	: int - The light mode.
 */
	STDMETHOD_(void,SetMode)(const object &,int) PURE;
/*** Activate - Clear the inactive flag on the light.
 *  	: const object & - The light object.
 */
	STDMETHOD_(void,Activate)(const object &) PURE;
/*** Deactivate - Set the inactive flag on the light.
 *  	: const object & - The light object.
 */
	STDMETHOD_(void,Deactivate)(const object &) PURE;
/*** Subscribe - 
 *  	: const object & - The light object.
 */
	STDMETHOD_(void,Subscribe)(const object &) PURE;
/*** Unsubscribe - 
 *  	: const object & - The light object.
 */
	STDMETHOD_(void,Unsubscribe)(const object &) PURE;
/*** GetMode - Return the current light mode.
 *  	= int - The current mode.
 *  	: const object & - The light object.
 */
	STDMETHOD_(int,GetMode)(const object &) PURE;
};
DEFINE_IIDSTRUCT(ILightScrSrv,IID_ILightScriptService)

extern  const GUID  IID_ILinkScriptService;
interface ILinkSrv : IScriptServiceBase
{
/*** Create - Add a link between two objects.
 *  	= link - The created link. Aggregate return.
 *  	: linkkind - The flavor of the link to create.
 *  	: object - The source of the link.
 *  	: object - The destination of the link.
 */
	STDMETHOD_(link*,Create)(link &,linkkind,object,object) PURE;
/*** Destroy - Removes a link.
 *  	= long - 
 *  	: link - The link to remove.
 */
	STDMETHOD_(long,Destroy)(link) PURE;
/*** AnyExist - Check if there is a link of a certain flavor between two objects.
 *  	= true_bool - True if at least one link exists. Aggregate return.
 *  	: linkkind - The flavor of the link to look for.
 *  	: object - The source of the link.
 *  	: object - The destination of the link.
 */
	STDMETHOD_(true_bool*,AnyExist)(true_bool &,linkkind,object,object) PURE;
/*** GetAll - Get all the links of certain flavor between two objects.
 *  	= linkset - The query of the requested links. Aggregate return.
 *  	: long - The link flavor requested. Not a linkkind for some buggy reason.
 *  	: object - The source of the links.
 *  	: object - The destination of the links.
 */
	STDMETHOD_(linkset*,GetAll)(linkset &,long,object,object) PURE;
/*** GetOne - Get just one link of certain flavor between two objects.
 *  	= link - The link. Aggregate return.
 *  	: linkkind - The flavor of the link requested.
 *  	: object - The source of the link.
 *  	: object - The destination of the link.
 */
	STDMETHOD_(link*,GetOne)(link &,linkkind,object,object) PURE;
/*** BroadcastOnAllLinks - Send a message to the destination of links from an object.
 *  	= long - 
 *  	: const object & - The source of the link(s). Also the source of the message(s).
 *  	: const char * - The name of the message to send.
 *  	: linkkind - The flavor of the link(s) to send the message accross.
 */
	STDMETHOD_(long,BroadcastOnAllLinks)(const object &,const char *,linkkind) PURE;
/*** BroadcastOnAllLinks - Send a message to the destination of links from an object, comparing data.
 *  	= long - 
 *  	: const object & - The source of the link(s). Also the source of the message(s).
 *  	: const char * - The name of the message to send.
 *  	: linkkind - The flavor of the link(s) to send the message accross.
 *  	: const cMultiParm & - The message is sent if the link data matches this exactly. It is copied to the first data parameter of the message.
 */
	STDMETHOD_(long,BroadcastOnAllLinks)(const object &,const char *,linkkind,const cMultiParm &) PURE;
/*** CreateMany - Add links between many different objects.
 *  	= long - 
 *  	: linkkind - The flavor of the links to create.
 *  	: const cScrStr & - The sources of the links.
 *  	: const cScrStr & - The destinations of the links.
 */
	STDMETHOD_(long,CreateMany)(linkkind,const cScrStr &,const cScrStr &) PURE;
/*** DestroyMany - Remove multiple links from objects.
 *  	= long - 
 *  	: linkkind - The flavor of the links to destroy.
 *  	: const cScrStr & - The sources of the links.
 *  	: const cScrStr & - The destinations of the links.
 */
	STDMETHOD_(long,DestroyMany)(linkkind,const cScrStr &,const cScrStr &) PURE;
/*** GetAllInherited - Retrieve links between objects and their descendants.
 *  	= linkset - The links between the objects. Aggregate return.
 *  	: linkkind - The flavor of the links to retrieve.
 *  	: object - The parent source of the links.
 *  	: object - The parent destination of the links.
 */
	STDMETHOD_(linkset*,GetAllInherited)(linkset &,linkkind,object,object) PURE;
/*** GetAllInheritedSingle - Retrieve links between objects and their descendants.
 *  	= linkset - The links between the objects. Aggregate return.
 *  	: linkkind - The flavor of the links to retrieve.
 *  	: object - The parent source of the links.
 *  	: object - The parent destination of the links.
 */
	STDMETHOD_(linkset*,GetAllInheritedSingle)(linkset &,linkkind,object,object) PURE;
};
DEFINE_IIDSTRUCT(ILinkSrv,IID_ILinkScriptService)

extern  const GUID  IID_ILinkToolsScriptService;
interface ILinkToolsSrv : IScriptServiceBase
{
/*** LinkKindNamed - Get the ID of a link flavor
 *  	= long - The flavor ID.
 *  	: const char * - The name of the flavor.
 */
	STDMETHOD_(long,LinkKindNamed)(const char *) PURE;
/*** LinkKindName - Get the name of a link flavor.
 *  	= cScrStr - The flavor name. Aggregate return.
 *  	: long - The ID of the flavor.
 */
	STDMETHOD_(cScrStr*,LinkKindName)(cScrStr &,long) PURE;
/*** LinkGet - Fill a structure with information about a link.
 *  	= long - 
 *  	: long - The link ID.
 *  	: sLink & - Address of the structure that will recieve the information.
 */
	STDMETHOD_(long,LinkGet)(long,sLink &) PURE;
/*** LinkGetData - Retrieve the data associated with a link.
 *  	= cMultiParm - Aggregate return. Caller should free.
 *  	: long - The link ID.
 *  	: const char * - Name of the structure field to retrieve.
 */
	STDMETHOD_(cMultiParm*,LinkGetData)(cMultiParm &,long,const char *) PURE;
/*** LinkSetData - Set the data associated with a link.
 *  	= long - 
 *  	: long - The link ID.
 *  	: const char * - The structure field to modify.
 *  	: const cMultiParm & - The data to assign.
 */
	STDMETHOD_(long,LinkSetData)(long,const char *,const cMultiParm &) PURE;
};
DEFINE_IIDSTRUCT(ILinkToolsSrv,IID_ILinkToolsScriptService)

extern  const GUID  IID_ILockedScriptService;
interface ILockSrv : IScriptServiceBase
{
/*** IsLocked - Test whether an object has the Locked property set.
 *  	= int - Non-zero if the object is locked.
 *  	: object - The object to check.
 */
	STDMETHOD_(int,IsLocked)(const object &) PURE;
};
DEFINE_IIDSTRUCT(ILockSrv,IID_ILockedScriptService)

#if (_DARKGAME == 3) || ((_DARKGAME == 2) && (_NETWORKING == 1))
extern  const GUID  IID_INetworkingScriptService;
interface INetworkingSrv : IScriptServiceBase
{
	STDMETHOD_(long,Broadcast)(const object &,const char*,int,const cMultiParm &) PURE;
	STDMETHOD_(long,SendToProxy)(const object &,const object &,const char*,const cMultiParm &) PURE;
	STDMETHOD_(long,TakeOver)(const object &) PURE;
	STDMETHOD_(long,GiveTo)(const object &,const object &) PURE;
	STDMETHOD_(int,IsPlayer)(const object &) PURE;
	STDMETHOD_(int,IsMultiplayer)(void) PURE;
	STDMETHOD_(int,SetProxyOneShotTimer)(const object &,const char *,float,const cMultiParm &) PURE;
	STDMETHOD_(object*,FirstPlayer)(object &) PURE;
	STDMETHOD_(object*,NextPlayer)(object &) PURE;
	STDMETHOD_(long,Suspend)(void) PURE;
	STDMETHOD_(long,Resume)(void) PURE;
	STDMETHOD_(int,HostedHere)(const object &) PURE;
	STDMETHOD_(int,IsProxy)(const object &) PURE;
	STDMETHOD_(int,LocalOnly)(const object &) PURE;
	STDMETHOD_(int,IsNetworking)(void) PURE;
#if (_DARKGAME == 2)
	STDMETHOD_(object*,Owner)(object &,const object &) PURE;
#endif
};
DEFINE_IIDSTRUCT(INetworkingSrv,IID_INetworkingScriptService)
#endif

extern  const GUID  IID_INullScriptService;

extern  const GUID  IID_IObjectScriptService;
interface IObjectSrv : IScriptServiceBase
{
/*** BeginCreate - Do the first half of a two-stage object creation.
 *  	= object - Aggregate return.
 *  	: object - The parent archetype for the new object.
 */
	STDMETHOD_(object*,BeginCreate)(object &,object) PURE;
/*** EndCreate - Finish creating an object returned from BeginCreate.
 *  	= long - 
 *  	: object - The created object.
 */
	STDMETHOD_(long,EndCreate)(object) PURE;
/*** Create - Add an object to the database.
 *  	= object - Aggregate return.
 *  	: object - The parent archetype for the new object.
 */
	STDMETHOD_(object*,Create)(object &,object) PURE;
/*** Destroy - Remove an object from the database.
 *  	= long - 
 *  	: object - The object to destroy.
 */
	STDMETHOD_(long,Destroy)(object) PURE;
/*** Exists - Check if an object is in the database.
 *  	= true_bool - Aggregate return.
 *  	: object - The object to look for.
 */
	STDMETHOD_(true_bool*,Exists)(true_bool &,object) PURE;
/*** SetName - Change an object's name.
 *  	= long - 
 *  	: object - The object to change.
 *  	: const char * - The new name of the object.
 */
	STDMETHOD_(long,SetName)(object,const char *) PURE;
/*** GetName - Return an object's name.
 *  	= cScrStr - The name of the object. Aggregate return.
 *  	: object - The object.
 */
	STDMETHOD_(cScrStr*,GetName)(cScrStr &,object) PURE;
/*** Named - Find an object given a name.
 *  	= object - The found object. Aggregate return.
 *  	: const char * - The name of the object.
 */
	STDMETHOD_(object*,Named)(object &,const char *) PURE;
/*** AddMetaProperty - Add a meta-property to an object.
 *  	= long - 
 *  	: object - The object to add the meta-property to.
 *  	: object - The meta-property to add.
 */
	STDMETHOD_(long,AddMetaProperty)(object,object) PURE;
/*** RemoveMetaProperty - Add a meta-property to an object.
 *  	= long - 
 *  	: object - The object to remove the meta-property from.
 *  	: object - The meta-property to remove.
 */
	STDMETHOD_(long,RemoveMetaProperty)(object,object) PURE;
/*** HasMetaProperty - Check if a meta-property is applied to an object.
 *  	= true_bool - Aggregate return.
 *  	: object - The object that might have the meta-property.
 *  	: object - The meta-property to look for.
 */
	STDMETHOD_(true_bool*,HasMetaProperty)(true_bool &,object,object) PURE;
/*** InheritsFrom - Check if an object inherits properties from another object. May be a direct ancestor or a meta-property.
 *  	= true_bool - Aggregate return.
 *  	: object - The object that might be a descendant.
 *  	: object - The ancestor object.
 */
	STDMETHOD_(true_bool*,InheritsFrom)(true_bool &,object,object) PURE;
/*** IsTransient - Check if an object is transient.
 *  	= true_bool - Aggregate return.
 *  	: object - The object to check.
 */
	STDMETHOD_(true_bool*,IsTransient)(true_bool &,object) PURE;
/*** SetTransience - Make an object transient, or not.
 *  	= long - 
 *  	: object - The object to change.
 *  	: true_bool - Whether or not the object should be transient.
 */
	STDMETHOD_(long,SetTransience)(object,true_bool) PURE;
/*** Position - Get the location of an object.
 *  	= cScrVec - The object's XYZ vector. Aggregate return.
 *  	: object - The object to query.
 */
	STDMETHOD_(cScrVec*,Position)(cScrVec &,object) PURE;
/*** Facing - Get the direction an object is facing.
 *  	= cScrVec - The object's HPB vector. Aggregate return.
 *  	: object - The object to query.
 */
	STDMETHOD_(cScrVec*,Facing)(cScrVec &,object) PURE;
/*** Teleport - Set the position and direction of an object.
 *  	= long - 
 *  	: object - The object to move.
 *  	: const cScrVec & - The new location for the object.
 *  	: const cScrVec & - The new facing for the object.
 *  	: object - An object to measure the new location and facing relative from. 0 for absolute positioning.
 */
	STDMETHOD_(long,Teleport)(object,const cScrVec  &,const cScrVec  &,object) PURE;
#if (_DARKGAME == 2) || (_DARKGAME == 3)
/*** IsPositionValid - Check if the object is inside the worldrep.
 *  	= true_bool - Aggregate return.
 *  	: object - The object to query.
 */
	STDMETHOD_(true_bool*,IsPositionValid)(true_bool &,object) PURE;
#endif
#if (_DARKGAME == 2)
/*** FindClosestObjectNamed -
 *  	= object - Aggregate return.
 *  	: int - 
 *  	: const char * - 
 */
	STDMETHOD_(object*,FindClosestObjectNamed)(object &, int,const char *) PURE;
#endif
/*** AddMetaPropertyToMany - Add a meta-property to multiple objects.
 *  	= int - 
 *  	: object - The meta-property to add.
 *  	: const cScrStr & - The objects to modify.
 */
	STDMETHOD_(int,AddMetaPropertyToMany)(object,const cScrStr  &) PURE;
/*** RemoveMetaPropertyToMany - Remove a meta-property from multiple objects.
 *  	= int - 
 *  	: object - The meta-property to remove.
 *  	: const cScrStr & - The objects to modify.
 */
	STDMETHOD_(int,RemoveMetaPropertyFromMany)(object,const cScrStr  &) PURE;
/*** RenderedThisFrame - Check if the object was included in the last rendering pass.
 *  	= true_bool - Aggregate return.
 *  	: object - The object to query.
 */
	STDMETHOD_(true_bool*,RenderedThisFrame)(true_bool &,object) PURE;
};
DEFINE_IIDSTRUCT(IObjectSrv,IID_IObjectScriptService)

extern  const GUID  IID_IPGroupScriptService;
interface IPGroupSrv : IScriptServiceBase
{
/*** SetActive - Turn a particle SFX on or off.
 *  	= long - 
 *  	: int - Object ID of the particle group.
 *  	: int - 1 to activate, 0 to deactivate.
 */
	STDMETHOD_(long,SetActive)(int,int) PURE;
};
DEFINE_IIDSTRUCT(IPGroupSrv,IID_IPGroupScriptService)

enum ePhysMsgType {
	kPhysCollision = 1,
	kPhysContact = 2,
	kPhysEnterExit = 4,
	kPhysFellAsleep = 8
};
extern  const GUID  IID_IPhysicsScriptService;
interface IPhysSrv : IScriptServiceBase
{
/*** SubscribeMsg - Register an object to receive certain messages.
 *  	= long - 
 *  	: object - Object that will recieve the messages.
 *  	: int - A bit-mask specifying which messages to begin listening for.
 */
	STDMETHOD_(long,SubscribeMsg)(object,int) PURE;
/*** UnsubscribeMsg - Stop listening for certain messages.
 *  	= long - 
 *  	: object - Object that recieved the messages.
 *  	: int - A bit-mask specifying which messages to stop listening for.
 */
	STDMETHOD_(long,UnsubscribeMsg)(object,int) PURE;
/*** LaunchProjectile - Emit an object.
 *  	= object - The emitted object. Aggregate return.
 *  	: object - Object to launch from.
 *  	: object - Archetype to emit.
 *  	: float - Velocity scale of projectile.
 *  	: int - Flags. 2 is PushOut.
 *  	: const cScrVec & - Randomize velocity.
 */
	STDMETHOD_(object*,LaunchProjectile)(object &,object,object,float,int,const cScrVec &) PURE;
/*** SetVelocity - Set the current speed of an object.
 *  	= long - 
 *  	: object - The object to modify.
 *  	: const cScrVec & - The new velocity of the object.
 */
	STDMETHOD_(long,SetVelocity)(object,const cScrVec &) PURE;
/*** GetVelocity - Retrieve the current speed of an object.
 *  	= long - 
 *  	: object - The object to query.
 *  	: cScrVec & - Address of a variable that will receive the velocity.
 */
	STDMETHOD_(long,GetVelocity)(object,cScrVec  &) PURE;
#if (_DARKGAME == 2)
/*** ControlVelocity - Lock the velocity of an object.
 *  	= long - 
 *  	: object - The object to modify.
 *  	: const cScrVec & - The new velocity of the object.
 */
	STDMETHOD_(long,ControlVelocity)(object,const cScrVec &) PURE;
/*** StopControlVelocity - Unlock the velocity of an object.
 *  	= long - 
 *  	: object - The object to modify.
 */
	STDMETHOD_(long,StopControlVelocity)(object) PURE;
#endif
/*** SetGravity - Set the gravity of an object.
 *  	= long - 
 *  	: object - The object to modify.
 *  	: float - The new gravity of the object.
 */
	STDMETHOD_(long,SetGravity)(object,float) PURE;
/*** GetGravity - Retrieve the gravity of an object.
 *  	= float - The current gravity.
 *  	: object - The object to query.
 */
	STDMETHOD_(float,GetGravity)(object) PURE;
#if (_DARKGAME != 1)
/*** HasPhysics - Check if an object has a physics model.
 *  	= int - Non-zero if the object has a physics model.
 *  	: object - The object to query.
 */
	STDMETHOD_(int,HasPhysics)(object) PURE;
/*** IsSphere - Check if an object is a sphere.
 *  	= int - Non-zero if the object has a sphere-type physics model.
 *  	: object - The object to query.
 */
	STDMETHOD_(int,IsSphere)(object) PURE;
/*** IsOBB - Check if an object is an oriented bounding-box.
 *  	= int - Non-zero if the object has an OBB-type physics model.
 *  	: object - The object to query.
 */
	STDMETHOD_(int,IsOBB)(object) PURE;
/*** ControlCurrentLocation - Lock the location of an object to its current values.
 *  	= long - 
 *  	: object - The object to lock.
 */
	STDMETHOD_(long,ControlCurrentLocation)(object) PURE;
/*** ControlCurrentRotation - Lock the facing of an object to its current values.
 *  	= long - 
 *  	: object - The object to lock.
 */
	STDMETHOD_(long,ControlCurrentRotation)(object) PURE;
/*** ControlCurrentPosition - Lock the location and facing of an object to its current values.
 *  	= long - 
 *  	: object - The object to lock.
 */
	STDMETHOD_(long,ControlCurrentPosition)(object) PURE;
/*** DeregisterModel - Remove the physics model of an object.
 *  	= long - 
 *  	: object - The object to modify.
 */
	STDMETHOD_(long,DeregisterModel)(object) PURE;
/*** PlayerMotionSetOffset - 
 *  	: int - 
 *  	: cScrVec & - 
 */
	STDMETHOD_(void,PlayerMotionSetOffset)(int,cScrVec &) PURE;
/*** Activate - 
 *  	= long - 
 *  	: object - 
 */
	STDMETHOD_(long,Activate)(object) PURE;
/*** ValidPos - 
 *  	= int - 
 *  	: object - 
 */
	STDMETHOD_(int,ValidPos)(object) PURE;
#endif
};
DEFINE_IIDSTRUCT(IPhysSrv,IID_IPhysicsScriptService)

extern  const GUID  IID_IPickLockScriptService;
interface IPickLockSrv : IScriptServiceBase
{
/*** Ready - Prepare a pick.
 *  	= int - 
 *  	: object - The host object.
 *  	: object - The pick.
 */
	STDMETHOD_(int,Ready)(object,object) PURE;
/*** UnReady - Release a pick.
 *  	= int - 
 *  	: object - The host object.
 *  	: object - The pick.
 */
	STDMETHOD_(int,UnReady)(object,object) PURE;
/*** StartPicking - Begin using a pick on an object.
 *  	= int - 
 *  	: object - The host object.
 *  	: object - The pick.
 *  	: object - The object being picked.
 */
	STDMETHOD_(int,StartPicking)(object,object,object) PURE;
/*** FinishPicking - Stop picking an object.
 *  	= int - 
 *  	: object - The pick.
 */
	STDMETHOD_(int,FinishPicking)(object) PURE;
/*** CheckPick - Test a pick.
 *  	= int - 
 *  	: object - The pick.
 *  	: object - The object being picked.
 *  	: int - ???
 */
	STDMETHOD_(int,CheckPick)(object,object,int) PURE;
/*** DirectMotion - ???
 *  	= int - 
 *  	: int - 
 */
	STDMETHOD_(int,DirectMotion)(int) PURE;
};
DEFINE_IIDSTRUCT(IPickLockSrv,IID_IPickLockScriptService)

extern  const GUID  IID_IPlayerLimbsScriptService;
interface IPlayerLimbsSrv : IScriptServiceBase
{
/*** Equip - Display the player arm.
 *  	= long - 
 *  	: object - 
 */
	STDMETHOD_(long,Equip)(object) PURE;
/*** UnEquip - Hide the player arm.
 *  	= long - 
 *  	: object - 
 */
	STDMETHOD_(long,UnEquip)(object) PURE;
/*** StartUse - Begin moving the player arm.
 *  	= long - 
 *  	: object - 
 */
	STDMETHOD_(long,StartUse)(object) PURE;
/*** FinishUse - End an arm motion.
 *  	= long - 
 *  	: object - 
 */
	STDMETHOD_(long,FinishUse)(object) PURE;
};
DEFINE_IIDSTRUCT(IPlayerLimbsSrv,IID_IPlayerLimbsScriptService)

extern  const GUID  IID_IPropertyScriptService;
interface IPropertySrv : IScriptServiceBase
{
/*** Get - Retrieve some data from a property.
 *  	= cMultiParm - Aggregate return. Caller should free if string or vector.
 *  	: object - The object to query.
 *  	: const char * - The property name.
 *  	: const char * - The field name. NULL if the property is only a single field, otherwise it's the exact name that's displayed in the editor dialog in DromEd.
 */
	STDMETHOD_(cMultiParm*,Get)(cMultiParm &,object,const char *,const char *) PURE;
/*** Set - Write a value into a field of a property.
 *  	= long - 
 *  	: const char * - The property name.
 *  	: const char * - The field name.
 *  	: const cMultiParm & - The data to write.
 */
	STDMETHOD_(long,Set)(object,const char *,const char *,const cMultiParm &) PURE;
/*** Set - Write a value into a simple property.
 *  	= long - 
 *  	: const char * - The property name.
 *  	: const cMultiParm & - The data to write.
 */
	STDMETHOD_(long,Set)(object,const char *,const cMultiParm &) PURE;
#if (_DARKGAME == 2) || (_DARKGAME == 3)
/*** Set - Write a value into a field of a property without propogating across the network.
 *  	= long - 
 *  	: const char * - The property name.
 *  	: const char * - The field name.
 *  	: const cMultiParm & - The data to write.
 */
	STDMETHOD_(long,SetLocal)(object,const char *,const char *,const cMultiParm &) PURE;
#endif
/*** Add - Create an instance of a property on an object.
 *  	= long - 
 *  	: object - The object to receive the property.
 *  	: const char * - The property name.
 */
	STDMETHOD_(long,Add)(object,const char *) PURE;
/*** Remove - Delete an instance of a property from an object.
 *  	= long - 
 *  	: object - The object to modify.
 *  	: const char * - The property name.
 */
	STDMETHOD_(long,Remove)(object,const char *) PURE;
/*** CopyFrom - Copy the contents of a property from one object to another.
 *  	= long - 
 *  	: object - The destination object.
 *  	: const char * - The property name.
 *  	: object - The source object.
 */
	STDMETHOD_(long,CopyFrom)(object,const char *,object) PURE;
/*** Possessed - Test if an object has an instance of a property.
 *  	= int - Non-zero if the property is on the object.
 *  	: object - The object to query.
 *  	: const char * - The property name.
 */
	STDMETHOD_(int,Possessed)(object,const char *) PURE;
};
DEFINE_IIDSTRUCT(IPropertySrv,IID_IPropertyScriptService)

extern  const GUID  IID_IPuppetScriptService;
interface IPuppetSrv : IScriptServiceBase
{
/*** PlayMotion - Play a motion schema on an object.
 *  	= true_bool - Aggregate return.
 *  	: object - The host object.
 *  	: const char * - The schema name.
 */
	STDMETHOD_(true_bool*,PlayMotion)(true_bool &,object,const char *) PURE;
};
DEFINE_IIDSTRUCT(IPuppetSrv,IID_IPuppetScriptService)

extern  const GUID  IID_IQuestScriptService;
interface IQuestSrv : IScriptServiceBase
{
/*** SubscribeMsg - Register an object to receive notifications when a quest variable changes.
 *  	= int - 
 *  	: object - The object that will receive the messages.
 *  	: const char * - The quest variable name.
 *  	: int - Flags of unknown relevance. Usually set to 2.
 */
	STDMETHOD_(int,SubscribeMsg)(object,const char *,int) PURE;
/*** UnsubscribeMsg - Stop listening for quest variable messages.
 *  	= int - 
 *  	: object - The object that was previously registered.
 *  	: const char * - The quest variable name.
 */
	STDMETHOD_(int,UnsubscribeMsg)(object,const char *) PURE;
/*** Set - Change the value of a quest variable.
 *  	= long - 
 *  	: const char * - The quest variable name.
 *  	: int - The new value of the variable.
 *  	: int - The database to modify. 0 is mission, 1 is campaign.
 */
	STDMETHOD_(long,Set)(const char *,int,int) PURE;
/*** Get - Retrieve the value of a quest variable.
 *  	= int - The value of the variable.
 *  	: const char * - The quest variable name.
 */
	STDMETHOD_(int,Get)(const char *) PURE;
/*** Exists - Test if a quest variable is defined.
 *  	= int - Non-zero if the variable exists.
 *  	: const char * - The quest variable name.
 */
	STDMETHOD_(int,Exists)(const char *) PURE;
/*** Delete - Remove a quest variable from either database.
 *  	= int - 
 *  	: const char * - The quest variable name.
 */
	STDMETHOD_(int,Delete)(const char *) PURE;
};
DEFINE_IIDSTRUCT(IQuestSrv,IID_IQuestScriptService)

#if (_DARKGAME == 3) || ((_DARKGAME == 1) && (_SHOCKINTERFACES == 1))
extern  const GUID  IID_IShockGameScriptService;
interface IShockGameSrv : IScriptServiceBase
{
	STDMETHOD_(long,DestroyCursorObj)(void) PURE;
	STDMETHOD_(long,DestroyInvObj)(const object &) PURE;
	STDMETHOD_(long,HideInvObj)(const object &) PURE;
	STDMETHOD_(long,SetPlayerPsiPoints)(int) PURE;
	STDMETHOD_(int,GetPlayerPsiPoints)(void) PURE;
	STDMETHOD_(long,AttachCamera)(const cScrStr &) PURE;
	STDMETHOD_(long,CutSceneModeOn)(const cScrStr &) PURE;
	STDMETHOD_(long,CutSceneModeOff)(void) PURE;
	STDMETHOD_(int,CreatePlayerPuppet)(const cScrStr &) PURE;
	STDMETHOD_(int,CreatePlayerPuppet)(void) PURE;
	STDMETHOD_(long,DestroyPlayerPuppet)(void) PURE;
	STDMETHOD_(long,Replicator)(const object &) PURE;
	STDMETHOD_(long,Container)(const object &) PURE;
#if (_DARKGAME == 1)
	virtual int __stdcall f13() = 0;
#endif
	STDMETHOD_(long,YorN)(const object &,const cScrStr &) PURE;
	STDMETHOD_(long,Keypad)(const object &) PURE;
#if (_DARKGAME == 3)
	STDMETHOD_(long,HRM)(int,const object &,int) PURE;
	STDMETHOD_(long,TechTool)(const object &) PURE;
#endif
	STDMETHOD_(long,UseLog)(const object &,int) PURE;
	STDMETHOD_(int,TriggerLog)(int,int,int,int) PURE;
	STDMETHOD_(long,FindLogData)(const object &,int,int *,int *) PURE;
	STDMETHOD_(long,PayNanites)(int) PURE;
	STDMETHOD_(long,OverlayChange)(int,int) PURE;
	STDMETHOD_(object*,Equipped)(object &,int) PURE;
	STDMETHOD_(long,LevelTransport)(const char *,int,unsigned int) PURE;
	STDMETHOD_(int,CheckLocked)(const object &,int,const object &) PURE;
	STDMETHOD_(long,AddText)(const char *,const object &,int) PURE;
	STDMETHOD_(long,AddTranslatableText)(const char *,const char *,const object &,int) PURE;
	STDMETHOD_(long,AmmoLoad)(const object &,const object &) PURE;
#if (_DARKGAME == 3)
	STDMETHOD_(int,GetClip)(const object &) PURE;
#endif
	STDMETHOD_(long,AddExp)(const object &,int,int) PURE;
	STDMETHOD_(int,HasTrait)(const object &,int) PURE;
	STDMETHOD_(int,HasImplant)(const object &,int) PURE;
	STDMETHOD_(long,HealObj)(const object &,int) PURE;
	STDMETHOD_(long,OverlaySetObj)(int,const object &) PURE;
	STDMETHOD_(long,Research)(void) PURE;
	STDMETHOD_(cScrStr*,GetArchetypeName)(cScrStr &,const object &) PURE;
	STDMETHOD_(int,OverlayOn)(int) PURE;
	STDMETHOD_(object*,FindSpawnPoint)(object &,const object &,unsigned int) PURE;
	STDMETHOD_(int,CountEcoMatching)(int) PURE;
	STDMETHOD_(int,GetStat)(const object &,int) PURE;
	STDMETHOD_(object*,GetSelectedObj)(object &) PURE;
	STDMETHOD_(int,AddInvObj)(const object &) PURE;
	STDMETHOD_(long,RecalcStats)(const object &) PURE;
	STDMETHOD_(long,PlayVideo)(const char *) PURE;
#if (_DARKGAME == 3)
	STDMETHOD_(long,ClearRadiation)(void) PURE;
	STDMETHOD_(void,SetPlayerVolume)(float) PURE;
	STDMETHOD_(int,RandRange)(int,int) PURE;
	STDMETHOD_(int,LoadCursor)(const object &) PURE;
	STDMETHOD_(void,AddSpeedControl)(const char *,float,float) PURE;
	STDMETHOD_(void,RemoveSpeedControl)(const char *) PURE;
	STDMETHOD_(long,PreventSwap)(void) PURE;
	STDMETHOD_(object*,GetDistantSelectedObj)(object &) PURE;
	STDMETHOD_(long,Equip)(int,const object &) PURE;
	STDMETHOD_(long,OverlayChangeObj)(int,int,const object &) PURE;
	STDMETHOD_(long,SetObjState)(const object &,int) PURE;
	STDMETHOD_(long,RadiationHack)(void) PURE;
	STDMETHOD_(long,DestroyAllByName)(char const *) PURE;
	STDMETHOD_(long,AddTextObjProp)(const object &,const char *,const object &,int) PURE;
	STDMETHOD_(long,DisableAlarmGlobal)(void) PURE;
	STDMETHOD_(void,Frob)(int) PURE;
	STDMETHOD_(long,TweqAllByName)(const char *,int) PURE;
	STDMETHOD_(long,SetExplored)(int,char) PURE;
	STDMETHOD_(long,RemoveFromContainer)(const object &,const object &) PURE;
	STDMETHOD_(long,ActivateMap)(void) PURE;
	STDMETHOD_(int,SimTime)(void) PURE;
	STDMETHOD_(void,StartFadeIn)(int,unsigned char,unsigned char,unsigned char) PURE;
	STDMETHOD_(void,StartFadeOut)(int,unsigned char,unsigned char,unsigned char) PURE;
	STDMETHOD_(long,GrantPsiPower)(const object &,int) PURE;
	STDMETHOD_(int,ResearchConsume)(const object &) PURE;
	STDMETHOD_(long,PlayerMode)(int) PURE;
	STDMETHOD_(long,EndGame)(void) PURE;
	STDMETHOD_(int,AllowDeath)(void) PURE;
	STDMETHOD_(long,AddAlarm)(int) PURE;
	STDMETHOD_(long,RemoveAlarm)(void) PURE;
	STDMETHOD_(float,GetHazardResistance)(int) PURE;
	STDMETHOD_(int,GetBurnDmg)(void) PURE;
	STDMETHOD_(object*,PlayerGun)(object &) PURE;
	STDMETHOD_(int,IsPsiActive)(int) PURE;
	STDMETHOD_(long,PsiRadarScan)(void) PURE;
	STDMETHOD_(object*,PseudoProjectile)(object &,const object &,const object &) PURE;
	STDMETHOD_(long,WearArmor)(const object &) PURE;
	STDMETHOD_(long,SetModify)(const object &,int) PURE;
	STDMETHOD_(int,Censored)(void) PURE;
	STDMETHOD_(long,DebriefMode)(int) PURE;
	STDMETHOD_(long,TlucTextAdd)(char *,char *,int) PURE;
	STDMETHOD_(long,Mouse)(int,int) PURE;
	STDMETHOD_(long,RefreshInv)(void) PURE;
	STDMETHOD_(long,TreasureTable)(const object &) PURE;
	STDMETHOD_(object*,OverlayGetObj)(object &) PURE;
	STDMETHOD_(long,VaporizeInv)(void) PURE;
	STDMETHOD_(long,ShutoffPsi)(void) PURE;
	STDMETHOD_(long,SetQBHacked)(const cScrStr &,int) PURE;
	STDMETHOD_(int,GetPlayerMaxPsiPoints)(void) PURE;
	STDMETHOD_(long,SetLogTime)(int,int,int) PURE;
	STDMETHOD_(long,AddTranslatableTextInt)(const char *,const char *,const object &,int,int) PURE;
	STDMETHOD_(long,ZeroControls)(const object &,int) PURE;
	STDMETHOD_(long,SetSelectedPsiPower)(int) PURE;
	STDMETHOD_(int,ValidGun)(const object &) PURE;
	STDMETHOD_(long,AddTranslatableTextIndexInt)(const char *,const char *,const object &,int,int,int) PURE;
	STDMETHOD_(int,IsAlarmActive)(void) PURE;
	STDMETHOD_(long,SlayAllByName)(const char *) PURE;
	STDMETHOD_(long,NoMove)(int) PURE;
	STDMETHOD_(long,PlayerModeSimple)(int) PURE;
	STDMETHOD_(long,UpdateMovingTerrainVelocity)(object,object,float) PURE;
	STDMETHOD_(int,MouseCursor)(void) PURE;
#endif

	//STDMETHOD_(long,LevelTransport)(const char *,object &,unsigned int) PURE;
	//STDMETHOD_(long,SpewLockData)(const object &,int) PURE;
};
DEFINE_IIDSTRUCT(IShockGameSrv,IID_IShockGameScriptService)

extern  const GUID  IID_IShockObjScriptService;
interface IShockObjSrv : IScriptServiceBase
{
	STDMETHOD_(int,FindScriptDonor)(int,cScrStr) PURE;
};
DEFINE_IIDSTRUCT(IShockObjSrv,IID_IShockObjScriptService)


/* These arguments were originally an enum. 
 */
extern  const GUID  IID_IShockPsiScriptService;
interface IShockPsiSrv : IScriptServiceBase
{
	STDMETHOD_(long,OnDeactivate)(int) PURE;
	STDMETHOD_(unsigned long,GetActiveTime)(int) PURE;
#if (_DARKGAME == 3)
	STDMETHOD_(int,IsOverloaded)(int) PURE;
#endif
};
DEFINE_IIDSTRUCT(IShockPsiSrv,IID_IShockPsiScriptService)
#endif

#if (_DARKGAME == 3)
extern  const GUID  IID_IShockAIScriptService;
interface IShockAISrv : IScriptServiceBase
{
	STDMETHOD_(int,Stun)(object,cScrStr,cScrStr,float) PURE;
	STDMETHOD_(int,IsStunned)(object) PURE;
	STDMETHOD_(int,UnStun)(object) PURE;
	STDMETHOD_(int,Freeze)(object,float) PURE;
	virtual int __stdcall f4() = 0;
	STDMETHOD_(int,UnFreeze)(object) PURE;
	STDMETHOD_(void,NotifyEnterTripwire)(object,object) PURE;
	STDMETHOD_(void,NotifyExitTripwire)(object,object) PURE;
	STDMETHOD_(int,ObjectLocked)(object) PURE;
	STDMETHOD_(void,ValidateSpawn)(object,object) PURE;
};
DEFINE_IIDSTRUCT(IShockAISrv,IID_IShockAIScriptService)

extern  const GUID  IID_IShockWeaponScriptService;
interface IShockWeaponSrv : IScriptServiceBase
{
	STDMETHOD_(void,SetWeaponModel)(const object &) PURE;
	STDMETHOD_(object*,GetWeaponModel)(object &) PURE;
	STDMETHOD_(object*,TargetScan)(object &,object) PURE;
	STDMETHOD_(void,Home)(object,object) PURE;
	STDMETHOD_(void,DestroyMelee)(const object &) PURE;
};
DEFINE_IIDSTRUCT(IShockWeaponSrv,IID_IShockWeaponScriptService)
#endif

extern  const GUID  IID_ISoundScriptService;
#if (_DARKGAME != 1)
#define SOUND_NET  ,eSoundNetwork
#else
#define SOUND_NET
#endif
interface ISoundScrSrv : IScriptServiceBase
{
/*** Play - Play a schema at a specific location.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: const cScrStr & - The schema tags.
 *  	: cScrVec & - The location the schema will originate from.
 *  	: eSoundSpecial - How the schema will be played.
 *  	: eSoundNetwork - 
 */
	STDMETHOD_(true_bool*,Play)(true_bool&,object,const cScrStr &,cScrVec &,eSoundSpecial SOUND_NET) PURE;
/*** Play - Play a schema at the location of an object.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: const cScrStr & - The schema tags.
 *  	: object - The object at which location the schema will be played.
 *  	: eSoundSpecial - How the schema will be played.
 *  	: eSoundNetwork - 
 */
	STDMETHOD_(true_bool*,Play)(true_bool&,object,const cScrStr &,object,eSoundSpecial SOUND_NET) PURE;
/*** Play - Play a schema.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: const cScrStr & - The schema tags.
 *  	: eSoundSpecial - How the schema will be played.
 *  	: eSoundNetwork - 
 */
	STDMETHOD_(true_bool*,Play)(true_bool&,object,const cScrStr &,eSoundSpecial SOUND_NET) PURE;
/*** PlayAmbient - Play an ambient schema.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: const cScrStr & - The schema tags.
 *  	: eSoundSpecial - How the schema will be played.
 *  	: eSoundNetwork - 
 */
	STDMETHOD_(true_bool*,PlayAmbient)(true_bool&,object,const cScrStr &,eSoundSpecial SOUND_NET) PURE;
/*** PlaySchema - Play a schema object at a specific location.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: object - The schema.
 *  	: cScrVec & - The location the schema will originate from.
 *  	: eSoundNetwork - 
 */
	STDMETHOD_(true_bool*,PlaySchema)(true_bool &,object,object,cScrVec & SOUND_NET) PURE;
/*** PlaySchema - Play a schema object at the location of an object.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: object - The schema.
 *  	: object - The object at which location the schema will be played.
 *  	: eSoundNetwork - 
 */
	STDMETHOD_(true_bool*,PlaySchema)(true_bool &,object,object,object SOUND_NET) PURE;
/*** PlaySchema - Play a schema object.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: object - The schema.
 *  	: eSoundNetwork - 
 */
	STDMETHOD_(true_bool*,PlaySchema)(true_bool &,object,object SOUND_NET) PURE;
/*** PlaySchemaAmbient - Play an ambient schema object.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: object - The schema.
 *  	: eSoundNetwork - 
 */
	STDMETHOD_(true_bool*,PlaySchemaAmbient)(true_bool &,object,object SOUND_NET) PURE;
/*** PlayEnvSchema - Play an environmental schema.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: const cScrStr & - The schema tags.
 *  	: object - The source object.
 *  	: object - 
 *  	: eEnvSoundLoc - How the schema will be played.
 *  	: eSoundNetwork - 
 */
	STDMETHOD_(true_bool*,PlayEnvSchema)(true_bool &,object,const cScrStr &,object,object,eEnvSoundLoc SOUND_NET) PURE;
/*** PlayVoiceOver - Play a voice-over schema.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: object - The schema.
 */
	STDMETHOD_(true_bool*,PlayVoiceOver)(true_bool &,object,object) PURE;
/*** Halt - Stop playing a schema.
 *  	= int - 
 *  	: object - The host object.
 *  	: const cScrStr & - The schema tags.
 *  	: object - The source object.
 */
	STDMETHOD_(int,Halt)(object,const cScrStr &,object) PURE;
/*** HaltSchema - Stop playing a schema.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: const cScrStr & - The schema tags.
 *  	: object - The source object.
 */
	STDMETHOD_(true_bool*,HaltSchema)(true_bool&,object,const cScrStr &,object) PURE;
/*** HaltSpeech - Prevent an AI from speaking.
 *  	= long - 
 *  	: object - The AI.
 */
	STDMETHOD_(long,HaltSpeech)(object) PURE;
/*** PreLoad - Read the waveform data for a schema before having to play it.
 *  	= true_bool - 
 *  	: object - The host object.
 *  	: const cScrStr & - The schema tags.
 */
	STDMETHOD_(true_bool*,PreLoad)(true_bool &,const cScrStr &) PURE;
};
#undef SOUND_NET
DEFINE_IIDSTRUCT(ISoundScrSrv,IID_ISoundScriptService)

extern  const GUID  IID_IWeaponScriptService;
interface IWeaponSrv : IScriptServiceBase
{
/*** Equip - Select an object as the current weapon.
 *  	= long - 
 *  	: object - The weapon.
 *  	: int - The type of weapon. 0 for a slashing weapon, 1 for a bashing weapon.
 */
	STDMETHOD_(long,Equip)(object,int) PURE;
/*** UnEquip - Clear the current weapon, if selected.
 *  	= long - 
 *  	: object - The weapon.
 */
	STDMETHOD_(long,UnEquip)(object) PURE;
/*** IsEquipped - Check if a particular object is the current weapon.
 *  	= long - 
 *  	: object - The host object.
 *  	: object - The weapon.
 */
	STDMETHOD_(int,IsEquipped)(object,object) PURE;
/*** StartAttack - Begin swinging the weapon.
 *  	= long - 
 *  	: object - The host object.
 *  	: object - The weapon.
 */
	STDMETHOD_(long,StartAttack)(object,object) PURE;
/*** FinishAttack - Stop swinging the weapon.
 *  	= long - 
 *  	: object - The host object.
 *  	: object - The weapon.
 */
	STDMETHOD_(long,FinishAttack)(object,object) PURE;
};
DEFINE_IIDSTRUCT(IWeaponSrv,IID_IWeaponScriptService)


#endif // _LG_SCRSERVICES_H
