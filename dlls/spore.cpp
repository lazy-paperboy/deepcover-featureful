#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"decals.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"spore.h"
#include	"game.h"

#define FEATURE_SPORE_AMMO_CEILING_LIGHT 1

#if FEATURE_SPOREGRENADE
LINK_ENTITY_TO_CLASS(spore, CSpore)

TYPEDESCRIPTION	CSpore::m_SaveData[] =
{
	DEFINE_FIELD(CSpore, m_SporeType, FIELD_INTEGER),
	DEFINE_FIELD(CSpore, m_flIgniteTime, FIELD_TIME),
	DEFINE_FIELD(CSpore, m_bIsAI, FIELD_BOOLEAN),
	DEFINE_FIELD(CSpore, m_hSprite, FIELD_EHANDLE)
};

IMPLEMENT_SAVERESTORE(CSpore, CGrenade)

void CSpore::Precache(void)
{
	PRECACHE_MODEL("models/spore.mdl");
	PRECACHE_MODEL("sprites/glow01.spr");

	m_iBlow = PRECACHE_MODEL("sprites/spore_exp_01.spr");
	m_iBlowSmall = PRECACHE_MODEL("sprites/spore_exp_c_01.spr");
	m_iSpitSprite = m_iTrail = PRECACHE_MODEL("sprites/tinyspit.spr");

	PRECACHE_SOUND("weapons/splauncher_bounce.wav");
	PRECACHE_SOUND("weapons/splauncher_impact.wav");
}

void CSpore::Spawn()
{
	Precache();

	if (m_SporeType == GRENADE)
		pev->movetype = MOVETYPE_BOUNCE;
	else
		pev->movetype = MOVETYPE_FLY;

	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("spore");

	SET_MODEL(edict(), "models/spore.mdl");

	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	UTIL_SetOrigin(pev, pev->origin);

	SetThink(&CSpore::FlyThink);

	if (m_SporeType == GRENADE)
	{
		SetTouch(&CSpore::MyBounceTouch);

		if (!m_bPuked)
		{
			pev->angles.x -= RANDOM_LONG(-5, 5) + 30;
		}
	}
	else
	{
		SetTouch(&CSpore::RocketTouch);
	}

	if (!m_bIsAI)
	{
		pev->gravity = 1;
	}
	else
	{
		pev->gravity = 0.5;
		pev->friction = 0.7;
	}

	pev->dmg = gSkillData.plrDmgSpore;

	m_flIgniteTime = gpGlobals->time;

	pev->nextthink = gpGlobals->time + 0.01;

	CSprite* sprite = CSprite::SpriteCreate("sprites/glow01.spr", pev->origin, false);
	if (sprite) {
		sprite->SetTransparency(kRenderTransAdd, 180, 180, 40, 100, kRenderFxDistort);
		sprite->SetScale(0.8);
		sprite->SetAttachment(edict(), 0);
		m_hSprite = sprite;
	}

	m_fRegisteredSound = false;

	m_flSoundDelay = gpGlobals->time;
}

void CSpore::BounceSound()
{
	//Nothing
}

void CSpore::IgniteThink()
{
	SetThink(NULL);
	SetTouch(NULL);

	if (m_hSprite)
	{
		UTIL_Remove(m_hSprite);
		m_hSprite = 0;
	}

	EmitSound(CHAN_WEAPON, "weapons/splauncher_impact.wav", VOL_NORM, ATTN_NORM);

	const Vector vecDir = pev->velocity.Normalize();

	TraceResult tr;

	UTIL_TraceLine(
		pev->origin, pev->origin + vecDir * (m_SporeType == GRENADE ? 64 : 32),
		dont_ignore_monsters, edict(), &tr);

	if (g_modFeatures.opfor_decals)
		UTIL_DecalTrace(&tr, DECAL_SPR_SPLT1 + RANDOM_LONG(0, 2));
	else
		UTIL_DecalTrace(&tr, DECAL_YBLOOD5 + RANDOM_LONG(0, 1));

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE_SPRAY);
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_COORD( tr.vecPlaneNormal.x );
	WRITE_COORD( tr.vecPlaneNormal.y );
	WRITE_COORD( tr.vecPlaneNormal.z );
	WRITE_SHORT(m_iSpitSprite);
	WRITE_BYTE(100);
	WRITE_BYTE(40);
	WRITE_BYTE(180);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_BYTE(10);
	WRITE_BYTE(15);
	WRITE_BYTE(220);
	WRITE_BYTE(40);
	WRITE_BYTE(5);
	WRITE_BYTE(10);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE);
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_SHORT(RANDOM_LONG(0, 1) ? m_iBlow : m_iBlowSmall);
	WRITE_BYTE(20);
	WRITE_BYTE(128);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE_SPRAY);
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_COORD(RANDOM_FLOAT(-1, 1));
	WRITE_COORD(1);
	WRITE_COORD(RANDOM_FLOAT(-1, 1));
	WRITE_SHORT(m_iTrail);
	WRITE_BYTE(2);
	WRITE_BYTE(20);
	WRITE_BYTE(80);
	MESSAGE_END();

	::RadiusDamage(pev->origin, pev, VARS(pev->owner), pev->dmg, 200, CLASS_NONE, DMG_ALWAYSGIB | DMG_BLAST);

	SetThink(&CSpore::SUB_Remove);

	pev->nextthink = gpGlobals->time;
}

void CSpore::FlyThink()
{
	const float flDelay = m_bIsAI ? 4.0 : 2.0;

	if (m_SporeType != GRENADE || (gpGlobals->time <= m_flIgniteTime + flDelay))
	{
		Vector velocity = pev->velocity.Normalize();

		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( velocity.x );
		WRITE_COORD( velocity.y );
		WRITE_COORD( velocity.z );
		WRITE_SHORT(m_iTrail);
		WRITE_BYTE(2);
		WRITE_BYTE(20);
		WRITE_BYTE(80);
		MESSAGE_END();
	}
	else
	{
		SetThink(&CSpore::IgniteThink);
	}

	pev->nextthink = gpGlobals->time + 0.03;
}

void CSpore::GibThink()
{
	//Nothing
}

void CSpore::RocketTouch(CBaseEntity* pOther)
{
	if (pOther->pev->takedamage != DAMAGE_NO)
	{
		pOther->TakeDamage(pev, VARS(pev->owner), gSkillData.plrDmgSpore, DMG_GENERIC);
	}

	IgniteThink();
}

void CSpore::MyBounceTouch(CBaseEntity* pOther)
{
	if (pOther->pev->takedamage == DAMAGE_NO)
	{
		if (pOther->edict() != pev->owner)
		{
			if (gpGlobals->time > m_flSoundDelay)
			{
				CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, (int)(pev->dmg * 2.5f), 0.3);

				m_flSoundDelay = gpGlobals->time + 1.0;
			}

			if ((pev->flags & FL_ONGROUND) != 0)
			{
				pev->velocity = pev->velocity * 0.5;
			}
			else
			{
				EmitSoundDyn(CHAN_VOICE, "weapons/splauncher_bounce.wav", 0.25, ATTN_NORM, 0, PITCH_NORM);
			}
		}
	}
	else
	{
		pOther->TakeDamage(pev, VARS(pev->owner), gSkillData.plrDmgSpore, DMG_GENERIC);

		IgniteThink();
	}
}

CSpore* CSpore::CreateSpore(const Vector& vecOrigin, const Vector& vecAngles, const Vector& vecVelocity, CBaseEntity* pOwner, SporeType sporeType, bool bIsAI, bool bPuked, string_t soundList)
{
	CSpore* pSpore = GetClassPtr((CSpore *)NULL);
	UTIL_SetOrigin(pSpore->pev, vecOrigin);

	pSpore->m_SporeType = sporeType;

	pSpore->m_bIsAI = bIsAI;
	pSpore->m_bPuked = bPuked;

	pSpore->Spawn();

	pSpore->pev->velocity = vecVelocity;
	pSpore->pev->angles = vecAngles;

	pSpore->pev->owner = pOwner->edict();

	return pSpore;
}

CSpore* CSpore::ShootContact(CBaseEntity *pOwner, const Vector &vecOrigin, const Vector& vecAngles, const Vector& vecVelocity)
{
	return CSpore::CreateSpore(vecOrigin, vecAngles, vecVelocity, pOwner, CSpore::ROCKET);
}

CSpore* CSpore::ShootTimed(CBaseEntity *pOwner, const Vector &vecOrigin, const Vector &vecAngles, const Vector& vecVelocity, bool bIsAI)
{
	return CSpore::CreateSpore(vecOrigin, vecAngles, vecVelocity, pOwner, CSpore::GRENADE, bIsAI, false);
}

void CSpore::UpdateOnRemove()
{
	CGrenade::UpdateOnRemove();
	if (m_hSprite)
	{
		UTIL_Remove(m_hSprite);
		m_hSprite = 0;
	}
}

//=========================================================
// Opposing Forces Spore Ammo
//=========================================================
#define		SACK_GROUP			1
#define		SACK_EMPTY			0
#define		SACK_FULL			1

class CSporeAmmo : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT IdleThink ( void );
	void EXPORT AmmoTouch ( CBaseEntity *pOther );
	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	virtual int SizeForGrapple() { return GRAPPLE_FIXED; }

	int m_iExplode;
};


typedef enum
{
	SPOREAMMO_IDLE = 0,
	SPOREAMMO_SPAWNUP,
	SPOREAMMO_SNATCHUP,
	SPOREAMMO_SPAWNDOWN,
	SPOREAMMO_SNATCHDOWN,
	SPOREAMMO_IDLE1,
	SPOREAMMO_IDLE2,
} SPOREAMMO;

LINK_ENTITY_TO_CLASS( ammo_spore, CSporeAmmo )

void CSporeAmmo :: Precache( void )
{
	PRECACHE_MODEL("models/spore_ammo.mdl");
	m_iExplode = PRECACHE_MODEL ("sprites/spore_exp_c_01.spr");
	PRECACHE_SOUND("weapons/spore_ammo.wav");
	UTIL_PrecacheOther ( "spore" );
}
//=========================================================
// Spawn
//=========================================================
void CSporeAmmo :: Spawn( void )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/spore_ammo.mdl");
	UTIL_SetSize(pev, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ));
	pev->takedamage = DAMAGE_YES;
	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_NONE;
	pev->framerate		= 1.0f;
	pev->health			= 1.0f;
	pev->animtime		= gpGlobals->time;

	pev->sequence = SPOREAMMO_SPAWNDOWN;
	pev->body = 1;

	pev->origin.z += 16;
	UTIL_SetOrigin( pev, pev->origin );

	pev->angles.x -= 90;// :3
#if FEATURE_SPORE_AMMO_CEILING_LIGHT
	if (fabs(pev->angles.x - 180.0f) < 5.0f) {
		pev->effects |= EF_INVLIGHT;
	}
#endif
	SetThink (&CSporeAmmo::IdleThink);
	SetTouch (&CSporeAmmo::AmmoTouch);

	pev->nextthink = gpGlobals->time + 4;
}

//=========================================================
// Override all damage
//=========================================================
int CSporeAmmo::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (pev->body != 0)
	{
		Vector vecSrc = pev->origin + gpGlobals->v_forward * -32;

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
			WRITE_COORD( vecSrc.x );	// Send to PAS because of the sound
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 25  ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NOSOUND );
		MESSAGE_END();


		//ALERT( at_console, "angles %f %f %f\n", pev->angles.x, pev->angles.y, pev->angles.z );

		Vector angles = pev->angles;
		angles.x -= 90;
		angles.y += 180;

		Vector vecLaunchDir = angles;

		vecLaunchDir.x += RANDOM_FLOAT( -20, 20 );
		vecLaunchDir.y += RANDOM_FLOAT( -20, 20 );
		vecLaunchDir.z += RANDOM_FLOAT( -20, 20 );

		UTIL_MakeVectors( vecLaunchDir );
		Vector vecVelocity = gpGlobals->v_forward * CSpore::SporeGrenadeSpeed();
		CSpore* pSpore = CSpore::CreateSpore(pev->origin, vecLaunchDir, vecVelocity, this, CSpore::GRENADE, false, true);

		pev->frame = 0;
		pev->animtime		= gpGlobals->time + 0.1;
		pev->sequence		= SPOREAMMO_SNATCHDOWN;
		pev->body			= 0;
		pev->nextthink = gpGlobals->time + 0.66f;
		SetThink (&CSporeAmmo::IdleThink);
		return 1;
	}
	return 0;
}

void CSporeAmmo :: IdleThink ( void )
{
	switch (pev->sequence)
	{
	case SPOREAMMO_SPAWNDOWN:
	{
		pev->sequence = SPOREAMMO_IDLE1;
		pev->animtime = gpGlobals->time;
		pev->frame = 0;
		break;
	}
	case SPOREAMMO_SNATCHDOWN:
	{
		pev->sequence = SPOREAMMO_IDLE;
		pev->animtime = gpGlobals->time;
		pev->frame = 0;
		pev->nextthink = gpGlobals->time + 10.0f;
		break;
	}
	case SPOREAMMO_IDLE:
	{
		pev->body = 1;
		pev->sequence = SPOREAMMO_SPAWNDOWN;
		pev->animtime = gpGlobals->time;
		pev->frame = 0;
		pev->nextthink = gpGlobals->time + 4.0f;
		break;
	}
	default:
		break;
	}
}

void CSporeAmmo :: AmmoTouch ( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() || pev->body == 0 )
		return;

	int bResult = (pOther->GiveAmmo( AMMO_SPORE_GIVE, "spores" ) != -1);
	if (bResult)
	{
		EmitSound(CHAN_ITEM, "weapons/spore_ammo.wav", 1, ATTN_NORM);

		pev->frame = 0;
		pev->animtime		= gpGlobals->time;
		pev->sequence = SPOREAMMO_SNATCHDOWN;
		pev->body = 0;
		pev->nextthink = gpGlobals->time + 0.66f;
	}
}

#endif
