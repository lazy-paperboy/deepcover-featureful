#pragma once
#ifndef MOD_FEATURES_H
#define MOD_FEATURES_H

/*
 * 0 to disable feature
 * 1 to enable feature
 */

// Enable opfor specific changes, like more weapon slots, green hud, etc.
// Follow the symbol to see what it actually changes
#define FEATURE_OPFOR_SPECIFIC 0

// Fast way to enable/disable entities that require extra content to be added in mod
#define FEATURE_OPFOR_WEAPONS 1
#define FEATURE_OPFOR_MONSTERS 1
#define FEATURE_BSHIFT_MONSTERS 1
#define FEATURE_SVENCOOP_MONSTERS 1

// New monsters
#define FEATURE_ZOMBIE_BARNEY (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_ZOMBIE_SOLDIER (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_GONOME (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_PITDRONE (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_SHOCKTROOPER (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_VOLTIFORE (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_MASSN (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_BLACK_OSPREY (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_BLACK_APACHE (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_OTIS (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_CLEANSUIT_SCIENTIST (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_BABYGARG (0 || FEATURE_SVENCOOP_MONSTERS)
#define FEATURE_OPFOR_GRUNT (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_PITWORM (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_GENEWORM (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_DRILLSERGEANT (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_RECRUIT (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_ROBOGRUNT (0 || FEATURE_SVENCOOP_MONSTERS)
#define FEATURE_HWGRUNT (0 || FEATURE_SVENCOOP_MONSTERS)
#define FEATURE_ROSENBERG (0 || FEATURE_BSHIFT_MONSTERS)
#define FEATURE_GUS 0

#define FEATURE_ISLAVE_DEAD (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_HOUNDEYE_DEAD (0 || FEATURE_OPFOR_MONSTERS)
#define FEATURE_OPFOR_DEADHAZ (0 || FEATURE_OPFOR_SPECIFIC)
#define FEATURE_SKELETON (0 || FEATURE_OPFOR_MONSTERS)

// whether fgrunts and black mesa personnel are enemies like in Opposing Force
#define FEATURE_OPFOR_ALLY_RELATIONSHIP (0 || FEATURE_OPFOR_SPECIFIC)

// whether race x and alien military are enemies
#define FEATURE_RACEX_AND_AMIL_ENEMIES 1

// whether race x and alien monsters are enemies
#define FEATURE_RACEX_AND_AMONSTERS_ENEMIES 0

// whether blackops monsters have a separate class which is enemy to military
#define FEATURE_BLACKOPS_CLASS 0

// whether monsters get health from eating meat or enemy corpses
#define FEATURE_EAT_FOR_HEALTH 1

// enable reverse relationship models, like barnabus
#define FEATURE_REVERSE_RELATIONSHIP_MODELS 0

// monsters who carry hand grenades will drop one hand grenade upon death
#define FEATURE_MONSTERS_DROP_HANDGRENADES 0

// New weapons
#define FEATURE_PIPEWRENCH (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_KNIFE (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_GRAPPLE (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_DESERT_EAGLE (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_PENGUIN (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_M249 (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_SNIPERRIFLE (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_DISPLACER (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_SHOCKRIFLE (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_SPORELAUNCHER (0 || FEATURE_OPFOR_WEAPONS)
#define FEATURE_MEDKIT 0
#define FEATURE_UZI 0

// Weapon features
#define FEATURE_CROWBAR_IDLE_ANIM 1
#define FEATURE_TRIPMINE_NONSOLID (0 || FEATURE_OPFOR_SPECIFIC)

// Dependent features
#define FEATURE_SHOCKBEAM (FEATURE_SHOCKTROOPER || FEATURE_SHOCKRIFLE)
#define FEATURE_SPOREGRENADE (FEATURE_SHOCKTROOPER || FEATURE_SPORELAUNCHER)

// Replacement stuff
#if FEATURE_DESERT_EAGLE
#define DESERT_EAGLE_DROP_NAME "weapon_eagle"
#else
#define DESERT_EAGLE_DROP_NAME "ammo_357"
#endif

#if FEATURE_M249
#define M249_DROP_NAME "weapon_m249"
#else
#define M249_DROP_NAME "ammo_9mmAR"
#endif

#if FEATURE_OPFOR_DEADHAZ
#define DEADHAZMODEL "models/deadhaz.mdl"
#else
#define DEADHAZMODEL "models/player.mdl"
#endif

// Misc features
#define FEATURE_STRAFE_BOBBING 0
#define FEATURE_SUIT_NO_SOUNDS (0 || FEATURE_OPFOR_SPECIFIC)
#define FEATURE_OPFOR_DECALS (0 || FEATURE_OPFOR_SPECIFIC)
#define FEATURE_OPFOR_NIGHTVISION (0 || FEATURE_OPFOR_SPECIFIC)
#define FEATURE_CS_NIGHTVISION (0 || FEATURE_OPFOR_SPECIFIC)
#define FEATURE_MOVE_MODE 0
#define FEATURE_FLASHLIGHT_ITEM 0
#define FEATURE_SUIT_FLASHLIGHT 1 // whether suit always provides flashlight
#define FEATURE_CLIENTSIDE_HUDSOUND 0

#define FEATURE_NIGHTVISION (FEATURE_OPFOR_NIGHTVISION || FEATURE_CS_NIGHTVISION)

#define FEATURE_ROPE (1 || FEATURE_OPFOR_SPECIFIC)

// whether items drop instantly when spawned, e.g. from func_breakable (this is default behavior)
#define FEATURE_ITEM_INSTANT_DROP 1

#endif
