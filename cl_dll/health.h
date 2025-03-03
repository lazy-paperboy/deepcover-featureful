/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#pragma once
#if !defined(HEALTH_H)
#define HEALTH_H

#define DMG_IMAGE_LIFE		2	// seconds that image is up

#define DMG_IMAGE_POISON	0
#define DMG_IMAGE_ACID		1
#define DMG_IMAGE_COLD		2
#define DMG_IMAGE_DROWN		3
#define DMG_IMAGE_BURN		4
#define DMG_IMAGE_NERVE		5
#define DMG_IMAGE_RAD		6
#define DMG_IMAGE_SHOCK		7

//tf defines
#define DMG_IMAGE_CALTROP	8
#define DMG_IMAGE_TRANQ		9
#define DMG_IMAGE_CONCUSS	10
#define DMG_IMAGE_HALLUC	11
#define NUM_DMG_TYPES		12

#include "dmg_types.h"

// time-based damage
//mask off TF-specific stuff too
#define DMG_TIMEBASED		(~(0xff003fff))	// mask for time-based damage

#define DMG_FIRSTTIMEBASED  DMG_DROWN

//TF ADDITIONS
#define DMG_IGNITE			(1 << 24)	// Players hit by this begin to burn
#define DMG_RADIUS_MAX		(1 << 25)	// Radius damage with this flag doesn't decrease over distance
#define DMG_RADIUS_QUAKE	(1 << 26)	// Radius damage is done like Quake. 1/2 damage at 1/2 radius.
#define DMG_IGNOREARMOR		(1 << 27)	// Damage ignores target's armor
#define DMG_AIMED			(1 << 28)   // Does Hit location damage
#define DMG_WALLPIERCING	(1 << 29)	// Blast Damages ents through walls

#define DMG_CALTROP				(1<<30)
#define DMG_HALLUC				(1<<31)

// TF Healing Additions for TakeHealth
#define DMG_IGNORE_MAXHEALTH	DMG_IGNITE

// TF Redefines since we never use the originals
#define DMG_NAIL				DMG_SLASH
#define DMG_NOT_SELF			DMG_FREEZE

#define DMG_TRANQ				DMG_MORTAR
#define DMG_CONCUSS				DMG_SONIC

typedef struct
{
	float fExpire;
	float fBaseline;
	int	x, y;
} DAMAGE_IMAGE;

#endif // HEALTH_H
