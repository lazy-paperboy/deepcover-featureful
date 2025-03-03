/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "gamerules.h"
#include "player.h"
#include "skill.h"
#include "nodes.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"
#ifndef CLIENT_DLL
#include "game.h"
#endif

#if FEATURE_GRAPPLE

void FindHullIntersection(const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity);

#ifndef CLIENT_DLL
#include "grapple_tonguetip.h"

LINK_ENTITY_TO_CLASS( grapple_tip, CBarnacleGrappleTip );

void CBarnacleGrappleTip::Precache()
{
	PRECACHE_MODEL( "models/shock_effect.mdl" );
}

void CBarnacleGrappleTip::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT(pev), "models/shock_effect.mdl" );

	UTIL_SetSize( pev, Vector(0, 0, 0), Vector(0, 0, 0) );

	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CBarnacleGrappleTip::FlyThink );
	SetTouch( &CBarnacleGrappleTip::TongueTouch );

	Vector vecAngles = pev->angles;

	vecAngles.x -= 30.0;

	pev->angles = vecAngles;

	UTIL_MakeVectors( pev->angles );

	vecAngles.x = -( 30.0 + vecAngles.x );

	pev->velocity = g_vecZero;

	pev->gravity = 1.0;

	pev->nextthink = gpGlobals->time + 0.02;

	m_bIsStuck = FALSE;
	m_bMissed = FALSE;
}

void CBarnacleGrappleTip::FlyThink()
{
	UTIL_MakeAimVectors( pev->angles );

	pev->angles = UTIL_VecToAngles( gpGlobals->v_forward );

	const float flNewVel = ( ( pev->velocity.Length() * 0.8 ) + 400.0 );

	pev->velocity = pev->velocity * 0.2 + ( flNewVel * gpGlobals->v_forward );

	if( !g_pGameRules->IsMultiplayer() )
	{
		//Note: the old grapple had a maximum velocity of 1600. - Solokiller
		if( pev->velocity.Length() > 750.0 )
		{
			pev->velocity = pev->velocity.Normalize() * 750.0;
		}
	}
	else
	{
		//TODO: should probably clamp at sv_maxvelocity to prevent the tip from going off course. - Solokiller
		if( pev->velocity.Length() > 2000.0 )
		{
			pev->velocity = pev->velocity.Normalize() * 2000.0;
		}
	}

	pev->nextthink = gpGlobals->time + 0.02;
}

void CBarnacleGrappleTip::OffsetThink()
{
	//Nothing
}

void CBarnacleGrappleTip::TongueTouch( CBaseEntity* pOther )
{
	if( !pOther )
	{
		targetClass = GRAPPLE_NOT_A_TARGET;
		m_bMissed = TRUE;
	}
	else
	{
		if( pOther->IsPlayer() )
		{
			targetClass = GRAPPLE_MEDIUM;

			m_hGrappleTarget = pOther;

			m_bIsStuck = TRUE;
		}
		else
		{
			targetClass = CheckTarget( pOther );

			if( targetClass != GRAPPLE_NOT_A_TARGET )
			{
				m_bIsStuck = TRUE;
			}
			else
			{
				m_bMissed = TRUE;
			}
		}
	}

	pev->velocity = g_vecZero;

	m_GrappleType = targetClass;

	SetThink( &CBarnacleGrappleTip::OffsetThink );
	pev->nextthink = gpGlobals->time + 0.02;

	SetTouch( NULL );
}

int CBarnacleGrappleTip::CheckTarget( CBaseEntity* pTarget )
{
	if( !pTarget )
		return GRAPPLE_NOT_A_TARGET;

	if( pTarget->IsPlayer() )
	{
		m_hGrappleTarget = pTarget;

		return pTarget->SizeForGrapple();
	}

	Vector vecStart = pev->origin;
	Vector vecEnd = pev->origin + pev->velocity * 1024.0;

	TraceResult tr;

	UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, edict(), &tr );

	CBaseEntity* pHit = Instance( tr.pHit );

/*	if( !pHit )
		pHit = CWorld::GetInstance();*/

	float rgfl1[3];
	float rgfl2[3];
	const char *pTexture;

	vecStart.CopyToArray(rgfl1);
	vecEnd.CopyToArray(rgfl2);

	if (pHit)
		pTexture = TRACE_TEXTURE(ENT(pHit->pev), rgfl1, rgfl2);
	else
		pTexture = TRACE_TEXTURE(ENT(0), rgfl1, rgfl2);

	bool bIsFixed = false;

	if( pTexture && strnicmp( pTexture, "xeno_grapple", 12 ) == 0 )
	{
		bIsFixed = true;
	}
	else if (pTarget->SizeForGrapple() != GRAPPLE_NOT_A_TARGET)
	{
		if (pTarget->SizeForGrapple() == GRAPPLE_FIXED) {
			bIsFixed = true;
		} else {
			m_hGrappleTarget = pTarget;
			m_vecOriginOffset = pev->origin - pTarget->pev->origin;
			return pTarget->SizeForGrapple();
		}
	}

	if( bIsFixed )
	{
		m_hGrappleTarget = pTarget;
		m_vecOriginOffset = g_vecZero;

		return GRAPPLE_FIXED;
	}

	return GRAPPLE_NOT_A_TARGET;
}

void CBarnacleGrappleTip::SetPosition( Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner )
{
	UTIL_SetOrigin( pev, vecOrigin );
	pev->angles = vecAngles;
	pev->owner = pOwner->edict();
}
#endif
enum BarnacleGrappleAnim
{
	BGRAPPLE_BREATHE = 0,
	BGRAPPLE_LONGIDLE,
	BGRAPPLE_SHORTIDLE,
	BGRAPPLE_COUGH,
	BGRAPPLE_DOWN,
	BGRAPPLE_UP,
	BGRAPPLE_FIRE,
	BGRAPPLE_FIREWAITING,
	BGRAPPLE_FIREREACHED,
	BGRAPPLE_FIRETRAVEL,
	BGRAPPLE_FIRERELEASE
};

LINK_ENTITY_TO_CLASS( weapon_grapple, CBarnacleGrapple )

void CBarnacleGrapple::Precache( void )
{
	PRECACHE_MODEL( "models/v_bgrap.mdl" );
	PRECACHE_MODEL( MyWModel() );
	PRECACHE_MODEL( "models/p_bgrap.mdl" );

	PRECACHE_SOUND( "weapons/bgrapple_release.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_impact.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_fire.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_cough.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_pull.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_wait.wav" );
	PRECACHE_SOUND( "weapons/alienweap_draw.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew1.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew2.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew3.wav" );

	PRECACHE_MODEL( "sprites/tongue.spr" );

	UTIL_PrecacheOther( "grapple_tip" );
}

void CBarnacleGrapple::Spawn( void )
{
	Precache();
	m_iId = WEAPON_GRAPPLE;
	SET_MODEL( ENT(pev), MyWModel() );
	m_pTip = NULL;
	m_bGrappling = FALSE;
	m_iClip = -1;

	FallInit();
}

bool CBarnacleGrapple::IsEnabledInMod()
{
#ifndef CLIENT_DLL
	return g_modFeatures.IsWeaponEnabled(WEAPON_GRAPPLE);
#else
	return true;
#endif
}

int CBarnacleGrapple::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->pszAmmo2 = NULL;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 3;
	p->iId = WEAPON_GRAPPLE;
	p->iWeight = GRAPPLE_WEIGHT;
	p->pszAmmoEntity = NULL;
	p->iDropAmmo = 0;
	return 1;
}

int CBarnacleGrapple::AddToPlayer( CBasePlayer* pPlayer )
{
	return AddToPlayerDefault(pPlayer);
}

BOOL CBarnacleGrapple::Deploy()
{
	int r = DefaultDeploy("models/v_bgrap.mdl", "models/p_bgrap.mdl", BGRAPPLE_UP, "gauss" );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.1;
	return r;
}

void CBarnacleGrapple::Holster()
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;

	if( m_fireState != OFF )
		EndAttack();

	SendWeaponAnim( BGRAPPLE_DOWN );
}

void CBarnacleGrapple::WeaponIdle( void )
{
	ResetEmptySound();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_fireState != OFF )
	{
		EndAttack();
		return;
	}

	m_bMissed = FALSE;

	const float flNextIdle = RANDOM_FLOAT( 0.0, 1.0 );

	int iAnim;

	if( flNextIdle <= 0.5 )
	{
		iAnim = BGRAPPLE_LONGIDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 10.0;
	}
	else if( flNextIdle > 0.95 )
	{
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/bgrapple_cough.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );

		iAnim = BGRAPPLE_COUGH;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.6;
	}
	else
	{
		iAnim = BGRAPPLE_BREATHE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.566;
	}

	SendWeaponAnim( iAnim );
}

void CBarnacleGrapple::PrimaryAttack( void )
{
	if( m_bMissed )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
#ifndef CLIENT_DLL
	if( m_pTip )
	{
		if( m_pTip->IsStuck() )
		{
			CBaseEntity* pTarget = m_pTip->GetGrappleTarget();

			if( !pTarget )
			{
				EndAttack();
				return;
			}

			if( m_pTip->GetGrappleType() > GRAPPLE_SMALL )
			{
				m_pPlayer->pev->movetype = MOVETYPE_FLY;
				m_pPlayer->pev->flags |= FL_IMMUNE_SLIME;
				//Tells the physics code that the player is not on a ladder - Solokiller
			}

			if( m_bMomentaryStuck )
			{
				SendWeaponAnim( BGRAPPLE_FIRETRAVEL );

				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/bgrapple_impact.wav", 0.98, ATTN_NORM, 0, 125 );

				if( pTarget->IsPlayer() )
				{
					EMIT_SOUND_DYN( ENT(pTarget->pev), CHAN_STATIC,"weapons/bgrapple_impact.wav", 0.98, ATTN_NORM, 0, 125 );
				}

				m_bMomentaryStuck = FALSE;
			}

			switch( m_pTip->GetGrappleType() )
			{
			case GRAPPLE_NOT_A_TARGET: break;

			case GRAPPLE_SMALL:
				//pTarget->BarnacleVictimGrabbed( this );
				m_pTip->pev->origin = pTarget->Center();

				pTarget->pev->velocity = pTarget->pev->velocity + ( m_pPlayer->pev->origin - pTarget->pev->origin );

				if( pTarget->pev->velocity.Length() > 450.0 )
				{
					pTarget->pev->velocity = pTarget->pev->velocity.Normalize() * 450.0;
				}

				break;

			case GRAPPLE_MEDIUM:
			case GRAPPLE_LARGE:
			case GRAPPLE_FIXED:
				//pTarget->BarnacleVictimGrabbed( this );

				if( m_pTip->GetGrappleType() != GRAPPLE_FIXED )
					UTIL_SetOrigin( m_pTip->pev, pTarget->Center() );

				m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + ( m_pTip->pev->origin - m_pPlayer->pev->origin );

				if( m_pPlayer->pev->velocity.Length() > 450.0 )
				{
					m_pPlayer->pev->velocity = m_pPlayer->pev->velocity.Normalize() * 450.0;

					Vector vecPitch = UTIL_VecToAngles( m_pPlayer->pev->velocity );

					if( (vecPitch.x > 55.0 && vecPitch.x < 205.0) || vecPitch.x < -55.0 )
					{
						m_bGrappling = FALSE;
						m_pPlayer->SetAnimation( PLAYER_IDLE );
					}
					else
					{
						if (!m_bGrappling)
							EMIT_SOUND_DYN(  ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/bgrapple_pull.wav", 0.98, ATTN_NORM, 0, 125 );
						m_bGrappling = TRUE;
						m_pPlayer->m_afPhysicsFlags |= PFLAG_LATCHING;
						m_pPlayer->SetAnimation(PLAYER_GRAPPLE);
					}
				}
				else
				{
					m_bGrappling = FALSE;
					m_pPlayer->SetAnimation( PLAYER_IDLE );
				}

				break;
			}
		}

		if( m_pTip->HasMissed() )
		{
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/bgrapple_release.wav", 0.98, ATTN_NORM, 0, 125 );

			EndAttack();
			return;
		}
	}
#endif
	if( m_fireState != OFF )
	{
		m_pPlayer->m_iWeaponVolume = 450;

		if( m_flShootTime != 0.0 && gpGlobals->time > m_flShootTime )
		{
			SendWeaponAnim( BGRAPPLE_FIREWAITING );

			Vector vecPunchAngle = m_pPlayer->pev->punchangle;

			vecPunchAngle.x += 2.0;

			m_pPlayer->pev->punchangle = vecPunchAngle;

			Fire( m_pPlayer->GetGunPosition(), gpGlobals->v_forward );
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/bgrapple_pull.wav", 0.98, ATTN_NORM, 0, 125 );
			m_flShootTime = 0;
		}
	}
	else
	{
		m_bMomentaryStuck = TRUE;

		SendWeaponAnim( BGRAPPLE_FIRE );

		m_pPlayer->m_iWeaponVolume = 450;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
#ifndef CLIENT_DLL
		if( g_pGameRules->IsMultiplayer() )
		{
			m_flShootTime = gpGlobals->time;
		}
		else
		{
			m_flShootTime = gpGlobals->time + 0.35;
		}
#endif
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/bgrapple_fire.wav", 0.98, ATTN_NORM, 0, 125 );
		m_fireState = CHARGE;
	}

	if( !m_pTip )
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;
		return;
	}

#ifndef CLIENT_DLL
	if( m_pTip->GetGrappleType() != GRAPPLE_FIXED && m_pTip->IsStuck() )
	{
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );

		Vector vecSrc = m_pPlayer->GetGunPosition();

		Vector vecEnd = vecSrc + gpGlobals->v_forward * 16.0;

		TraceResult tr;

		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr );

		if( tr.flFraction >= 1.0 )
		{
			UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &tr );
			if( tr.flFraction < 1.0 )
			{
				if (!tr.pHit || FNullEnt(tr.pHit) || ((CBaseEntity*)GET_PRIVATE(tr.pHit))->IsBSPModel())
				{
					FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
				}
			}
		}

		if( tr.flFraction < 1.0 )
		{
			CBaseEntity* pHit = Instance( tr.pHit );

			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			if( pHit )
			{
				if( m_pTip )
				{
					bool bValidTarget = FALSE;
#ifndef CLIENT_DLL
					if( pHit->IsPlayer() )
					{
						m_pTip->SetGrappleTarget( pHit );
						bValidTarget = TRUE;
					}
					else if( m_pTip->CheckTarget( pHit ) != GRAPPLE_NOT_A_TARGET )
					{
						bValidTarget = TRUE;
					}
#endif
					if( bValidTarget )
					{
						if( m_flDamageTime + 0.5 < gpGlobals->time )
						{
#ifndef CLIENT_DLL
							ClearMultiDamage();

							float flDamage = gSkillData.plrDmgGrapple;

							if( g_pGameRules->IsMultiplayer() )
							{
								flDamage *= 2;
							}

							pHit->TraceAttack( m_pPlayer->pev, m_pPlayer->pev, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB );

							ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
#endif

							m_flDamageTime = gpGlobals->time;

							const char* pszSample;

							switch( RANDOM_LONG( 0, 2 ) )
							{
							default:
							case 0: pszSample = "barnacle/bcl_chew1.wav"; break;
							case 1: pszSample = "barnacle/bcl_chew2.wav"; break;
							case 2: pszSample = "barnacle/bcl_chew3.wav"; break;
							}
							EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, pszSample, VOL_NORM, ATTN_NORM, 0, 125 );
						}
					}
				}
			}
		}
	}
#endif

	//TODO: CTF support - Solokiller
	/*
	if( g_pGameRules->IsMultiplayer() && g_pGameRules->IsCTF() )
	{
		m_flNextPrimaryAttack = gpGlobals->time;
	}
	else
	*/
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;
	}
}

void CBarnacleGrapple::Fire( Vector vecOrigin, Vector vecDir )
{
#ifndef CLIENT_DLL
	Vector vecSrc = vecOrigin;

	Vector vecEnd = vecSrc + vecDir * 2048.0;

	TraceResult tr;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr );

	if( !tr.fAllSolid )
	{
		CBaseEntity* pHit = Instance( tr.pHit );
/*
		if( !pHit )
			pHit = CWorld::GetInstance();
*/
		if( pHit )
		{
			UpdateEffect();

			m_flDamageTime = gpGlobals->time;
		}
	}
#endif
}

void CBarnacleGrapple::EndAttack( void )
{
	m_fireState = OFF;
	SendWeaponAnim( BGRAPPLE_FIRERELEASE );

	EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/bgrapple_pull.wav", 0.0, ATTN_NONE, SND_STOP, 100 );

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/bgrapple_release.wav", 1, ATTN_NORM);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.9;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;

	DestroyEffect();

	if( m_bGrappling && m_pPlayer->IsAlive() )
	{
		m_pPlayer->SetAnimation( PLAYER_IDLE );
	}

	m_pPlayer->pev->movetype = MOVETYPE_WALK;
	m_pPlayer->pev->flags &= ~(FL_IMMUNE_SLIME);
	m_pPlayer->m_afPhysicsFlags &= ~PFLAG_LATCHING;
}

void CBarnacleGrapple::CreateEffect( void )
{
#ifndef CLIENT_DLL
	DestroyEffect();

	m_pTip = GetClassPtr((CBarnacleGrappleTip *)NULL);
	m_pTip->Spawn();

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	Vector vecOrigin =
		m_pPlayer->GetGunPosition() +
		gpGlobals->v_forward * 16.0 +
		gpGlobals->v_right * 8.0 +
		gpGlobals->v_up * -8.0;

	Vector vecAngles = m_pPlayer->pev->v_angle;

	vecAngles.x = -vecAngles.x;

	m_pTip->SetPosition( vecOrigin, vecAngles, m_pPlayer );

	if( !m_pBeam )
	{
		m_pBeam = CBeam::BeamCreate( "sprites/tongue.spr", 16 );

		m_pBeam->EntsInit( m_pTip->entindex(), m_pPlayer->entindex() );

		m_pBeam->SetFlags( BEAM_FSOLID );

		m_pBeam->SetBrightness( 100.0 );

		m_pBeam->SetEndAttachment( 1 );

		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	}
#endif
}

void CBarnacleGrapple::UpdateEffect( void )
{
#ifndef CLIENT_DLL
	if( !m_pBeam || !m_pTip )
		CreateEffect();
#endif
}

void CBarnacleGrapple::DestroyEffect( void )
{
	if( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}

#ifndef CLIENT_DLL
	if( m_pTip )
	{
		m_pTip->Killed( NULL, NULL, GIB_NEVER );
		m_pTip = NULL;
	}
#endif
}

#endif
