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
/*

===== world.cpp ========================================================

  precaches and defs for entities and other data that must always be available.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "nodes.h"
#include "soundent.h"
#include "client.h"
#include "decals.h"
#include "skill.h"
#include "effects.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "game.h"

extern CSoundEnt *pSoundEnt;

extern CBaseEntity				*g_pLastSpawn;
DLL_GLOBAL edict_t				*g_pBodyQueueHead;
CGlobalState					gGlobalState;
extern DLL_GLOBAL int				gDisplayTitle;

extern void W_Precache( void );

//
// This must match the list in util.h
//
DLL_DECALLIST gDecals[] = {
	{ "{shot1", 0 },		// DECAL_GUNSHOT1 
	{ "{shot2", 0 },		// DECAL_GUNSHOT2
	{ "{shot3", 0 },		// DECAL_GUNSHOT3
	{ "{shot4", 0 },		// DECAL_GUNSHOT4
	{ "{shot5", 0 },		// DECAL_GUNSHOT5
	{ "{lambda01", 0 },		// DECAL_LAMBDA1
	{ "{lambda02", 0 },		// DECAL_LAMBDA2
	{ "{lambda03", 0 },		// DECAL_LAMBDA3
	{ "{lambda04", 0 },		// DECAL_LAMBDA4
	{ "{lambda05", 0 },		// DECAL_LAMBDA5
	{ "{lambda06", 0 },		// DECAL_LAMBDA6
	{ "{scorch1", 0 },		// DECAL_SCORCH1
	{ "{scorch2", 0 },		// DECAL_SCORCH2
	{ "{blood1", 0 },		// DECAL_BLOOD1
	{ "{blood2", 0 },		// DECAL_BLOOD2
	{ "{blood3", 0 },		// DECAL_BLOOD3
	{ "{blood4", 0 },		// DECAL_BLOOD4
	{ "{blood5", 0 },		// DECAL_BLOOD5
	{ "{blood6", 0 },		// DECAL_BLOOD6
	{ "{yblood1", 0 },		// DECAL_YBLOOD1
	{ "{yblood2", 0 },		// DECAL_YBLOOD2
	{ "{yblood3", 0 },		// DECAL_YBLOOD3
	{ "{yblood4", 0 },		// DECAL_YBLOOD4
	{ "{yblood5", 0 },		// DECAL_YBLOOD5
	{ "{yblood6", 0 },		// DECAL_YBLOOD6
	{ "{break1", 0 },		// DECAL_GLASSBREAK1
	{ "{break2", 0 },		// DECAL_GLASSBREAK2
	{ "{break3", 0 },		// DECAL_GLASSBREAK3
	{ "{bigshot1", 0 },		// DECAL_BIGSHOT1
	{ "{bigshot2", 0 },		// DECAL_BIGSHOT2
	{ "{bigshot3", 0 },		// DECAL_BIGSHOT3
	{ "{bigshot4", 0 },		// DECAL_BIGSHOT4
	{ "{bigshot5", 0 },		// DECAL_BIGSHOT5
	{ "{spit1", 0 },		// DECAL_SPIT1
	{ "{spit2", 0 },		// DECAL_SPIT2
	{ "{bproof1", 0 },		// DECAL_BPROOF1
	{ "{gargstomp", 0 },		// DECAL_GARGSTOMP1,	// Gargantua stomp crack
	{ "{smscorch1", 0 },		// DECAL_SMALLSCORCH1,	// Small scorch mark
	{ "{smscorch2", 0 },		// DECAL_SMALLSCORCH2,	// Small scorch mark
	{ "{smscorch3", 0 },		// DECAL_SMALLSCORCH3,	// Small scorch mark
	{ "{mommablob", 0 },		// DECAL_MOMMABIRTH		// BM Birth spray
	{ "{mommablob", 0 },		// DECAL_MOMMASPLAT		// BM Mortar spray?? need decal
	{ "{spr_splt1", 0 },
	{ "{spr_splt2", 0 },
	{ "{spr_splt3", 0 },
	{ "{ofscorch1", 0 },
	{ "{ofscorch2", 0 },
	{ "{ofscorch3", 0 },
	{ "{ofsmscorch1", 0 },
	{ "{ofsmscorch2", 0 },
	{ "{ofsmscorch3", 0 }
};

/*
==============================================================================

BODY QUE

==============================================================================
*/

#define SF_DECAL_NOTINDEATHMATCH		2048

class CDecal : public CBaseEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT StaticDecal( void );
	void EXPORT TriggerDecal( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( infodecal, CDecal )

// UNDONE:  These won't get sent to joining players in multi-player
void CDecal::Spawn( void )
{
	if( pev->skin < 0 || ( gpGlobals->deathmatch && FBitSet( pev->spawnflags, SF_DECAL_NOTINDEATHMATCH ) ) )
	{
		REMOVE_ENTITY( ENT( pev ) );
		return;
	}

	if( FStringNull( pev->targetname ) )
	{
		SetThink( &CDecal::StaticDecal );
		// if there's no targetname, the decal will spray itself on as soon as the world is done spawning.
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		// if there IS a targetname, the decal sprays itself on when it is triggered.
		SetThink( &CBaseEntity::SUB_DoNothing );
		SetUse( &CDecal::TriggerDecal );
	}
}

void CDecal::TriggerDecal( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// this is set up as a USE function for infodecals that have targetnames, so that the
	// decal doesn't get applied until it is fired. (usually by a scripted sequence)
	TraceResult trace;
	int entityIndex;

	UTIL_TraceLine( pev->origin - Vector( 5, 5, 5 ), pev->origin + Vector( 5, 5, 5 ),  ignore_monsters, ENT( pev ), &trace );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BSPDECAL );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( (int)pev->skin );
		entityIndex = (short)ENTINDEX( trace.pHit );
		WRITE_SHORT( entityIndex );
		if( entityIndex )
			WRITE_SHORT( (int)VARS( trace.pHit )->modelindex );
	MESSAGE_END();

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CDecal::StaticDecal( void )
{
	TraceResult trace;
	int entityIndex, modelIndex;

	UTIL_TraceLine( pev->origin - Vector( 5, 5, 5 ), pev->origin + Vector( 5, 5, 5 ),  ignore_monsters, ENT( pev ), &trace );

	entityIndex = (short)ENTINDEX( trace.pHit );
	if( entityIndex )
		modelIndex = (int)VARS( trace.pHit )->modelindex;
	else
		modelIndex = 0;

	g_engfuncs.pfnStaticDecal( pev->origin, (int)pev->skin, entityIndex, modelIndex );

	SUB_Remove();
}

void CDecal::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "texture" ) )
	{
		pev->skin = DECAL_INDEX( pkvd->szValue );

		// Found
		if( pev->skin >= 0 )
			return;
		ALERT( at_console, "Can't find decal %s\n", pkvd->szValue );
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

// Body queue class here.... It's really just CBaseEntity
class CCorpse : public CBaseEntity
{
	virtual int ObjectCaps( void ) { return FCAP_DONT_SAVE; }	
};

LINK_ENTITY_TO_CLASS( bodyque, CCorpse )

static void InitBodyQue( void )
{
	string_t istrClassname = MAKE_STRING( "bodyque" );

	g_pBodyQueueHead = CREATE_NAMED_ENTITY( istrClassname );
	entvars_t *pev = VARS( g_pBodyQueueHead );

	// Reserve 3 more slots for dead bodies
	for( int i = 0; i < 3; i++ )
	{
		pev->owner = CREATE_NAMED_ENTITY( istrClassname );
		pev = VARS( pev->owner );
	}

	pev->owner = g_pBodyQueueHead;
}

//
// make a body que entry for the given ent so the ent can be respawned elsewhere
//
// GLOBALS ASSUMED SET:  g_eoBodyQueueHead
//
void CopyToBodyQue( entvars_t *pev ) 
{
	if( pev->effects & EF_NODRAW )
		return;

	entvars_t *pevHead = VARS( g_pBodyQueueHead );

	pevHead->angles		= pev->angles;
	pevHead->model		= pev->model;
	pevHead->modelindex	= pev->modelindex;
	pevHead->frame		= pev->frame;
	pevHead->colormap	= pev->colormap;
	pevHead->movetype	= MOVETYPE_TOSS;
	pevHead->velocity	= pev->velocity;
	pevHead->flags		= 0;
	pevHead->deadflag	= pev->deadflag;
	pevHead->renderfx	= kRenderFxDeadPlayer;
	pevHead->renderamt	= ENTINDEX( ENT( pev ) );

	pevHead->effects = pev->effects | EF_NOINTERP;
	//pevHead->goalstarttime = pev->goalstarttime;
	//pevHead->goalframe = pev->goalframe;
	//pevHead->goalendtime = pev->goalendtime ;

	pevHead->sequence = pev->sequence;
	pevHead->animtime = pev->animtime;

	UTIL_SetOrigin( pevHead, pev->origin );
	UTIL_SetSize( pevHead, pev->mins, pev->maxs );
	g_pBodyQueueHead = pevHead->owner;
}

CGlobalState::CGlobalState( void )
{
	Reset();
}

void CGlobalState::Reset( void )
{
	m_pList = NULL; 
	m_listCount = 0;
}

globalentity_t *CGlobalState::Find( const char *globalname )
{
	if( !globalname || *globalname == '\0' )
		return NULL;

	globalentity_t *pTest;
	const char *pEntityName = globalname;

	pTest = m_pList;
	while( pTest )
	{
		if( FStrEq( pEntityName, pTest->name ) )
			break;

		pTest = pTest->pNext;
	}

	return pTest;
}

globalentity_t *CGlobalState::Find( string_t globalname )
{
	if( !globalname )
		return NULL;
	return Find(STRING(globalname));
}

// This is available all the time now on impulse 104, remove later
//#if _DEBUG
void CGlobalState::DumpGlobals( void )
{
	static const char *estates[] = { "Off", "On", "Dead" };
	globalentity_t *pTest;

	ALERT( at_console, "-- Globals --\n" );
	pTest = m_pList;
	while( pTest )
	{
		ALERT( at_console, "%s: %s (state: %s, value: %d)\n", pTest->name, pTest->levelName, estates[pTest->state], pTest->value );
		pTest = pTest->pNext;
	}
}
//#endif

void CGlobalState::EntityAdd(const char* globalname, string_t mapName, GLOBALESTATE state, int value)
{
	ASSERT( !Find( globalname ) );

	globalentity_t *pNewEntity = (globalentity_t *)calloc( sizeof( globalentity_t ), 1 );
	ASSERT( pNewEntity != NULL );
	pNewEntity->pNext = m_pList;
	m_pList = pNewEntity;
	strcpy( pNewEntity->name, globalname );
	strcpy( pNewEntity->levelName, STRING( mapName ) );
	pNewEntity->state = state;
	pNewEntity->value = value;
	m_listCount++;
}

void CGlobalState::EntityAdd(string_t globalname, string_t mapName, GLOBALESTATE state, int value)
{
	return EntityAdd(STRING(globalname), mapName, state, value);
}

void CGlobalState::EntitySetState( const char* globalname, GLOBALESTATE state )
{
	globalentity_t *pEnt = Find( globalname );
	if( pEnt )
		pEnt->state = state;
}

void CGlobalState::EntitySetState( string_t globalname, GLOBALESTATE state )
{
	globalentity_t *pEnt = Find( globalname );
	if( pEnt )
		pEnt->state = state;
}

void CGlobalState::IncrementValue(string_t globalname)
{
	globalentity_t *pEnt = Find( globalname );
	if( pEnt )
		pEnt->value += 1;
}

void CGlobalState::DecrementValue(string_t globalname)
{
	globalentity_t *pEnt = Find( globalname );
	if( pEnt )
		pEnt->value -= 1;
}

void CGlobalState::SetValue(const char* globalname, int value)
{
	globalentity_t *pEnt = Find( globalname );
	if( pEnt )
		pEnt->value = value;
}

void CGlobalState::SetValue(string_t globalname, int value)
{
	globalentity_t *pEnt = Find( globalname );
	if( pEnt )
		pEnt->value = value;
}

const globalentity_t *CGlobalState::EntityFromTable( const char* globalname )
{
	return Find( globalname );
}

const globalentity_t *CGlobalState::EntityFromTable( string_t globalname )
{
	return Find( globalname );
}

GLOBALESTATE CGlobalState::EntityGetState( string_t globalname )
{
	globalentity_t *pEnt = Find( globalname );
	if( pEnt )
		return pEnt->state;

	return GLOBAL_OFF;
}

int CGlobalState::GetValue(string_t globalname)
{
	globalentity_t *pEnt = Find( globalname );
	if( pEnt )
		return pEnt->value;
	return 0;
}

// Global Savedata for Delay
TYPEDESCRIPTION	CGlobalState::m_SaveData[] =
{
	DEFINE_FIELD( CGlobalState, m_listCount, FIELD_INTEGER ),
};

// Global Savedata for Delay
TYPEDESCRIPTION	gGlobalEntitySaveData[] =
{
	DEFINE_ARRAY( globalentity_t, name, FIELD_CHARACTER, 64 ),
	DEFINE_ARRAY( globalentity_t, levelName, FIELD_CHARACTER, 32 ),
	DEFINE_FIELD( globalentity_t, state, FIELD_INTEGER ),
	DEFINE_FIELD( globalentity_t, value, FIELD_INTEGER ),
};

int CGlobalState::Save( CSave &save )
{
	int i;
	globalentity_t *pEntity;

	if( !save.WriteFields( "GLOBAL", this, m_SaveData, ARRAYSIZE( m_SaveData ) ) )
		return 0;
	
	pEntity = m_pList;
	for( i = 0; i < m_listCount && pEntity; i++ )
	{
		if( !save.WriteFields( "GENT", pEntity, gGlobalEntitySaveData, ARRAYSIZE( gGlobalEntitySaveData ) ) )
			return 0;

		pEntity = pEntity->pNext;
	}
	
	return 1;
}

int CGlobalState::Restore( CRestore &restore )
{
	int i, listCount;
	globalentity_t tmpEntity;

	ClearStates();
	if( !restore.ReadFields( "GLOBAL", this, m_SaveData, ARRAYSIZE( m_SaveData ) ) )
		return 0;

	listCount = m_listCount;	// Get new list count
	m_listCount = 0;				// Clear loaded data

	for( i = 0; i < listCount; i++ )
	{
		if( !restore.ReadFields( "GENT", &tmpEntity, gGlobalEntitySaveData, ARRAYSIZE( gGlobalEntitySaveData ) ) )
			return 0;
		EntityAdd( MAKE_STRING( tmpEntity.name ), MAKE_STRING( tmpEntity.levelName ), tmpEntity.state, tmpEntity.value );
	}
	return 1;
}

void CGlobalState::EntityUpdate( string_t globalname, string_t mapname )
{
	globalentity_t *pEnt = Find( globalname );

	if( pEnt )
		strcpy( pEnt->levelName, STRING( mapname ) );
}

void CGlobalState::ClearStates( void )
{
	globalentity_t *pFree = m_pList;
	while( pFree )
	{
		globalentity_t *pNext = pFree->pNext;
		free( pFree );
		pFree = pNext;
	}
	Reset();
}

void SaveGlobalState( SAVERESTOREDATA *pSaveData )
{
	CSave saveHelper( pSaveData );
	gGlobalState.Save( saveHelper );
}

void RestoreGlobalState( SAVERESTOREDATA *pSaveData )
{
	CRestore restoreHelper( pSaveData );
	gGlobalState.Restore( restoreHelper );
}

void ResetGlobalState( void )
{
	gGlobalState.ClearStates();
	gInitHUD = TRUE;	// Init the HUD on a new game / load game
}

// moved CWorld class definition to cbase.h
//=======================
// CWorld
//
// This spawns first when each level begins.
//=======================

LINK_ENTITY_TO_CLASS( worldspawn, CWorld )

extern DLL_GLOBAL BOOL		g_fGameOver;

void CWorld::Spawn( void )
{
	g_fGameOver = FALSE;
	Precache();
}

int CWorld::wallPuffsIndices[] = {0,0,0,0};

void CWorld::Precache( void )
{
	g_pLastSpawn = NULL;
#if 1
	CVAR_SET_STRING( "sv_gravity", "800" ); // 67ft/sec
	CVAR_SET_STRING( "sv_stepsize", "18" );
#else
	CVAR_SET_STRING( "sv_gravity", "384" ); // 32ft/sec
	CVAR_SET_STRING( "sv_stepsize", "24" );
#endif
	CVAR_SET_STRING( "room_type", "0" );// clear DSP

	// Set up game rules
	if( g_pGameRules )
	{
		delete g_pGameRules;
		g_pGameRules = NULL;
	}

	g_pGameRules = InstallGameRules();

	//!!!UNDONE why is there so much Spawn code in the Precache function? I'll just keep it here 

	///!!!LATER - do we want a sound ent in deathmatch? (sjb)
	//pSoundEnt = CBaseEntity::Create( "soundent", g_vecZero, g_vecZero, edict() );
	pSoundEnt = GetClassPtr( ( CSoundEnt *)NULL );

	if( pSoundEnt )
	{
		pSoundEnt->Spawn();
	}
	else
	{
		ALERT ( at_console, "**COULD NOT CREATE SOUNDENT**\n" );
	}

	InitBodyQue();

	// init sentence group playback stuff from sentences.txt.
	// ok to call this multiple times, calls after first are ignored.
	SENTENCEG_Init();

	// the area based ambient sounds MUST be the first precache_sounds
	// player precaches
	W_Precache();				// get weapon precaches

	ClientPrecache();

	// sounds used from C physics code
	PRECACHE_SOUND( "common/null.wav" );// clears sound channels

	PRECACHE_SOUND( "items/suitchargeok1.wav" );//!!! temporary sound for respawning weapons.
	PRECACHE_SOUND( "items/gunpickup2.wav" );// player picks up a gun.

	PRECACHE_SOUND( "common/bodydrop3.wav" );// dead bodies hitting the ground (animation events)
	PRECACHE_SOUND( "common/bodydrop4.wav" );
	
	g_Language = (int)CVAR_GET_FLOAT( "sv_language" );
	if( g_Language == LANGUAGE_GERMAN )
	{
		PRECACHE_MODEL( "models/germangibs.mdl" );
	}
	else
	{
		PRECACHE_MODEL( "models/hgibs.mdl" );
		PRECACHE_MODEL( "models/agibs.mdl" );
	}

	PRECACHE_SOUND( "weapons/ric1.wav" );
	PRECACHE_SOUND( "weapons/ric2.wav" );
	PRECACHE_SOUND( "weapons/ric3.wav" );
	PRECACHE_SOUND( "weapons/ric4.wav" );
	PRECACHE_SOUND( "weapons/ric5.wav" );

	const char* wallPuffs[ARRAYSIZE(wallPuffsIndices)] = {
		g_modFeatures.wall_puff1,
		g_modFeatures.wall_puff2,
		g_modFeatures.wall_puff3,
		g_modFeatures.wall_puff4,
	};
	for (int wi = 0; wi < ARRAYSIZE(wallPuffsIndices); ++wi)
	{
		if (*wallPuffs[wi])
			wallPuffsIndices[wi] = PRECACHE_MODEL(wallPuffs[wi]);
	}

	//
	// Setup light animation tables. 'a' is total darkness, 'z' is maxbright.
	//

	// 0 normal
	LIGHT_STYLE( 0, "m" );

	// 1 FLICKER (first variety)
	LIGHT_STYLE( 1, "mmnmmommommnonmmonqnmmo" );

	// 2 SLOW STRONG PULSE
	LIGHT_STYLE( 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba" );

	// 3 CANDLE (first variety)
	LIGHT_STYLE( 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg" );

	// 4 FAST STROBE
	LIGHT_STYLE( 4, "mamamamamama" );

	// 5 GENTLE PULSE 1
	LIGHT_STYLE( 5,"jklmnopqrstuvwxyzyxwvutsrqponmlkj" );

	// 6 FLICKER (second variety)
	LIGHT_STYLE( 6, "nmonqnmomnmomomno" );

	// 7 CANDLE (second variety)
	LIGHT_STYLE( 7, "mmmaaaabcdefgmmmmaaaammmaamm" );

	// 8 CANDLE (third variety)
	LIGHT_STYLE( 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa" );

	// 9 SLOW STROBE (fourth variety)
	LIGHT_STYLE( 9, "aaaaaaaazzzzzzzz" );

	// 10 FLUORESCENT FLICKER
	LIGHT_STYLE( 10, "mmamammmmammamamaaamammma" );

	// 11 SLOW PULSE NOT FADE TO BLACK
	LIGHT_STYLE( 11, "abcdefghijklmnopqrrqponmlkjihgfedcba" );

	// 12 UNDERWATER LIGHT MUTATION
	// this light only distorts the lightmap - no contribution
	// is made to the brightness of affected surfaces
	LIGHT_STYLE( 12, "mmnnmmnnnmmnn" );

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	LIGHT_STYLE( 63, "a" );

	const int decalCount = g_modFeatures.opfor_decals ? (int)ARRAYSIZE( gDecals ) : DECAL_BASE_COUNT;

	for( int i = 0; i < decalCount; i++ )
		gDecals[i].index = DECAL_INDEX( gDecals[i].name );

	// init the WorldGraph.
	WorldGraph.InitGraph();

	// make sure the .NOD file is newer than the .BSP file.
	if( !WorldGraph.CheckNODFile( STRING( gpGlobals->mapname ) ) )
	{
		// NOD file is not present, or is older than the BSP file.
		WorldGraph.AllocNodes();
	}
	else
	{
		// Load the node graph for this level
		if( !WorldGraph.FLoadGraph( STRING( gpGlobals->mapname ) ) )
		{
			// couldn't load, so alloc and prepare to build a graph.
			ALERT( at_console, "*Error opening .NOD file\n" );
			WorldGraph.AllocNodes();
		}
		else
		{
			ALERT( at_console, "\n*Graph Loaded!\n" );
		}
	}

	if( pev->speed > 0 )
		CVAR_SET_FLOAT( "sv_zmax", pev->speed );
	else
		CVAR_SET_FLOAT( "sv_zmax", 4096 );

	// g-cont. moved here to right restore global WaveHeight on save\restore level
	CVAR_SET_FLOAT( "sv_wateramp", pev->scale );

	if( pev->netname )
	{
		ALERT( at_aiconsole, "Chapter title: %s\n", STRING( pev->netname ) );
		CBaseEntity *pEntity = CBaseEntity::Create( "env_message", g_vecZero, g_vecZero, NULL );
		if( pEntity )
		{
			pEntity->SetThink( &CBaseEntity::SUB_CallUseToggle );
			pEntity->pev->message = pev->netname;
			pev->netname = 0;
			pEntity->pev->nextthink = gpGlobals->time + 0.3f;
			pEntity->pev->spawnflags = SF_MESSAGE_ONCE;
		}
	}

	if( pev->spawnflags & SF_WORLD_DARK )
		CVAR_SET_FLOAT( "v_dark", 1.0f );
	else
		CVAR_SET_FLOAT( "v_dark", 0.0f );

	pev->spawnflags &= ~SF_WORLD_DARK;		// g-cont. don't apply fade after save\restore

	if( pev->spawnflags & SF_WORLD_TITLE )
		gDisplayTitle = TRUE;		// display the game title if this key is set
	else
		gDisplayTitle = FALSE;

	pev->spawnflags &= ~SF_WORLD_TITLE;		// g-cont. don't show logo after save\restore

	if( pev->spawnflags & SF_WORLD_FORCETEAM )
	{
		CVAR_SET_FLOAT( "mp_defaultteam", 1.0f );
	}
	else
	{
		CVAR_SET_FLOAT( "mp_defaultteam", 0.0f );
	}
}

//
// Just to ignore the "wad" field.
//
void CWorld::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "skyname" ) )
	{
		// Sent over net now.
		CVAR_SET_STRING( "sv_skyname", pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "sounds" ) )
	{
		gpGlobals->cdAudioTrack = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "WaveHeight" ) )
	{
		// Sent over net now.
		pev->scale = atof( pkvd->szValue ) * ( 1.0f / 8.0f );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "MaxRange" ) )
	{
		pev->speed = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "chaptertitle" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "startdark" ) )
	{
		// UNDONE: This is a gross hack!!! The CVAR is NOT sent over the client/sever link
		// but it will work for single player
		int flag = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
		if( flag )
			pev->spawnflags |= SF_WORLD_DARK;
	}
	else if( FStrEq(pkvd->szKeyName, "newunit") )
	{
		// Single player only.  Clear save directory if set
		if( atoi( pkvd->szValue ) )
			CVAR_SET_FLOAT( "sv_newunit", 1 );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "gametitle" ) )
	{
		if( atoi( pkvd->szValue ) )
			pev->spawnflags |= SF_WORLD_TITLE;

		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "mapteams" ) )
	{
		pev->team = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "defaultteam" ) )
	{
		if( atoi( pkvd->szValue ) )
		{
			pev->spawnflags |= SF_WORLD_FORCETEAM;
		}
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "freeroam" ) )
	{
		if (atoi( pkvd->szValue ))
		{
			pev->spawnflags |= SF_WORLD_FREEROAM;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


struct TitleCommet
{
	const char *mapname;
	const char *titlename;
};

TitleCommet gTitleComments[] =
{
	// default Half-Life map titles
	// ordering is important
	// strings hw.so| grep T0A0TITLE -B 50 -A 150
	{ "T0A0", "#T0A0TITLE" },
	{ "C0A0", "#C0A0TITLE" },
	{ "C1A0", "#C0A1TITLE" },
	{ "C1A1", "#C1A1TITLE" },
	{ "C1A2", "#C1A2TITLE" },
	{ "C1A3", "#C1A3TITLE" },
	{ "C1A4", "#C1A4TITLE" },
	{ "C2A1", "#C2A1TITLE" },
	{ "C2A2", "#C2A2TITLE" },
	{ "C2A3", "#C2A3TITLE" },
	{ "C2A4D", "#C2A4TITLE2" },
	{ "C2A4E", "#C2A4TITLE2" },
	{ "C2A4F", "#C2A4TITLE2" },
	{ "C2A4G", "#C2A4TITLE2" },
	{ "C2A4", "#C2A4TITLE1" },
	{ "C2A5", "#C2A5TITLE" },
	{ "C3A1", "#C3A1TITLE" },
	{ "C3A2", "#C3A2TITLE" },
	{ "C4A1A", "#C4A1ATITLE" },
	{ "C4A1B", "#C4A1ATITLE" },
	{ "C4A1C", "#C4A1ATITLE" },
	{ "C4A1D", "#C4A1ATITLE" },
	{ "C4A1E", "#C4A1ATITLE" },
	{ "C4A1", "#C4A1TITLE" },
	{ "C4A2", "#C4A2TITLE" },
	{ "C4A3", "#C4A3TITLE" },
	{ "C5A1", "#C5TITLE" },
	{ "OFBOOT", "#OF_BOOT0TITLE" },
	{ "OF0A", "#OF1A1TITLE" },
	{ "OF1A1", "#OF1A3TITLE" },
	{ "OF1A2", "#OF1A3TITLE" },
	{ "OF1A3", "#OF1A3TITLE" },
	{ "OF1A4", "#OF1A3TITLE" },
	{ "OF1A", "#OF1A5TITLE" },
	{ "OF2A1", "#OF2A1TITLE" },
	{ "OF2A2", "#OF2A1TITLE" },
	{ "OF2A3", "#OF2A1TITLE" },
	{ "OF2A", "#OF2A4TITLE" },
	{ "OF3A1", "#OF3A1TITLE" },
	{ "OF3A2", "#OF3A1TITLE" },
	{ "OF3A", "#OF3A3TITLE" },
	{ "OF4A1", "#OF4A1TITLE" },
	{ "OF4A2", "#OF4A1TITLE" },
	{ "OF4A3", "#OF4A1TITLE" },
	{ "OF4A", "#OF4A4TITLE" },
	{ "OF5A", "#OF5A1TITLE" },
	{ "OF6A1", "#OF6A1TITLE" },
	{ "OF6A2", "#OF6A1TITLE" },
	{ "OF6A3", "#OF6A1TITLE" },
	{ "OF6A4b", "#OF6A4TITLE" },
	{ "OF6A4", "#OF6A4TITLE" },
	{ "OF6A5", "#OF6A4TITLE" },
	{ "OF6A", "#OF6A4TITLE" },
	{ "OF7A", "#OF7A0TITLE" },
	{ "ba_tram", "#BA_TRAMTITLE" },
	{ "ba_security", "#BA_SECURITYTITLE" },
	{ "ba_main", "#BA_SECURITYTITLE" },
	{ "ba_elevator", "#BA_SECURITYTITLE" },
	{ "ba_canal", "#BA_CANALSTITLE" },
	{ "ba_yard", "#BA_YARDTITLE" },
	{ "ba_xen", "#BA_XENTITLE" },
	{ "ba_hazard", "#BA_HAZARD" },
	{ "ba_power", "#BA_POWERTITLE" },
	{ "ba_teleport1", "#BA_POWERTITLE" },
	{ "ba_teleport", "#BA_TELEPORTTITLE" },
	{ "ba_outro", "#BA_OUTRO" },
};

/*
=============
SV_SaveGameComment
Exporting this allows to replace the engine function.
=============
*/
extern "C" EXPORT void SV_SaveGameComment( char *text, int maxlength )
{
	const char *pName = NULL;

	text[0] = '\0'; // clear

	unsigned long i;
	const char *mapname = STRING( gpGlobals->mapname );

	bool justMapName = false;

	for( i = 0; i < ARRAYSIZE( gTitleComments ); i++ )
	{
		// compare if strings are equal at beginning
		int len = strlen( gTitleComments[i].mapname );
		if( !strnicmp( mapname, gTitleComments[i].mapname, len ))
		{
			pName = gTitleComments[i].titlename;
			break;
		}
	}

	if( !pName )
	{
		entvars_t *pevWorld = VARS( INDEXENT( 0 ) );

		if( pevWorld->message != 0 )
		{
			// trying to extract message from the world
			pName = STRING( pevWorld->message );
		}
		else
		{
			// or use mapname
			pName = STRING( gpGlobals->mapname );
			justMapName = true;
		}
	}

	char buf[65];
	if (!justMapName)
	{
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(NULL);
		if (pPlayer)
		{
			const char* keyVal = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "cl_save_mapname" );
			if (keyVal && *keyVal)
			{
				const int saveMapName = atoi(keyVal);
				if (saveMapName)
				{
					_snprintf(buf, sizeof(buf), "%s [%s]", pName, STRING(gpGlobals->mapname));
					pName = buf;
				}
			}
		}
	}

	_snprintf( text, maxlength, "%-64.64s %02d:%02d", pName, (int)(gpGlobals->time / 60.0 ), (int)fmod( gpGlobals->time, 60.0 ));
}
