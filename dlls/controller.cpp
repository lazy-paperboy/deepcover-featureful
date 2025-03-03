/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

//=========================================================
// CONTROLLER
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"effects.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"squadmonster.h"
#include	"scripted.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	CONTROLLER_AE_HEAD_OPEN			1
#define	CONTROLLER_AE_BALL_SHOOT		2
#define	CONTROLLER_AE_SMALL_SHOOT		3
#define CONTROLLER_AE_POWERUP_FULL		4
#define CONTROLLER_AE_POWERUP_HALF		5

#define CONTROLLER_FLINCH_DELAY			2		// at most one flinch every n secs

class CController : public CSquadMonster
{
public:
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	void UpdateOnRemove();
	void SetYawSpeed( void );
	int DefaultClassify( void );
	const char* DefaultDisplayName() { return "Alien Controller"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void RunAI( void );
	BOOL CheckRangeAttack1( float flDot, float flDist );	// balls
	BOOL CheckRangeAttack2( float flDot, float flDist );	// head
	BOOL CheckMeleeAttack1( float flDot, float flDist );	// block, throw
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	CUSTOM_SCHEDULES

	void Stop( void );
	void Move( float flInterval );
	int CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );
	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void SetActivity( Activity NewActivity );
	BOOL ShouldAdvanceRoute( float flWaypointDist );
	int LookupFloat();

	float m_flNextFlinch;

	float m_flShootTime;
	float m_flShootEnd;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void DeathSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void OnDying();
	void GibMonster( void );

	bool IsDisplaceable() { return true; }
	Vector DefaultMinHullSize() { return Vector( -32.0f, -32.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 32.0f, 32.0f, 64.0f ); }

	CSprite *m_pBall[2];	// hand balls
	int m_iBall[2];		// how bright it should be
	float m_iBallTime[2];	// when it should be that color
	int m_iBallCurrent[2];	// current brightness

	Vector m_vecEstVelocity;

	Vector m_velocity;
	int m_fInCombat;
};

LINK_ENTITY_TO_CLASS( monster_alien_controller, CController )

TYPEDESCRIPTION	CController::m_SaveData[] =
{
	DEFINE_ARRAY( CController, m_pBall, FIELD_CLASSPTR, 2 ),
	DEFINE_ARRAY( CController, m_iBall, FIELD_INTEGER, 2 ),
	DEFINE_ARRAY( CController, m_iBallTime, FIELD_TIME, 2 ),
	DEFINE_ARRAY( CController, m_iBallCurrent, FIELD_INTEGER, 2 ),
	DEFINE_FIELD( CController, m_vecEstVelocity, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CController, CSquadMonster )

const char *CController::pAttackSounds[] =
{
	"controller/con_attack1.wav",
	"controller/con_attack2.wav",
	"controller/con_attack3.wav",
};

const char *CController::pIdleSounds[] =
{
	"controller/con_idle1.wav",
	"controller/con_idle2.wav",
	"controller/con_idle3.wav",
	"controller/con_idle4.wav",
	"controller/con_idle5.wav",
};

const char *CController::pAlertSounds[] =
{
	"controller/con_alert1.wav",
	"controller/con_alert2.wav",
	"controller/con_alert3.wav",
};

const char *CController::pPainSounds[] =
{
	"controller/con_pain1.wav",
	"controller/con_pain2.wav",
	"controller/con_pain3.wav",
};

const char *CController::pDeathSounds[] =
{
	"controller/con_die1.wav",
	"controller/con_die2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CController::DefaultClassify( void )
{
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CController::SetYawSpeed( void )
{
	pev->yaw_speed = 120;
}

int CController::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// HACK HACK -- until we fix this.
	if( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CController::OnDying()
{
	// shut off balls
	/*
	m_iBall[0] = 0;
	m_iBallTime[0] = gpGlobals->time + 4.0f;
	m_iBall[1] = 0;
	m_iBallTime[1] = gpGlobals->time + 4.0f;
	*/

	// fade balls
	if( m_pBall[0] )
	{
		m_pBall[0]->SUB_StartFadeOut();
		m_pBall[0] = NULL;
	}
	if( m_pBall[1] )
	{
		m_pBall[1]->SUB_StartFadeOut();
		m_pBall[1] = NULL;
	}
	CSquadMonster::OnDying();
}

void CController::GibMonster( void )
{
	// delete balls
	if( m_pBall[0] )
	{
		UTIL_Remove( m_pBall[0] );
		m_pBall[0] = NULL;
	}
	if( m_pBall[1] )
	{
		UTIL_Remove( m_pBall[1] );
		m_pBall[1] = NULL;
	}
	CSquadMonster::GibMonster();
}

void CController::PainSound( void )
{
	if( RANDOM_LONG( 0, 5 ) < 2 )
		EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pPainSounds ); 
}	

void CController::AlertSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pAlertSounds ); 
}

void CController::IdleSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pIdleSounds ); 
}

void CController::AttackSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pAttackSounds ); 
}

void CController::DeathSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pDeathSounds ); 
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CController::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case CONTROLLER_AE_HEAD_OPEN:
		{
			Vector vecStart, angleGun;

			GetAttachment( 0, vecStart, angleGun );

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex() + 0x1000 );		// entity, attachment
				WRITE_COORD( vecStart.x );		// origin
				WRITE_COORD( vecStart.y );
				WRITE_COORD( vecStart.z );
				WRITE_COORD( 1 );	// radius
				WRITE_BYTE( 255 );	// R
				WRITE_BYTE( 192 );	// G
				WRITE_BYTE( 64 );	// B
				WRITE_BYTE( 20 );	// life * 10
				WRITE_COORD( -32 ); // decay
			MESSAGE_END();

			m_iBall[0] = 192;
			m_iBallTime[0] = gpGlobals->time + atoi( pEvent->options ) / 15.0f;
			m_iBall[1] = 255;
			m_iBallTime[1] = gpGlobals->time + atoi( pEvent->options ) / 15.0f;
		}
		break;
		case CONTROLLER_AE_BALL_SHOOT:
		{
			Vector vecStart, angleGun;
			
			GetAttachment( 0, vecStart, angleGun );

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex() + 0x1000 );		// entity, attachment
				WRITE_COORD( 0 );		// origin
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );
				WRITE_COORD( 32 );	// radius
				WRITE_BYTE( 255 );	// R
				WRITE_BYTE( 192 );	// G
				WRITE_BYTE( 64 );	// B
				WRITE_BYTE( 10 );	// life * 10
				WRITE_COORD( 32 ); // decay
			MESSAGE_END();

			CBaseMonster *pBall = (CBaseMonster*)Create( "controller_head_ball", vecStart, pev->angles, edict(), m_soundList );

			pBall->pev->velocity = Vector( 0.0f, 0.0f, 32.0f );
			if (m_pCine)
			{
				pBall->m_hEnemy = m_hTargetEnt;
			}
			else
			{
				pBall->m_hEnemy = m_hEnemy;
			}

			m_iBall[0] = 0;
			m_iBall[1] = 0;
		}
		break;
		case CONTROLLER_AE_SMALL_SHOOT:
		{
			AttackSound();
			m_flShootTime = gpGlobals->time;
			m_flShootEnd = m_flShootTime + atoi( pEvent->options ) / 15.0f;
		}
		break;
		case CONTROLLER_AE_POWERUP_FULL:
		{
			m_iBall[0] = 255;
			m_iBallTime[0] = gpGlobals->time + atoi( pEvent->options ) / 15.0f;
			m_iBall[1] = 255;
			m_iBallTime[1] = gpGlobals->time + atoi( pEvent->options ) / 15.0f;
		}
		break;
		case CONTROLLER_AE_POWERUP_HALF:
		{
			m_iBall[0] = 192;
			m_iBallTime[0] = gpGlobals->time + atoi( pEvent->options ) / 15.0f;
			m_iBall[1] = 192;
			m_iBallTime[1] = gpGlobals->time + atoi( pEvent->options ) / 15.0f;
		}
		break;
		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CController::Spawn()
{
	Precache();

	SetMyModel( "models/controller.mdl" );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->flags		|= FL_FLY;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	SetMyHealth( gSkillData.controllerHealth );
	pev->view_ofs		= Vector( 0.0f, 0.0f, -2.0f );// position of the eyes relative to monster's origin.
	SetMyFieldOfView(VIEW_FIELD_FULL);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CController::Precache()
{
	PrecacheMyModel( "models/controller.mdl" );

	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );

	PRECACHE_MODEL( "sprites/xspark4.spr" );

	UTIL_PrecacheOther( "controller_energy_ball" );
	UTIL_PrecacheOther( "controller_head_ball" );
}	

void CController::UpdateOnRemove()
{
	if( m_pBall[0] )
	{
		UTIL_Remove( m_pBall[0] );
		m_pBall[0] = 0;
	}

	if( m_pBall[1] )
	{
		UTIL_Remove( m_pBall[1] );
		m_pBall[1] = 0;
	}
	CSquadMonster::UpdateOnRemove();
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// Chase enemy schedule
Task_t tlControllerChaseEnemy[] =
{
	{ TASK_GET_PATH_TO_ENEMY, 128.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
};

Schedule_t slControllerChaseEnemy[] =
{
	{
		tlControllerChaseEnemy,
		ARRAYSIZE( tlControllerChaseEnemy ),
		bits_COND_NEW_ENEMY |
		bits_COND_TASK_FAILED,
		0,
		"ControllerChaseEnemy"
	},
};

Task_t tlControllerStrafe[] =
{
	{ TASK_WAIT, 0.2f },
	{ TASK_GET_PATH_TO_ENEMY, 128.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
	{ TASK_WAIT, 1.0f },
};

Schedule_t slControllerStrafe[] =
{
	{
		tlControllerStrafe,
		ARRAYSIZE( tlControllerStrafe ),
		bits_COND_NEW_ENEMY,
		0,
		"ControllerStrafe"
	},
};

Task_t tlControllerTakeCover[] =
{
	{ TASK_WAIT, 0.2f },
	{ TASK_FIND_COVER_FROM_ENEMY, 0.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
	{ TASK_WAIT, 1.0f },
};

Schedule_t slControllerTakeCover[] =
{
	{
		tlControllerTakeCover,
		ARRAYSIZE( tlControllerTakeCover ),
		bits_COND_NEW_ENEMY,
		0,
		"ControllerTakeCover"
	},
};

Task_t tlControllerFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 },
	{ TASK_WAIT_PVS, (float)0 },
};

Schedule_t slControllerFail[] =
{
	{
		tlControllerFail,
		ARRAYSIZE( tlControllerFail ),
		0,
		0,
		"ControllerFail"
	},
};

DEFINE_CUSTOM_SCHEDULES( CController )
{
	slControllerChaseEnemy,
	slControllerStrafe,
	slControllerTakeCover,
	slControllerFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CController, CSquadMonster )

//=========================================================
// StartTask
//=========================================================
void CController::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		CSquadMonster::StartTask( pTask );
		break;
	case TASK_GET_PATH_TO_ENEMY_LKP:
		{
			if( BuildNearestRoute( m_vecEnemyLKP, pev->view_ofs, pTask->flData, (m_vecEnemyLKP - pev->origin).Length() + 1024.0f ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				TaskFail("can't build path to enemy last known position");
			}
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if( pEnemy == NULL )
			{
				TaskFail("no enemy");
				return;
			}

			if( BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, pTask->flData, ( pEnemy->pev->origin - pev->origin).Length() + 1024.0f ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				TaskFail("can't build path to enemy");
			}
			break;
		}
	default:
		CSquadMonster::StartTask( pTask );
		break;
	}
}

Vector Intersect( Vector vecSrc, Vector vecDst, Vector vecMove, float flSpeed )
{
	Vector vecTo = vecDst - vecSrc;

	float a = DotProduct( vecMove, vecMove ) - flSpeed * flSpeed;
	float b = 0 * DotProduct( vecTo, vecMove ); // why does this work?
	float c = DotProduct( vecTo, vecTo );
	float t;

	if( a == 0 )
	{
		t = c / ( flSpeed * flSpeed );
	}
	else
	{
		t = b * b - 4.0f * a * c;
		t = sqrt( t ) / ( 2.0f * a );
		float t1 = -b +t;
		float t2 = -b -t;

		if( t1 < 0 || t2 < t1 )
			t = t2;
		else
			t = t1;
	}

	// ALERT( at_console, "Intersect %f\n", t );

	if( t < 0.1f )
		t = 0.1f;
	if( t > 10.0f )
		t = 10.0f;

	Vector vecHit = vecTo + vecMove * t;
	return vecHit.Normalize() * flSpeed;
}

int CController::LookupFloat()
{
	if( m_velocity.Length() < 32.0f )
	{
		return LookupSequence( "up" );
	}

	UTIL_MakeAimVectors( pev->angles );
	float x = DotProduct( gpGlobals->v_forward, m_velocity );
	float y = DotProduct( gpGlobals->v_right, m_velocity );
	float z = DotProduct( gpGlobals->v_up, m_velocity );

	if( fabs( x ) > fabs( y ) && fabs( x ) > fabs( z ) )
	{
		if( x > 0 )
			return LookupSequence( "forward" );
		else
			return LookupSequence( "backward" );
	}
	else if( fabs( y ) > fabs( z ) )
	{
		if( y > 0 )
			return LookupSequence( "right" );
		else
			return LookupSequence( "left" );
	}
	else
	{
		if( z > 0 )
			return LookupSequence( "up" );
		else
			return LookupSequence( "down" );
	}
}

//=========================================================
// RunTask 
//=========================================================
void CController::RunTask( Task_t *pTask )
{
	if( m_flShootEnd > gpGlobals->time )
	{
		Vector vecHand, vecAngle;

		GetAttachment( 2, vecHand, vecAngle );

		while( m_flShootTime < m_flShootEnd && m_flShootTime < gpGlobals->time )
		{
			Vector vecSrc = vecHand + pev->velocity * ( m_flShootTime - gpGlobals->time );
			Vector vecDir;

			if( m_pCine != 0 || m_hEnemy != 0 )
			{
				if (m_pCine != 0) // LRC- is this a script that's telling it to fire?
				{
					if (m_hTargetEnt != 0 && m_pCine->PreciseAttack())
					{
						vecDir = (m_hTargetEnt->pev->origin - pev->origin).Normalize() * gSkillData.controllerSpeedBall;
					}
					else
					{
						UTIL_MakeVectors(pev->angles);
						vecDir = gpGlobals->v_forward * gSkillData.controllerSpeedBall;
					}
				}
				else if (m_hEnemy != 0)
				{
					if( HasConditions( bits_COND_SEE_ENEMY ) )
					{
						m_vecEstVelocity = m_vecEstVelocity * 0.5f + m_hEnemy->pev->velocity * 0.5f;
					}
					else
					{
						m_vecEstVelocity = m_vecEstVelocity * 0.8f;
					}
					vecDir = Intersect( vecSrc, m_hEnemy->BodyTarget( pev->origin ), m_vecEstVelocity, gSkillData.controllerSpeedBall );
				}
				float delta = 0.03490f; // +-2 degree
				vecDir = vecDir + Vector( RANDOM_FLOAT( -delta, delta ), RANDOM_FLOAT( -delta, delta ), RANDOM_FLOAT( -delta, delta ) ) * gSkillData.controllerSpeedBall;

				vecSrc = vecSrc + vecDir * ( gpGlobals->time - m_flShootTime );
				CBaseMonster *pBall = (CBaseMonster*)Create( "controller_energy_ball", vecSrc, pev->angles, edict(), m_soundList );
				pBall->pev->velocity = vecDir;
			}
			m_flShootTime += 0.2f;
		}

		if( m_flShootTime > m_flShootEnd )
		{
			m_iBall[0] = 64;
			m_iBallTime[0] = m_flShootEnd;
			m_iBall[1] = 64;
			m_iBallTime[1] = m_flShootEnd;
			m_fInCombat = FALSE;
		}
	}

	switch( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
	case TASK_WAIT:
	case TASK_WAIT_FACE_ENEMY:
	case TASK_WAIT_PVS:
		if (m_hEnemy != 0)
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );
		}

		if( m_fSequenceFinished )
		{
			m_fInCombat = FALSE;
		}

		CSquadMonster::RunTask( pTask );

		if( !m_fInCombat )
		{
			if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				pev->sequence = LookupActivity( ACT_RANGE_ATTACK1 );
				pev->frame = 0;
				ResetSequenceInfo();
				m_fInCombat = TRUE;
			}
			else if( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
			{
				pev->sequence = LookupActivity( ACT_RANGE_ATTACK2 );
				pev->frame = 0;
				ResetSequenceInfo();
				m_fInCombat = TRUE;
			}
			else
			{
				int iFloat = LookupFloat();
				if( m_fSequenceFinished || iFloat != pev->sequence )
				{
					pev->sequence = iFloat;
					pev->frame = 0;
					ResetSequenceInfo();
				}
			}
		}
		break;
	default: 
		CSquadMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CController::GetSchedule( void )
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// Vector vecTmp = Intersect( Vector( 0, 0, 0 ), Vector( 100, 4, 7 ), Vector( 2, 10, -3 ), 20.0f );

			// dead enemy
			if( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// m_iFrustration++;
			}
			if( HasConditions( bits_COND_HEAVY_DAMAGE ) )
			{
				// m_iFrustration++;
			}
		}
		break;
        case MONSTERSTATE_IDLE:
        case MONSTERSTATE_ALERT:
                break;
	default:
		break;
	}

	return CSquadMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t *CController::GetScheduleOfType( int Type )
{
	// ALERT( at_console, "%d\n", m_iFrustration );
	switch( Type )
	{
	case SCHED_CHASE_ENEMY:
		return slControllerChaseEnemy;
	case SCHED_RANGE_ATTACK1:
		return slControllerStrafe;
	case SCHED_RANGE_ATTACK2:
	case SCHED_MELEE_ATTACK1:
	case SCHED_MELEE_ATTACK2:
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return slControllerTakeCover;
	case SCHED_FAIL:
		return slControllerFail;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// CheckRangeAttack1  - shoot a bigass energy ball out of their head
//
//=========================================================
BOOL CController::CheckRangeAttack1( float flDot, float flDist )
{
	if( flDot > 0.5f && flDist > 256.0f && flDist <= 2048.0f )
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CController::CheckRangeAttack2( float flDot, float flDist )
{
	if( flDot > 0.5f && flDist > 64.0f && flDist <= 2048.0f )
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CController::CheckMeleeAttack1( float flDot, float flDist )
{
	return FALSE;
}

void CController::SetActivity( Activity NewActivity )
{
	CBaseMonster::SetActivity( NewActivity );

	switch( m_Activity )
	{
	case ACT_WALK:
		m_flGroundSpeed = 100.0f;
		break;
	default:
		m_flGroundSpeed = 100.0f;
		break;
	}
}

//=========================================================
// RunAI
//=========================================================
void CController::RunAI( void )
{
	CBaseMonster::RunAI();
	Vector vecStart, angleGun;

	if( HasMemory( bits_MEMORY_KILLED ) )
		return;

	for( int i = 0; i < 2; i++ )
	{
		if( m_pBall[i] == NULL )
		{
			m_pBall[i] = CSprite::SpriteCreate( "sprites/xspark4.spr", pev->origin, TRUE );
			m_pBall[i]->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
			m_pBall[i]->SetAttachment( edict(), ( i + 3 ) );
			m_pBall[i]->SetScale( 1.0f );
		}

		float t = m_iBallTime[i] - gpGlobals->time;
		if( t > 0.1f )
			t = 0.1f / t;
		else
			t = 1.0f;

		m_iBallCurrent[i] += ( m_iBall[i] - m_iBallCurrent[i] ) * t;

		m_pBall[i]->SetBrightness( m_iBallCurrent[i] );

		GetAttachment( i + 2, vecStart, angleGun );
		UTIL_SetOrigin( m_pBall[i]->pev, vecStart );

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_ELIGHT );
			WRITE_SHORT( entindex() + 0x1000 * ( i + 3 ) );		// entity, attachment
			WRITE_COORD( vecStart.x );		// origin
			WRITE_COORD( vecStart.y );
			WRITE_COORD( vecStart.z );
			WRITE_COORD( m_iBallCurrent[i] / 8 );	// radius
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 192 );	// G
			WRITE_BYTE( 64 );	// B
			WRITE_BYTE( 5 );	// life * 10
			WRITE_COORD( 0 ); // decay
		MESSAGE_END();
	}
}

extern void DrawRoute( entvars_t *pev, WayPoint_t *m_Route, int m_iRouteIndex, int r, int g, int b );

void CController::Stop( void )
{ 
	m_IdealActivity = GetStoppedActivity(); 
}

#define DIST_TO_CHECK	200

void CController::Move( float flInterval )
{
	float		flWaypointDist;
	float		flCheckDist;
	float		flDist;// how far the lookahead check got before hitting an object.
	float		flMoveDist;
	Vector		vecDir;
	Vector		vecApex;
	CBaseEntity	*pTargetEnt;

	// Don't move if no valid route
	if( FRouteClear() )
	{
		TaskFail("route is empty");
		return;
	}

	if( m_flMoveWaitFinished > gpGlobals->time )
		return;

// Debug, test movement code
#if 0
//	if( CVAR_GET_FLOAT( "stopmove" ) != 0 )
	{
		if( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();
		return;
	}
#else
// Debug, draw the route
//	DrawRoute( pev, m_Route, m_iRouteIndex, 0, 0, 255 );
#endif
	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	if( m_flGroundSpeed == 0.0f )
	{
		m_flGroundSpeed = 100.0f;
		// TaskFail();
		// return;
	}

	flMoveDist = m_flGroundSpeed * flInterval;

	do
	{
		// local move to waypoint.
		vecDir = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Normalize();
		flWaypointDist = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Length();

		// MakeIdealYaw( m_Route[m_iRouteIndex].vecLocation );
		// ChangeYaw( pev->yaw_speed );

		// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
		if( flWaypointDist < DIST_TO_CHECK )
		{
			flCheckDist = flWaypointDist;
		}
		else
		{
			flCheckDist = DIST_TO_CHECK;
		}

		if( ( m_Route[m_iRouteIndex].iType & ( ~bits_MF_NOT_TO_MASK ) ) == bits_MF_TO_ENEMY )
		{
			// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
			pTargetEnt = m_hEnemy;
		}
		else if( ( m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK ) == bits_MF_TO_TARGETENT )
		{
			pTargetEnt = m_hTargetEnt;
		}

		// !!!BUGBUG - CheckDist should be derived from ground speed.
		// If this fails, it should be because of some dynamic entity blocking this guy.
		// We've already checked this path, so we should wait and time out if the entity doesn't move
		flDist = 0;
		if( CheckLocalMove( pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist ) != LOCALMOVE_VALID )
		{
			CBaseEntity *pBlocker;

			// Can't move, stop
			Stop();
			// Blocking entity is in global trace_ent
			pBlocker = CBaseEntity::Instance( gpGlobals->trace_ent );
			if( pBlocker )
			{
				DispatchBlocked( edict(), pBlocker->edict() );
			}

			if( pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && (gpGlobals->time-m_flMoveWaitFinished) > 3.0f )
			{
				// Can we still move toward our target?
				if( flDist < m_flGroundSpeed )
				{
					// Wait for a second
					m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
					//ALERT( at_aiconsole, "Move %s!!!\n", STRING( pBlocker->pev->classname ) );
					return;
				}
			}
			else
			{
				// try to triangulate around whatever is in the way.
				if( FTriangulate( pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist, pTargetEnt, &vecApex ) )
				{
					InsertWaypoint( vecApex, bits_MF_TO_DETOUR );
					RouteSimplify( pTargetEnt );
				}
				else
				{
	 				ALERT ( at_aiconsole, "Couldn't Triangulate\n" );
					Stop();
					if( m_moveWaitTime > 0 )
					{
						FRefreshRoute();
						m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime * 0.5f;
					}
					else
					{
						TaskFail("failed to move");
						//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, ( pev->origin + ( vecDir * flCheckDist ) ).z, m_Route[m_iRouteIndex].vecLocation.z );
					}
					return;
				}
			}
		}

		// UNDONE: this is a hack to quit moving farther than it has looked ahead.
		if( flCheckDist < flMoveDist )
		{
			MoveExecute( pTargetEnt, vecDir, flCheckDist / m_flGroundSpeed );

			// ALERT( at_console, "%.02f\n", flInterval );
			AdvanceRoute( flWaypointDist );
			flMoveDist -= flCheckDist;
		}
		else
		{
			MoveExecute( pTargetEnt, vecDir, flMoveDist / m_flGroundSpeed );

			if( ShouldAdvanceRoute( flWaypointDist - flMoveDist ) )
			{
				AdvanceRoute( flWaypointDist );
			}
			flMoveDist = 0;
		}

		if( MovementIsComplete() )
		{
			Stop();
			RouteClear();
		}
	} while( flMoveDist > 0.0f && flCheckDist > 0.0f );

	// cut corner?
	if( flWaypointDist < 128.0f )
	{
		if( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();

		if( m_flGroundSpeed > 100.0f )
			m_flGroundSpeed -= 40.0f;
	}
	else
	{
		if( m_flGroundSpeed < 400.0f )
			m_flGroundSpeed += 10.0f;
	}
}

BOOL CController::ShouldAdvanceRoute( float flWaypointDist )
{
	if( flWaypointDist <= 32.0f )
	{
		return TRUE;
	}

	return FALSE;
}

int CController::CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	TraceResult tr;

	UTIL_TraceHull( vecStart + Vector( 0, 0, 32 ), vecEnd + Vector( 0, 0, 32 ), dont_ignore_monsters, large_hull, edict(), &tr );

	// ALERT( at_console, "%.0f %.0f %.0f : ", vecStart.x, vecStart.y, vecStart.z );
	// ALERT( at_console, "%.0f %.0f %.0f\n", vecEnd.x, vecEnd.y, vecEnd.z );

	if( pflDist )
	{
		*pflDist = ( ( tr.vecEndPos - Vector( 0, 0, 32 ) ) - vecStart ).Length();// get the distance.
	}

	// ALERT( at_console, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
	if( tr.fStartSolid || tr.flFraction < 1.0f )
	{
		if( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
	}

	return LOCALMOVE_VALID;
}

void CController::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	if( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	// ALERT( at_console, "move %.4f %.4f %.4f : %f\n", vecDir.x, vecDir.y, vecDir.z, flInterval );

	// float flTotal = m_flGroundSpeed * pev->framerate * flInterval;
	// UTIL_MoveToOrigin ( ENT( pev ), m_Route[m_iRouteIndex].vecLocation, flTotal, MOVE_STRAFE );

	m_velocity = m_velocity * 0.8f + m_flGroundSpeed * vecDir * 0.2f;

	UTIL_MoveToOrigin( ENT( pev ), pev->origin + m_velocity, m_velocity.Length() * flInterval, MOVE_STRAFE );	
}

class CControllerDead : public CDeadMonster
{
public:
	void Spawn( void );
	int	DefaultClassify ( void ) { return	CLASS_ALIEN_MILITARY; }

	const char* getPos(int pos) const;
};

const char* CControllerDead::getPos(int pos) const
{
	return "die1";
}

LINK_ENTITY_TO_CLASS( monster_alien_controller_dead, CControllerDead )

void CControllerDead :: Spawn( )
{
	SpawnHelper("models/controller.mdl", BLOOD_COLOR_YELLOW, gSkillData.controllerHealth/2);
	MonsterInitDead();
	pev->frame = 255;
}

//=========================================================
// Controller bouncy ball attack
//=========================================================
class CControllerHeadBall : public CBaseMonster
{
	void Spawn( void );
	void Precache( void );
	void EXPORT HuntThink( void );
	void EXPORT DieThink( void );
	void EXPORT BounceTouch( CBaseEntity *pOther );
	void MovetoTarget( Vector vecTarget );
	void Crawl( void );
	int m_flNextAttack;
	Vector m_vecIdeal;
	EHANDLE m_hOwner;
};

LINK_ENTITY_TO_CLASS( controller_head_ball, CControllerHeadBall )

void CControllerHeadBall::Spawn( void )
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT( pev ), "sprites/xspark4.spr" );
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->renderamt = 255;
	pev->scale = 2.0f;

	UTIL_SetSize(pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CControllerHeadBall::HuntThink );
	SetTouch( &CControllerHeadBall::BounceTouch );

	m_vecIdeal = Vector( 0, 0, 0 );

	pev->nextthink = gpGlobals->time + 0.1f;

	m_hOwner = Instance( pev->owner );
	pev->dmgtime = gpGlobals->time;
}

void CControllerHeadBall::Precache( void )
{
	PRECACHE_MODEL( "sprites/xspark4.spr" );
	PRECACHE_SOUND( "debris/zap4.wav" );
	PRECACHE_SOUND( "weapons/electro4.wav" );
}

void CControllerHeadBall::HuntThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;

	pev->renderamt -= 5;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( entindex() );		// entity, attachment
		WRITE_COORD( pev->origin.x );		// origin
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( pev->renderamt / 16 );	// radius
		WRITE_BYTE( 255 );	// R
		WRITE_BYTE( 255 );	// G
		WRITE_BYTE( 255 );	// B
		WRITE_BYTE( 2 );	// life * 10
		WRITE_COORD( 0 ); // decay
	MESSAGE_END();

	// check world boundaries
	if( gpGlobals->time - pev->dmgtime > 5 || pev->renderamt < 64 || m_hEnemy == 0 || m_hOwner == 0 || pev->origin.x < -4096 || pev->origin.x > 4096 || pev->origin.y < -4096 || pev->origin.y > 4096 || pev->origin.z < -4096 || pev->origin.z > 4096 )
	{
		SetTouch( NULL );
		UTIL_Remove( this );
		return;
	}

	MovetoTarget( m_hEnemy->Center() );

	if( ( m_hEnemy->Center() - pev->origin ).Length() < 64 )
	{
		TraceResult tr;

		UTIL_TraceLine( pev->origin, m_hEnemy->Center(), dont_ignore_monsters, ENT( pev ), &tr );

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if( pEntity != NULL && pEntity->pev->takedamage )
		{
			pEntity->ApplyTraceAttack(pev, m_hOwner->pev, gSkillData.controllerDmgZap, pev->velocity, &tr, DMG_SHOCK);
		}

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMENTPOINT );
			WRITE_SHORT( entindex() );
			WRITE_COORD( tr.vecEndPos.x );
			WRITE_COORD( tr.vecEndPos.y );
			WRITE_COORD( tr.vecEndPos.z );
			WRITE_SHORT( g_sModelIndexLaser );
			WRITE_BYTE( 0 ); // frame start
			WRITE_BYTE( 10 ); // framerate
			WRITE_BYTE( 3 ); // life
			WRITE_BYTE( 20 );  // width
			WRITE_BYTE( 0 );   // noise
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 255 );	// brightness
			WRITE_BYTE( 10 );		// speed
		MESSAGE_END();

		EmitAmbientSound( tr.vecEndPos, "weapons/electro4.wav", 0.5f, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );

		m_flNextAttack = gpGlobals->time + 3.0f;

		SetThink( &CControllerHeadBall::DieThink );
		pev->nextthink = gpGlobals->time + 0.3f;
	}

	//Crawl();
}

void CControllerHeadBall::DieThink( void )
{
	UTIL_Remove( this );
}

void CControllerHeadBall::MovetoTarget( Vector vecTarget )
{
	// accelerate
	float flSpeed = m_vecIdeal.Length();
	if( flSpeed == 0.0f )
	{
		m_vecIdeal = pev->velocity;
		flSpeed = m_vecIdeal.Length();
	}

	if( flSpeed > 400.0f )
	{
		m_vecIdeal = m_vecIdeal.Normalize() * 400.0f;
	}
	m_vecIdeal = m_vecIdeal + ( vecTarget - pev->origin ).Normalize() * 100.0f;
	pev->velocity = m_vecIdeal;
}

void CControllerHeadBall::Crawl( void )
{
	Vector vecAim = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ) ).Normalize();
	Vector vecPnt = pev->origin + pev->velocity * 0.3f + vecAim * 64.0f;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMENTPOINT );
		WRITE_SHORT( entindex() );
		WRITE_COORD( vecPnt.x );
		WRITE_COORD( vecPnt.y );
		WRITE_COORD( vecPnt.z );
		WRITE_SHORT( g_sModelIndexLaser );
		WRITE_BYTE( 0 ); // frame start
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( 3 ); // life
		WRITE_BYTE( 20 );  // width
		WRITE_BYTE( 0 );   // noise
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 10 );		// speed
	MESSAGE_END();
}

void CControllerHeadBall::BounceTouch( CBaseEntity *pOther )
{
	Vector vecDir = m_vecIdeal.Normalize();

	TraceResult tr = UTIL_GetGlobalTrace();

	float n = -DotProduct( tr.vecPlaneNormal, vecDir );

	vecDir = 2.0f * tr.vecPlaneNormal * n + vecDir;

	m_vecIdeal = vecDir * m_vecIdeal.Length();
}

class CControllerZapBall : public CBaseMonster
{
	void Spawn( void );
	void Precache( void );
	void EXPORT AnimateThink( void );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );

	EHANDLE m_hOwner;

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( controller_energy_ball, CControllerZapBall )

TYPEDESCRIPTION	CControllerZapBall::m_SaveData[] =
{
	DEFINE_FIELD( CControllerZapBall, m_hOwner, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CControllerZapBall, CBaseMonster )

void CControllerZapBall::Spawn( void )
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "sprites/xspark4.spr" );
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->renderamt = 255;
	pev->scale = 0.5f;

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CControllerZapBall::AnimateThink );
	SetTouch( &CControllerZapBall::ExplodeTouch );

	m_hOwner = Instance( pev->owner );
	pev->dmgtime = gpGlobals->time; // keep track of when ball spawned
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CControllerZapBall::Precache( void )
{
	PRECACHE_MODEL( "sprites/xspark4.spr" );
	// PRECACHE_SOUND( "debris/zap4.wav" );
	// PRECACHE_SOUND( "weapons/electro4.wav" );
}

void CControllerZapBall::AnimateThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;

	pev->frame = ( (int)pev->frame + 1 ) % 11;

	if( gpGlobals->time - pev->dmgtime > 5 || pev->velocity.Length() < 10.0f )
	{
		SetTouch( NULL );
		UTIL_Remove( this );
	}
}

void CControllerZapBall::ExplodeTouch( CBaseEntity *pOther )
{
	if( pOther->pev->takedamage )
	{
		TraceResult tr = UTIL_GetGlobalTrace();

		entvars_t *pevOwner;

		if( m_hOwner != 0 )
		{
			pevOwner = m_hOwner->pev;
		}
		else
		{
			pevOwner = pev;
		}

		pOther->ApplyTraceAttack(pev, pevOwner, gSkillData.controllerDmgBall, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM);

		EmitAmbientSound( tr.vecEndPos, "weapons/electro4.wav", 0.3f, ATTN_NORM, 0, RANDOM_LONG( 90, 99 ) );
	}

	UTIL_Remove( this );
}

#define ZAPBALLTRAP_DETECT_SOUND "ambience/alien_frantic.wav"
#define ZAPBALLTRAP_LAUNCH_SOUND "debris/beamstart4.wav"

class CZapBallTrap : public CBaseEntity
{
public:
	void KeyValue( KeyValueData* pkvd );
	void Spawn();
	void Precache();
	void Animate();
	void EXPORT DetectThink();
	void EXPORT Materialize();
	void LaunchBall(CBaseEntity* pTarget);
	void EXPORT EnableUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	float IdleThinkPeriod() {
		return 0.2f;
	}
	float AlertThinkPeriod() {
		return 0.1f;
	}
	float ThinkPeriod() const {
		return pev->frags;
	}
	void SetThinkPeriod(float value) const {
		pev->frags = value;
	}

	float SenseRadius() const {
		return pev->health > 0.0f ? pev->health : gSkillData.zaptrapSenseRadius;
	}
	float FastSenseRadius() const {
		return SenseRadius() / 3;
	}

	bool IncreaseAwareness(CBaseEntity *pTarget, int value);
	void DecreaseAwareness();
	void AwareEffect();
	int CurrentAwareness() const {
		return pev->button;
	}
	int MaxAwareness() const {
		return 20;
	}

	int BaseBrigthness() const {
		return 80;
	}
	int MaxBrightness() const {
		return 255;
	}

	float BaseScale() const {
		return 1.0f;
	}
	float MaxScale() const {
		return BaseScale() * 2;
	}

	float RespawnTime() const {
		return pev->dmg_take > 0 ? pev->dmg_take : gSkillData.zaptrapRespawnTime;
	}
};

LINK_ENTITY_TO_CLASS( env_energy_ball_trap, CZapBallTrap )

void CZapBallTrap::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		pev->dmg_take = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "radius"))
	{
		pev->health = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CZapBallTrap::Precache()
{
	UTIL_PrecacheOther("controller_head_ball");
	PRECACHE_SOUND(ZAPBALLTRAP_DETECT_SOUND);
	PRECACHE_SOUND(ZAPBALLTRAP_LAUNCH_SOUND);
}

void CZapBallTrap::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	SET_MODEL(ENT( pev ), "sprites/xspark4.spr" );
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;

	UTIL_SetSize(pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	if (FStringNull(pev->targetname))
	{
		Materialize();
	}
	else
	{
		pev->effects |= EF_NODRAW;
		SetUse(&CZapBallTrap::EnableUse);
	}
}

void CZapBallTrap::EnableUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (useType != USE_OFF)
	{
		Materialize();
		SetUse(NULL);
	}
}

void CZapBallTrap::Animate()
{
	pev->frame = ( (int)pev->frame + 1 ) % 11;
}

void CZapBallTrap::DetectThink()
{
	Animate();
	CBaseEntity *pFoundTarget = NULL;
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer && pPlayer->IsPlayer())
		{
			const float distance = (pPlayer->pev->origin - pev->origin).Length();
			if (distance <= SenseRadius())
			{
				TraceResult tr;
				UTIL_TraceLine(pev->origin, pPlayer->Center(), dont_ignore_monsters, ENT(pev), &tr);
				CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
				if (pEntity == pPlayer)
				{
					pFoundTarget = pPlayer;
					bool ballLaunched = IncreaseAwareness(pPlayer, distance <= FastSenseRadius() ? 4 : 2);
					if (ballLaunched)
						return;
				}
			}
		}
	}
	if (!pFoundTarget)
	{
		DecreaseAwareness();
	}
	pev->nextthink = gpGlobals->time + ThinkPeriod();
}

void CZapBallTrap::LaunchBall(CBaseEntity *pTarget)
{
	pev->effects |= EF_NODRAW;
	SetThink(&CZapBallTrap::Materialize);
	pev->nextthink = gpGlobals->time + RespawnTime();

	EMIT_SOUND(edict(), CHAN_WEAPON, ZAPBALLTRAP_LAUNCH_SOUND, 1.0f, ATTN_STATIC);

	CBaseMonster *pBall = (CBaseMonster*)CBaseEntity::Create( "controller_head_ball", pev->origin, pev->angles, edict() );

	pBall->pev->velocity = Vector( 0.0f, 0.0f, 32.0f );
	pBall->m_hEnemy = pTarget;
}

void CZapBallTrap::Materialize()
{
	pev->renderamt = BaseBrigthness();
	pev->scale = BaseScale();
	pev->button = 0;
	SetThinkPeriod(IdleThinkPeriod());
	pev->effects &= ~EF_NODRAW;
	pev->frame = 0;
	SetThink( &CZapBallTrap::DetectThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

bool CZapBallTrap::IncreaseAwareness(CBaseEntity *pTarget, int value)
{
	const int prevAwareness = CurrentAwareness();
	pev->button += value;
	const int newAwareness = pev->button;

	if (newAwareness >= MaxAwareness())
	{
		STOP_SOUND(edict(), CHAN_ITEM, ZAPBALLTRAP_DETECT_SOUND);
		LaunchBall(pTarget);
		return true;
	}
	if (prevAwareness == 0)
	{
		SetThinkPeriod(AlertThinkPeriod());
		EMIT_SOUND_DYN(edict(), CHAN_ITEM, ZAPBALLTRAP_DETECT_SOUND, 0.8f, ATTN_STATIC, 0, 110);
	}

	AwareEffect();

	return false;
}

void CZapBallTrap::DecreaseAwareness()
{
	if (pev->button > 0)
	{
		pev->button--;
		AwareEffect();
		if (pev->button == 0)
		{
			STOP_SOUND(edict(), CHAN_ITEM, ZAPBALLTRAP_DETECT_SOUND);
			SetThinkPeriod(IdleThinkPeriod());
		}
	}
}

void CZapBallTrap::AwareEffect()
{
	const float factor = (float)CurrentAwareness() / (float)MaxAwareness();
	pev->scale = BaseScale() + (MaxScale() - BaseScale()) * factor;
	pev->renderamt = BaseBrigthness() + (MaxBrightness() - BaseBrigthness()) * factor;
}

