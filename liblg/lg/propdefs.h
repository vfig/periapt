/***************************
 * LGS Property Definitions
 */

#ifndef _LG_PROPDEFS_H
#define _LG_PROPDEFS_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <lg/types.h>
#include <lg/defs.h>

#define AITEAM_GOOD		0 
#define AITEAM_NEUTRAL	1

#define AIAWARE_SEEN		0x00000001
#define AIAWARE_HEARD		0x00000002
#define AIAWARE_CANRAYCAST	0x00000004
#define AIAWARE_HAVELOS		0x00000008
#define AIAWARE_BLIND		0x00000010
#define AIAWARE_DEAF		0x00000020
#define AIAWARE_HIGHEST		0x00000040
#define AIAWARE_FIRSTHAND	0x00000080

// This should really be in linkdefs.h
struct sAIAwareness {
	int  i;
	int  flags;
	int  level;
	int  PeakLevel;
	int  LevelEnterTime;
	int  TimeLastContact;
	mxs_vector PosLastContact;
	int  i2;
	int  VisCone;
	int  TimeLastUpdate;
	int  TimeLastUpdateLOS;
};

struct  sAIAlertCap {
	int  iMax, iMin, iMinRelaxAfterPeak;
};


struct sLoot {
	int  iGold;
	int  iGems;
	int  iArt;
	int  iSpecial;
};

struct sMovingTerrain
{
	unsigned long	unknown;
	unsigned long	active;
};


#define PHYS_CONTROL_AXISVELS	1
#define PHYS_CONTROL_VELS		2
#define PHYS_CONTROL_ROTVELS	4

struct sPhysControl {
	int uiActive;
	mxs_vector axis_vels;
	mxs_vector vels;
	mxs_vector rot_vels;
};

struct sPhysState {
	mxs_vector loc;
	mxs_vector facing;
	mxs_vector vels;
	mxs_vector rot_vels;
};

#pragma pack(push,1)
struct sRenderFlash
{
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
	unsigned char	active;
	unsigned long	worldduration;
	unsigned long	screenduration;
	unsigned long	effectduration;
	float	time;
	float	range;
	unsigned long	starttime;
};
#pragma pack(pop)

#endif // _LG_PROPS_H
