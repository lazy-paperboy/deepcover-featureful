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
#if !defined(REF_PARAMS_H)
#define REF_PARAMS_H

typedef struct ref_params_s
{
	// output
	float vieworg[3];
	float viewangles[3];

	float forward[3];
	float right[3];
	float up[3];
	
	// Client frametime;
	float		frametime;
	// Client time
	float		time;

	// Misc
	int		intermission;
	int		paused;
	int		spectator;
	int		onground;
	int		waterlevel;

	float simvel[3];
	float simorg[3];

	float viewheight[3];
	float		idealpitch;

	float cl_viewangles[3];
	int		health;
	float crosshairangle[3];
	float		viewsize;

	float punchangle[3];
	int		maxclients;
	int		viewentity;
	int		playernum;
	int		max_entities;
	int		demoplayback;	
	int		hardware;
	int		smoothing;

	// Last issued usercmd
	struct usercmd_s	*cmd;

	// Movevars
	struct movevars_s	*movevars;

	int		viewport[4];	// the viewport coordinates x, y, width, height
	int		nextView;		// the renderer calls ClientDLL_CalcRefdef() and Renderview
					// so long in cycles until this value is 0 (multiple views)
	int		onlyClientDraw;	// if !=0 nothing is drawn by the engine except clientDraw functions
// Xash3D extension
	float		fov_x, fov_y;	// actual fov can be overrided on nextView
} ref_params_t;

// same as ref_params but for overview mode
typedef struct ref_overview_s
{
	float		origin[3];
	qboolean		rotated;

	float		xLeft;
	float		xRight;
	float		xTop;
	float		xBottom;
	float		zFar;
	float		zNear;
	float		flZoom;
} ref_overview_t;

#endif//REF_PARAMS_H
