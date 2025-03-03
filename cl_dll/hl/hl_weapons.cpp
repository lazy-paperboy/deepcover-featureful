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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

#include "usercmd.h"
#include "entity_state.h"
#include "demo_api.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"

#include "../hud_iface.h"
#include "../com_weapons.h"
#include "../demo.h"
#include "mod_features.h"

extern globalvars_t *gpGlobals;
extern int g_iUser1;

// Pool of client side entities/entvars_t
static entvars_t ev[MAX_WEAPONS];
static int num_ents = 0;

// The entity we'll use to represent the local client
static CBasePlayer player;

// Local version of game .dll global variables ( time, etc. )
static globalvars_t Globals; 

static CBasePlayerWeapon *g_pWpns[MAX_WEAPONS];
float g_flApplyVel = 0.0;
int g_irunninggausspred = 0;

Vector g_vPlayerVelocity;

Vector previousorigin;

// HLDM Weapon placeholder entities.
CGlock g_Glock;
CCrowbar g_Crowbar;
CPython g_Python;
CMP5 g_Mp5;
CCrossbow g_Crossbow;
CShotgun g_Shotgun;
CRpg g_Rpg;
CGauss g_Gauss;
CEgon g_Egon;
CHgun g_HGun;
CHandGrenade g_HandGren;
CSatchel g_Satchel;
CTripmine g_Tripmine;
CSqueak g_Snark;
#if FEATURE_DESERT_EAGLE
CEagle g_Eagle;
#endif
#if FEATURE_PIPEWRENCH
CPipeWrench g_PipeWrench;
#endif
#if FEATURE_KNIFE
CKnife g_Knife;
#endif
#if FEATURE_PENGUIN
CPenguin g_Penguin;
#endif
#if FEATURE_M249
CM249 g_M249;
#endif
#if FEATURE_SNIPERRIFLE
CSniperrifle g_Sniper;
#endif
#if FEATURE_DISPLACER
CDisplacer g_Displacer;
#endif
#if FEATURE_SHOCKRIFLE
CShockrifle g_Shock;
#endif
#if FEATURE_SPORELAUNCHER
CSporelauncher g_Spore;
#endif
#if FEATURE_GRAPPLE
CBarnacleGrapple g_Grapple;
#endif
#if FEATURE_MEDKIT
CMedkit g_Medkit;
#endif
#if FEATURE_UZI
CUzi g_Uzi;
#endif

/*
======================
AlertMessage

Print debug messages to console
======================
*/
void AlertMessage( ALERT_TYPE atype, const char *szFmt, ... )
{
	va_list argptr;
	static char string[1024];

	va_start( argptr, szFmt );
	vsprintf( string, szFmt, argptr );
	va_end( argptr );

	gEngfuncs.Con_Printf( "cl:  " );
	gEngfuncs.Con_Printf( string );
}

//Returns if it's multiplayer.
//Mostly used by the client side weapons.
bool bIsMultiplayer( void )
{
	return gEngfuncs.GetMaxClients() == 1 ? 0 : 1;
}

//Just loads a v_ model.
void LoadVModel( const char *szViewModel, CBasePlayer *m_pPlayer )
{
	gEngfuncs.CL_LoadModel( szViewModel, &m_pPlayer->pev->viewmodel );
}

/*
=====================
HUD_PrepEntity

Links the raw entity to an entvars_s holder.  If a player is passed in as the owner, then
we set up the m_pPlayer field.
=====================
*/
void HUD_PrepEntity( CBaseEntity *pEntity, CBasePlayer *pWeaponOwner )
{
	memset( &ev[num_ents], 0, sizeof(entvars_t) );
	pEntity->pev = &ev[num_ents++];

	pEntity->Precache();
	pEntity->Spawn();

	if( pWeaponOwner )
	{
		CBasePlayerWeapon* pWeapon = (CBasePlayerWeapon *)pEntity;
		pWeapon->m_pPlayer = pWeaponOwner;

		if (pWeapon->m_iId == WEAPON_NONE)
			gEngfuncs.Con_Printf("Got 0 as weapon id!\n");
		g_pWpns[pWeapon->m_iId] = (CBasePlayerWeapon *)pEntity;
	}
}

/*
=====================
CBaseEntity::Killed

If weapons code "kills" an entity, just set its effects to EF_NODRAW
=====================
*/
void CBaseEntity::Killed( entvars_t * pevInclictor, entvars_t *pevAttacker, int iGib )
{
	pev->effects |= EF_NODRAW;
}

/*
=====================
CBasePlayerWeapon::DefaultDeploy

=====================
*/
BOOL CBasePlayerWeapon::DefaultDeploy( const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt, int body )
{
	if( !CanDeploy() )
		return FALSE;

	gEngfuncs.CL_LoadModel( szViewModel, &m_pPlayer->pev->viewmodel );

	SendWeaponAnim( iAnim, body );

	g_irunninggausspred = false;
	m_pPlayer->m_flNextAttack = 0.5f;
	m_flTimeWeaponIdle = 1.0f;
	return TRUE;
}

/*
=====================
CBasePlayerWeapon::PlayEmptySound

=====================
*/
BOOL CBasePlayerWeapon::PlayEmptySound( void )
{
	if( m_iPlayEmptySound )
	{
		HUD_PlaySound( "weapons/357_cock1.wav", 0.8f );
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}


/*
=====================
CBasePlayerWeapon::Holster

Put away weapon
=====================
*/
void CBasePlayerWeapon::Holster()
{ 
	m_fInReload = FALSE; // cancel any reload in progress.
	g_irunninggausspred = false;
	m_pPlayer->pev->viewmodel = 0; 
}

/*
=====================
CBasePlayerWeapon::SendWeaponAnim

Animate weapon model
=====================
*/
void CBasePlayerWeapon::SendWeaponAnim( int iAnim, int body )
{
	m_pPlayer->pev->weaponanim = iAnim;

	HUD_SendWeaponAnim( iAnim, body, 0 );
}

/*
=====================
CBaseEntity::FireBulletsPlayer

Only produces random numbers to match the server ones.
=====================
*/
Vector CBaseEntity::FireBulletsPlayer ( ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker, int shared_rand )
{
	float x = 0.0f, y = 0.0f, z;

	for( ULONG iShot = 1; iShot <= cShots; iShot++ )
	{
		if( pevAttacker == NULL )
		{
			// get circular gaussian spread
			do {
					x = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
					y = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
					z = x * x + y * y;
			} while( z > 1 );
		}
		else
		{
			//Use player's random seed.
			// get circular gaussian spread
			x = UTIL_SharedRandomFloat( shared_rand + iShot, -0.5f, 0.5f ) + UTIL_SharedRandomFloat( shared_rand + ( 1 + iShot ) , -0.5f, 0.5f );
			y = UTIL_SharedRandomFloat( shared_rand + ( 2 + iShot ), -0.5f, 0.5f ) + UTIL_SharedRandomFloat( shared_rand + ( 3 + iShot ), -0.5f, 0.5f );
			// z = x * x + y * y;
		}			
	}

	return Vector( x * vecSpread.x, y * vecSpread.y, 0.0f );
}

/*
=====================
CBasePlayer::SelectItem

  Switch weapons
=====================
*/
void CBasePlayer::SelectItem( const char *pstr )
{
	if( !pstr )
		return;

	CBasePlayerWeapon *pItem = NULL;

	if( !pItem )
		return;

	if( pItem == m_pActiveItem )
		return;

	if( m_pActiveItem )
		m_pActiveItem->Holster();

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if( m_pActiveItem )
	{
		m_pActiveItem->Deploy();
	}
}

/*
=====================
CBasePlayer::SelectLastItem

=====================
*/
void CBasePlayer::SelectLastItem( void )
{
	if( !m_pLastItem )
	{
		return;
	}

	if( m_pActiveItem && !m_pActiveItem->CanHolster() )
	{
		return;
	}

	if( m_pActiveItem )
		m_pActiveItem->Holster();

	CBasePlayerWeapon *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;
	m_pActiveItem->Deploy( );
}

/*
=====================
CBasePlayer::Killed

=====================
*/
void CBasePlayer::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	// Holster weapon immediately, to allow it to cleanup
	if( m_pActiveItem )
		 m_pActiveItem->Holster();

	g_irunninggausspred = false;
}

/*
=====================
CBasePlayer::Spawn

=====================
*/
void CBasePlayer::Spawn( void )
{
	if( m_pActiveItem )
		m_pActiveItem->Deploy( );

	g_irunninggausspred = false;
}

/*
=====================
UTIL_TraceLine

Don't actually trace, but act like the trace didn't hit anything.
=====================
*/
void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
	memset( ptr, 0, sizeof(*ptr) );
	ptr->flFraction = 1.0f;
}

/*
=====================
UTIL_ParticleBox

For debugging, draw a box around a player made out of particles
=====================
*/
void UTIL_ParticleBox( CBasePlayer *player, float *mins, float *maxs, float life, unsigned char r, unsigned char g, unsigned char b )
{
	int i;
	Vector mmin, mmax;

	for( i = 0; i < 3; i++ )
	{
		mmin[i] = player->pev->origin[i] + mins[i];
		mmax[i] = player->pev->origin[i] + maxs[i];
	}

	gEngfuncs.pEfxAPI->R_ParticleBox( (float *)&mmin, (float *)&mmax, 5.0, 0, 255, 0 );
}

/*
=====================
UTIL_ParticleBoxes

For debugging, draw boxes for other collidable players
=====================
*/
void UTIL_ParticleBoxes( void )
{
	int idx;
	physent_t *pe;
	cl_entity_t *player;
	Vector mins, maxs;

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	player = gEngfuncs.GetLocalPlayer();
	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( player->index - 1 );	

	for( idx = 1; idx < 100; idx++ )
	{
		pe = gEngfuncs.pEventAPI->EV_GetPhysent( idx );
		if( !pe )
			break;

		if( pe->info >= 1 && pe->info <= gEngfuncs.GetMaxClients() )
		{
			mins = pe->origin + pe->mins;
			maxs = pe->origin + pe->maxs;

			gEngfuncs.pEfxAPI->R_ParticleBox( (float *)&mins, (float *)&maxs, 0, 0, 255, 2.0 );
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

/*
=====================
UTIL_ParticleLine

For debugging, draw a line made out of particles
=====================
*/
void UTIL_ParticleLine( CBasePlayer *player, float *start, float *end, float life, unsigned char r, unsigned char g, unsigned char b )
{
	gEngfuncs.pEfxAPI->R_ParticleLine( start, end, r, g, b, life );
}

/*
=====================
HUD_InitClientWeapons

Set up weapons, player and functions needed to run weapons code client-side.
=====================
*/
void HUD_InitClientWeapons( void )
{
	static int initialized = 0;
	if( initialized )
		return;

	initialized = 1;

	// Set up pointer ( dummy object )
	gpGlobals = &Globals;

	// Fill in current time ( probably not needed )
	gpGlobals->time = gEngfuncs.GetClientTime();

	// Fake functions
	g_engfuncs.pfnPrecacheModel = stub_PrecacheModel;
	g_engfuncs.pfnPrecacheSound = stub_PrecacheSound;
	g_engfuncs.pfnPrecacheEvent = stub_PrecacheEvent;
	g_engfuncs.pfnNameForFunction = stub_NameForFunction;
	g_engfuncs.pfnSetModel = stub_SetModel;
	g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;

	// Handled locally
	g_engfuncs.pfnPlaybackEvent = HUD_PlaybackEvent;
	g_engfuncs.pfnAlertMessage = AlertMessage;

	// Pass through to engine
	g_engfuncs.pfnPrecacheEvent = gEngfuncs.pfnPrecacheEvent;
	g_engfuncs.pfnRandomFloat = gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnRandomLong = gEngfuncs.pfnRandomLong;

	// Allocate a slot for the local player
	HUD_PrepEntity( &player, NULL );

	// Allocate slot(s) for each weapon that we are going to be predicting
	HUD_PrepEntity( &g_Glock, &player );
	HUD_PrepEntity( &g_Crowbar, &player );
	HUD_PrepEntity( &g_Python, &player );
	HUD_PrepEntity( &g_Mp5, &player );
	HUD_PrepEntity( &g_Crossbow, &player );
	HUD_PrepEntity( &g_Shotgun, &player );
	HUD_PrepEntity( &g_Rpg, &player );
	HUD_PrepEntity( &g_Gauss, &player );
	HUD_PrepEntity( &g_Egon, &player );
	HUD_PrepEntity( &g_HGun, &player );
	HUD_PrepEntity( &g_HandGren, &player );
	HUD_PrepEntity( &g_Satchel, &player );
	HUD_PrepEntity( &g_Tripmine, &player );
	HUD_PrepEntity( &g_Snark, &player );
#if FEATURE_DESERT_EAGLE
	HUD_PrepEntity( &g_Eagle, &player );
#endif
#if FEATURE_PIPEWRENCH
	HUD_PrepEntity( &g_PipeWrench, &player );
#endif
#if FEATURE_KNIFE
	HUD_PrepEntity( &g_Knife, &player );
#endif
#if FEATURE_PENGUIN
	HUD_PrepEntity( &g_Penguin, &player );
#endif
#if FEATURE_M249
	HUD_PrepEntity( &g_M249, &player );
#endif
#if FEATURE_SNIPERRIFLE
	HUD_PrepEntity( &g_Sniper, &player );
#endif
#if FEATURE_DISPLACER
	HUD_PrepEntity( &g_Displacer, &player );
#endif
#if FEATURE_SHOCKRIFLE
	HUD_PrepEntity( &g_Shock, &player );
#endif
#if FEATURE_SPORELAUNCHER
	HUD_PrepEntity( &g_Spore, &player );
#endif
#if FEATURE_GRAPPLE
	HUD_PrepEntity( &g_Grapple, &player );
#endif
#if FEATURE_MEDKIT
	HUD_PrepEntity( &g_Medkit, &player );
#endif
#if FEATURE_UZI
	HUD_PrepEntity( &g_Uzi, &player );
#endif
}

/*
=====================
HUD_GetLastOrg

Retruns the last position that we stored for egon beam endpoint.
=====================
*/
void HUD_GetLastOrg( float *org )
{
	int i;

	// Return last origin
	for( i = 0; i < 3; i++ )
	{
		org[i] = previousorigin[i];
	}
}

/*
=====================
HUD_SetLastOrg

Remember our exact predicted origin so we can draw the egon to the right position.
=====================
*/
void HUD_SetLastOrg( void )
{
	int i;

	// Offset final origin by view_offset
	for( i = 0; i < 3; i++ )
	{
		previousorigin[i] = g_finalstate->playerstate.origin[i] + g_finalstate->client.view_ofs[i];
	}
}

/*
=====================
HUD_WeaponsPostThink

Run Weapon firing code on client
=====================
*/
void HUD_WeaponsPostThink( local_state_s *from, local_state_s *to, usercmd_t *cmd, double time, unsigned int random_seed )
{
	int i;
	int buttonsChanged;
	CBasePlayerWeapon *pWeapon = NULL;
	CBasePlayerWeapon *pCurrent;
	weapon_data_t nulldata = {0}, *pfrom, *pto;
	static int lasthealth;

	// Get current clock
	gpGlobals->time = gEngfuncs.GetClientTime();

	// Fill in data based on selected weapon
	// FIXME, make this a method in each weapon?  where you pass in an entity_state_t *?
	switch( from->client.m_iId )
	{
		case WEAPON_CROWBAR:
			pWeapon = &g_Crowbar;
			break;
		case WEAPON_GLOCK:
			pWeapon = &g_Glock;
			break;
		case WEAPON_PYTHON:
			pWeapon = &g_Python;
			break;
		case WEAPON_MP5:
			pWeapon = &g_Mp5;
			break;
		case WEAPON_CROSSBOW:
			pWeapon = &g_Crossbow;
			break;
		case WEAPON_SHOTGUN:
			pWeapon = &g_Shotgun;
			break;
		case WEAPON_RPG:
			pWeapon = &g_Rpg;
			break;
		case WEAPON_GAUSS:
			pWeapon = &g_Gauss;
			break;
		case WEAPON_EGON:
			pWeapon = &g_Egon;
			break;
		case WEAPON_HORNETGUN:
			pWeapon = &g_HGun;
			break;
		case WEAPON_HANDGRENADE:
			pWeapon = &g_HandGren;
			break;
		case WEAPON_SATCHEL:
			pWeapon = &g_Satchel;
			break;
		case WEAPON_TRIPMINE:
			pWeapon = &g_Tripmine;
			break;
		case WEAPON_SNARK:
			pWeapon = &g_Snark;
			break;
#if FEATURE_DESERT_EAGLE
		case WEAPON_EAGLE:
			pWeapon = &g_Eagle;
			break;
#endif
#if FEATURE_PIPEWRENCH
		case WEAPON_PIPEWRENCH:
			pWeapon = &g_PipeWrench;
			break;
#endif
#if FEATURE_KNIFE
		case WEAPON_KNIFE:
			pWeapon = &g_Knife;
			break;
#endif
#if FEATURE_PENGUIN
		case WEAPON_PENGUIN:
			pWeapon = &g_Penguin;
			break;
#endif
#if FEATURE_M249
		case WEAPON_M249:
			pWeapon = &g_M249;
			break;
#endif
#if FEATURE_SNIPERRIFLE
		case WEAPON_SNIPERRIFLE:
			pWeapon = &g_Sniper;
			break;
#endif
#if FEATURE_DISPLACER
		case WEAPON_DISPLACER:
			pWeapon = &g_Displacer;
			break;
#endif
#if FEATURE_SHOCKRIFLE
	case WEAPON_SHOCKRIFLE:
			pWeapon = &g_Shock;
			break;
#endif
#if FEATURE_SPORELAUNCHER
	case WEAPON_SPORELAUNCHER:
			pWeapon = &g_Spore;
			break;
#endif
#if FEATURE_GRAPPLE
	case WEAPON_GRAPPLE:
			pWeapon = &g_Grapple;
			break;
#endif
#if FEATURE_MEDKIT
		case WEAPON_MEDKIT:
			pWeapon = &g_Medkit;
			break;
#endif
#if FEATURE_UZI
		case WEAPON_UZI:
			pWeapon = &g_Uzi;
			break;
#endif
	}

	// Store pointer to our destination entity_state_t so we can get our origin, etc. from it
	//  for setting up events on the client
	g_finalstate = to;

	// If we are running events/etc. go ahead and see if we
	//  managed to die between last frame and this one
	// If so, run the appropriate player killed or spawn function
	if( g_runfuncs )
	{
		if( to->client.health <= 0 && lasthealth > 0 )
		{
			player.Killed( NULL, NULL, 0 );
		}
		else if( to->client.health > 0 && lasthealth <= 0 )
		{
			player.Spawn();
		}

		lasthealth = to->client.health;
	}

	// We are not predicting the current weapon, just bow out here.
	if( !pWeapon )
		return;

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		pCurrent = g_pWpns[i];
		if( !pCurrent )
		{
			continue;
		}

		pfrom = &from->weapondata[i];

		pCurrent->m_fInReload = pfrom->m_fInReload;
		pCurrent->m_fInSpecialReload = pfrom->m_fInSpecialReload;
		//pCurrent->m_flPumpTime = pfrom->m_flPumpTime;
		pCurrent->m_iClip = pfrom->m_iClip;
		pCurrent->m_flNextPrimaryAttack	= pfrom->m_flNextPrimaryAttack;
		pCurrent->m_flNextSecondaryAttack = pfrom->m_flNextSecondaryAttack;
		pCurrent->m_flTimeWeaponIdle = pfrom->m_flTimeWeaponIdle;
		pCurrent->pev->fuser1 = pfrom->fuser1;

		pCurrent->m_iSecondaryAmmoType = (int)from->client.vuser3[2];
		pCurrent->m_iPrimaryAmmoType = (int)from->client.vuser4[0];
		player.m_rgAmmo[pCurrent->m_iPrimaryAmmoType] = (int)from->client.vuser4[1];
		player.m_rgAmmo[pCurrent->m_iSecondaryAmmoType] = (int)from->client.vuser4[2];

		pCurrent->SetWeaponData(*pfrom);
	}

	// For random weapon events, use this seed to seed random # generator
	player.random_seed = random_seed;

	// Get old buttons from previous state.
	player.m_afButtonLast = from->playerstate.oldbuttons;

	// Which buttsons chave changed
	buttonsChanged = ( player.m_afButtonLast ^ cmd->buttons );	// These buttons have changed this frame

	// Debounced button codes for pressed/released
	// The changed ones still down are "pressed"
	player.m_afButtonPressed =  buttonsChanged & cmd->buttons;	
	// The ones not down are "released"
	player.m_afButtonReleased = buttonsChanged & ( ~cmd->buttons );

	// Set player variables that weapons code might check/alter
	player.pev->button = cmd->buttons;

	player.pev->velocity = from->client.velocity;
	player.pev->flags = from->client.flags;

	player.pev->deadflag = from->client.deadflag;
	player.pev->waterlevel = from->client.waterlevel;
	player.pev->maxspeed = from->client.maxspeed;
	player.pev->fov = from->client.fov;
	player.pev->weaponanim = from->client.weaponanim;
	player.pev->viewmodel = from->client.viewmodel;
	player.m_flNextAttack = from->client.m_flNextAttack;
	player.m_flNextAmmoBurn = from->client.fuser2;
	player.m_flAmmoStartCharge = from->client.fuser3;

	g_vPlayerVelocity = player.pev->velocity;

	// Point to current weapon object
	if( from->client.m_iId )
	{
		player.m_pActiveItem = g_pWpns[from->client.m_iId];
	}

	player.m_suppressedCapabilities = from->client.vuser2[0];

	// Don't go firing anything if we have died.
	// Or if we don't have a weapon model deployed
	if( ( player.pev->deadflag != ( DEAD_DISCARDBODY + 1 ) ) && 
		 !CL_IsDead() && player.pev->viewmodel && !g_iUser1 )
	{
		if( player.m_flNextAttack <= 0 )
		{
			pWeapon->ItemPostFrame();
		}
	}

	// Assume that we are not going to switch weapons
	to->client.m_iId = from->client.m_iId;

	// Now see if we issued a changeweapon command ( and we're not dead )
	if( cmd->weaponselect && ( player.pev->deadflag != ( DEAD_DISCARDBODY + 1 ) ) )
	{
		// Switched to a different weapon?
		if( from->weapondata[cmd->weaponselect].m_iId == cmd->weaponselect )
		{
			CBasePlayerWeapon *pNew = g_pWpns[cmd->weaponselect];
			if( pNew && ( pNew != pWeapon ) )
			{
				// Put away old weapon
				if( player.m_pActiveItem )
					player.m_pActiveItem->Holster();

				player.m_pLastItem = player.m_pActiveItem;
				player.m_pActiveItem = pNew;

				// Deploy new weapon
				if( player.m_pActiveItem )
				{
					player.m_pActiveItem->Deploy();
				}

				// Update weapon id so we can predict things correctly.
				to->client.m_iId = cmd->weaponselect;
			}
		}
	}

	// Copy in results of prediction code
	to->client.viewmodel = player.pev->viewmodel;
	to->client.fov = player.pev->fov;
	to->client.weaponanim = player.pev->weaponanim;
	to->client.m_flNextAttack = player.m_flNextAttack;
	to->client.fuser2 = player.m_flNextAmmoBurn;
	to->client.fuser3 = player.m_flAmmoStartCharge;
	to->client.maxspeed = player.pev->maxspeed;

	// Make sure that weapon animation matches what the game .dll is telling us
	//  over the wire ( fixes some animation glitches )
	if( g_runfuncs && ( HUD_GetWeaponAnim() != to->client.weaponanim ) )
	{
		int body = 0;

		//Show laser sight/scope combo
		if( pWeapon == &g_Python && bIsMultiplayer() )
			 body = 1;

#if FEATURE_M249
		if (pWeapon == &g_M249) {
			body = g_M249.BodyFromClip();
		}
#endif

		// Force a fixed anim down to viewmodel
		HUD_SendWeaponAnim( to->client.weaponanim, body, 1 );
	}

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		pCurrent = g_pWpns[i];

		pto = &to->weapondata[i];

		if( !pCurrent )
		{
			memset( pto, 0, sizeof(weapon_data_t) );
			continue;
		}

		pto->m_fInReload = pCurrent->m_fInReload;
		pto->m_fInSpecialReload = pCurrent->m_fInSpecialReload;
		//pto->m_flPumpTime = pCurrent->m_flPumpTime;
		pto->m_iClip = pCurrent->m_iClip; 
		pto->m_flNextPrimaryAttack = pCurrent->m_flNextPrimaryAttack;
		pto->m_flNextSecondaryAttack = pCurrent->m_flNextSecondaryAttack;
		pto->m_flTimeWeaponIdle = pCurrent->m_flTimeWeaponIdle;
		pto->fuser1 = pCurrent->pev->fuser1;

		// Decrement weapon counters, server does this at same time ( during post think, after doing everything else )
		pto->m_flNextReload -= cmd->msec / 1000.0f;
		pto->m_fNextAimBonus -= cmd->msec / 1000.0f;
		pto->m_flNextPrimaryAttack -= cmd->msec / 1000.0f;
		pto->m_flNextSecondaryAttack -= cmd->msec / 1000.0f;
		pto->m_flTimeWeaponIdle -= cmd->msec / 1000.0f;
		pto->fuser1 -= cmd->msec / 1000.0f;

		to->client.vuser3[2] = pCurrent->m_iSecondaryAmmoType;
		to->client.vuser4[0] = pCurrent->m_iPrimaryAmmoType;
		to->client.vuser4[1] = player.m_rgAmmo[pCurrent->m_iPrimaryAmmoType];
		to->client.vuser4[2] = player.m_rgAmmo[pCurrent->m_iSecondaryAmmoType];

		pCurrent->GetWeaponData(*pto);

/*		if( pto->m_flPumpTime != -9999.0f )
		{
			pto->m_flPumpTime -= cmd->msec / 1000.0f;
			if( pto->m_flPumpTime < -0.001f )
				pto->m_flPumpTime = -0.001f;
		}*/

		if( pto->m_fNextAimBonus < -1.0f )
		{
			pto->m_fNextAimBonus = -1.0f;
		}

		if( pto->m_flNextPrimaryAttack < -1.0f )
		{
			pto->m_flNextPrimaryAttack = -1.0f;
		}

		if( pto->m_flNextSecondaryAttack < -0.001f )
		{
			pto->m_flNextSecondaryAttack = -0.001f;
		}

		if( pto->m_flTimeWeaponIdle < -0.001f )
		{
			pto->m_flTimeWeaponIdle = -0.001f;
		}

		if( pto->m_flNextReload < -0.001f )
		{
			pto->m_flNextReload = -0.001f;
		}

		if( pto->fuser1 < -0.001f )
		{
			pto->fuser1 = -0.001f;
		}
	}

	// m_flNextAttack is now part of the weapons, but is part of the player instead
	to->client.m_flNextAttack -= cmd->msec / 1000.0f;
	if( to->client.m_flNextAttack < -0.001f )
	{
		to->client.m_flNextAttack = -0.001f;
	}

	to->client.fuser2 -= cmd->msec / 1000.0f;
	if( to->client.fuser2 < -0.001f )
	{
		to->client.fuser2 = -0.001f;
	}

	to->client.fuser3 -= cmd->msec / 1000.0f;
	if( to->client.fuser3 < -0.001f )
	{
		to->client.fuser3 = -0.001f;
	}

	// Store off the last position from the predicted state.
	HUD_SetLastOrg();

	// Wipe it so we can't use it after this frame
	g_finalstate = NULL;
}

/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
void _DLLEXPORT HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed )
{
	g_runfuncs = runfuncs;

	//Event code depends on this stuff, so always initialize it.
	HUD_InitClientWeapons();

#if CLIENT_WEAPONS
	if( cl_lw && cl_lw->value )
	{
		HUD_WeaponsPostThink( from, to, cmd, time, random_seed );
	}
	else
#endif
	{
		to->client.fov = g_lastFOV;
	}

	if( g_irunninggausspred == 1 )
	{
		Vector forward;
		gEngfuncs.pfnAngleVectors( v_angles, forward, NULL, NULL );
		to->client.velocity = to->client.velocity - forward * g_flApplyVel * 5; 
		g_irunninggausspred = false;
	}

	// All games can use FOV state
	g_lastFOV = to->client.fov;
}
