/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file cg_draw_hud.c
 * @brief Draws the player's hud
 *
 */

#include "cg_local.h"

typedef enum
{
	FLAGS_MOVE_TIMERS  = BIT(0),
	FLAGS_REMOVE_RANKS = BIT(1),
	FLAGS_MOVE_POPUPS  = BIT(2)
} althud_flags;

typedef enum
{
	STYLE_NORMAL,
	STYLE_SIMPLE
} componentStyle;

#define SKILL_ICON_SIZE     14

#define SKILLS_X 112
#define SKILLS_Y 20

#define SKILL_BAR_OFFSET    (2 * SKILL_BAR_X_INDENT)
#define SKILL_BAR_X_INDENT  0
#define SKILL_BAR_Y_INDENT  6

#define SKILL_BAR_WIDTH     (SKILL_ICON_SIZE - SKILL_BAR_OFFSET)
#define SKILL_BAR_X         (SKILL_BAR_OFFSET + SKILL_BAR_X_INDENT + SKILLS_X)
#define SKILL_BAR_X_SCALE   (SKILL_ICON_SIZE + 2)
#define SKILL_ICON_X        (SKILL_BAR_OFFSET + SKILLS_X)
#define SKILL_ICON_X_SCALE  (SKILL_ICON_SIZE + 2)
#define SKILL_BAR_Y         (SKILL_BAR_Y_INDENT - SKILL_BAR_OFFSET - SKILLS_Y)
#define SKILL_BAR_Y_SCALE   (SKILL_ICON_SIZE + 2)
#define SKILL_ICON_Y        (-(SKILL_ICON_SIZE + 2) - SKILL_BAR_OFFSET - SKILLS_Y)

#define MAXHUDS 128

int           hudCount = 0;
hudStucture_t hudlist[MAXHUDS];

hudStucture_t *activehud;
hudStucture_t hud0;

lagometer_t lagometer;

/**
 * @brief Using the stringizing operator to save typing...
 */
#define HUDF(x) # x, offsetof(hudStucture_t, x), qfalse

typedef struct
{
	const char *name;
	size_t offset;
	qboolean isAlias;
} hudComponentFields_t;

/**
* @var hudComponentFields
* @brief for accessing hudStucture_t's fields in a loop
*/
static const hudComponentFields_t hudComponentFields[] =
{
	{ HUDF(compass)          },
	{ "compas", offsetof(hudStucture_t, compass), qtrue}, // v2.78 backward compatibility
	{ HUDF(staminabar)       },
	{ HUDF(breathbar)        },
	{ HUDF(healthbar)        },
	{ HUDF(weaponchargebar)  },
	{ "weaponchangebar", offsetof(hudStucture_t, weaponchargebar), qtrue}, // v2.78 backward compatibility
	{ HUDF(healthtext)       },
	{ HUDF(xptext)           },
	{ HUDF(ranktext)         },
	{ HUDF(statsdisplay)     },
	{ HUDF(weaponicon)       },
	{ HUDF(weaponammo)       },
	{ HUDF(fireteam)         },
	{ HUDF(popupmessages)    },
	{ HUDF(powerups)         },
	{ HUDF(objectives)       },
	{ HUDF(hudhead)          },
	{ HUDF(cursorhints)      },
	{ HUDF(weaponstability)  },
	{ HUDF(livesleft)        },
	{ HUDF(roundtimer)       },
	{ HUDF(reinforcement)    },
	{ HUDF(spawntimer)       },
	{ HUDF(localtime)        },
	{ HUDF(votetext)         },
	{ HUDF(spectatortext)    },
	{ HUDF(limbotext)        },
	{ HUDF(followtext)       },
	{ HUDF(demotext)         },
	{ HUDF(missilecamera)    },
	{ HUDF(sprinttext)       },
	{ HUDF(breathtext)       },
	{ HUDF(weaponchargetext) },
	{ NULL, 0                },
};

/*
 * @brief CG_getRect
 * @param x
 * @param y
 * @param w
 * @param h
 * @return
 * @note Unused
rectDef_t CG_getRect(float x, float y, float w, float h)
{
    rectDef_t rect = { x, y, w, h };

    return rect;
}
*/

/**
 * @brief CG_getActiveHUD Returns reference to an active hud structure.
 * @return
 */
hudStucture_t *CG_GetActiveHUD()
{
	return activehud;
}

/**
 * @brief CG_getComponent
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] visible
 * @param[in] style
 * @return
 */
hudComponent_t CG_getComponent(float x, float y, float w, float h, qboolean visible, componentStyle style, float scale, const vec4_t color)
{
	hudComponent_t comp = { { x, y, w, h }, visible, style, scale };
	comp.color[0] = color[0];
	comp.color[1] = color[1];
	comp.color[2] = color[2];
	comp.color[3] = color[3];
	return comp;
}

vec4_t HUD_Background = { 0.16f, 0.2f, 0.17f, 0.5f };
vec4_t HUD_Border     = { 0.5f, 0.5f, 0.5f, 0.5f };
vec4_t HUD_Text       = { 0.6f, 0.6f, 0.6f, 1.0f };

/**
 * @brief CG_setDefaultHudValues
 * @param[out] hud
 */
void CG_setDefaultHudValues(hudStucture_t *hud)
{
	// the Default hud
	hud->hudnumber        = 0;
	hud->compass          = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 100 - 20 - 16, 16, 100 + 32, 100 + 32, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->staminabar       = CG_getComponent(4, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->breathbar        = CG_getComponent(4, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->healthbar        = CG_getComponent(24, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->weaponchargebar  = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 16, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->healthtext       = CG_getComponent(SKILLS_X - 28, SCREEN_HEIGHT - 4, 0, 0, qtrue, STYLE_NORMAL, 0.25f, colorWhite);
	hud->xptext           = CG_getComponent(SKILLS_X + 28, SCREEN_HEIGHT - 4, 0, 0, qtrue, STYLE_NORMAL, 0.25f, colorWhite);
	hud->ranktext         = CG_getComponent(0, SCREEN_HEIGHT, 0, 0, qfalse, STYLE_NORMAL, 0.2f, colorWhite);   // disable
	hud->statsdisplay     = CG_getComponent(SKILL_ICON_X, 0, 0, 0, qtrue, STYLE_NORMAL, 0.25f, colorWhite);
	hud->weaponicon       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 82, SCREEN_HEIGHT - 56, 60, 32, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->weaponammo       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 22, SCREEN_HEIGHT - 1 * (16 + 2) + 12 - 4, 0, 0, qtrue, STYLE_NORMAL, 0.25f, colorWhite);
	hud->fireteam         = CG_getComponent(10, 10, 100, 100, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->popupmessages    = CG_getComponent(4, 320, 72, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->powerups         = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 40, SCREEN_HEIGHT - 136, 36, 36, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->objectives       = CG_getComponent(8, SCREEN_HEIGHT - 136, 36, 36, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->hudhead          = CG_getComponent(44, SCREEN_HEIGHT - 92, 62, 80, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->cursorhints      = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) * .5f - 24, 260, 48, 48, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->weaponstability  = CG_getComponent(50, 208, 10, 64, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->livesleft        = CG_getComponent(0, 0, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->reinforcement    = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 60, SCREEN_HEIGHT - 70, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorLtBlue);
	hud->roundtimer       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 55, SCREEN_HEIGHT - 70, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->spawntimer       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 60, SCREEN_HEIGHT - 60, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorRed);
	hud->localtime        = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 55, SCREEN_HEIGHT - 60, 0, 0, qtrue, STYLE_NORMAL, 0.19f, HUD_Text);
	hud->votetext         = CG_getComponent(8, 224, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->spectatortext    = CG_getComponent(8, 188, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->limbotext        = CG_getComponent(8, 164, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->followtext       = CG_getComponent(8, 164, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud->demotext         = CG_getComponent(10, 9, 0.22f, 0.22f, qtrue, STYLE_SIMPLE, 0.22f, colorRed);
	hud->missilecamera    = CG_getComponent(4, 120, 160, 120, qtrue, STYLE_NORMAL, 1, colorWhite);
	hud->sprinttext       = CG_getComponent(20, SCREEN_HEIGHT - 96, 0, 0, qfalse, STYLE_NORMAL, 0.25f, colorWhite);
	hud->breathtext       = CG_getComponent(20, SCREEN_HEIGHT - 96, 0, 0, qfalse, STYLE_NORMAL, 0.25, colorWhite);
	hud->weaponchargetext = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 16, SCREEN_HEIGHT - 96, 0, 0, qfalse, STYLE_NORMAL, 0.25f, colorWhite);
}

/**
 * @brief CG_getHudByNumber
 * @param[in] number
 * @return
 */
static hudStucture_t *CG_getHudByNumber(int number)
{
	int           i;
	hudStucture_t *hud;

	for (i = 0; i < hudCount; i++)
	{
		hud = &hudlist[i];

		if (hud->hudnumber == number)
		{
			return hud;
		}
	}

	return NULL;
}

/**
 * @brief CG_isHudNumberAvailable checks if the hud by said number is available for use, 0 to 2 are forbidden.
 * @param[in] number
 * @return
 */
static qboolean CG_isHudNumberAvailable(int number)
{
	// values from 0 to 2 are used by the default hud's
	if (number <= 2 || number >= MAXHUDS)
	{
		Com_Printf(S_COLOR_RED "CG_isHudNumberAvailable: invalid HUD number %i, allowed values: 3 - %i\n", number, MAXHUDS);
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief CG_addHudToList
 * @param[in] hud
 */
static hudStucture_t *CG_addHudToList(hudStucture_t *hud)
{
	hudStucture_t *out = NULL;

	hudlist[hudCount] = *hud;
	out               = &hudlist[hudCount];
	hudCount++;
	return out;
}

//  HUD SCRIPT FUNCTIONS BELLOW

/**
 * @brief CG_HUD_ParseError
 * @param[in] handle
 * @param[in] format
 * @return
 */
static qboolean CG_HUD_ParseError(int handle, const char *format, ...)
{
	int         line;
	char        filename[MAX_QPATH];
	va_list     argptr;
	static char string[4096];

	va_start(argptr, format);
	Q_vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	filename[0] = '\0';
	line        = 0;
	trap_PC_SourceFileAndLine(handle, filename, &line);

	Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);

	trap_PC_FreeSource(handle);

	return qfalse;
}

/**
 * @brief CG_RectParse
 * @param[in] handle
 * @param[in,out] r
 * @return
 */
static qboolean CG_RectParse(int handle, rectDef_t *r)
{
	float      x = 0;
	pc_token_t peakedToken;

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == '(')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	if (PC_Float_Parse(handle, &x))
	{
		r->x = Ccg_WideX(x);
		if (PC_Float_Parse(handle, &r->y))
		{
			if (PC_Float_Parse(handle, &r->w))
			{
				if (PC_Float_Parse(handle, &r->h))
				{
					return qtrue;
				}
			}
		}
	}

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == ')')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	return qfalse;
}

static qboolean CG_Vec4Parse(int handle, vec4_t v)
{
	float      r, g, b, a = 0;
	pc_token_t peakedToken;

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == '(')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	if (PC_Float_Parse(handle, &r))
	{
		if (PC_Float_Parse(handle, &g))
		{
			if (PC_Float_Parse(handle, &b))
			{
				if (PC_Float_Parse(handle, &a))
				{
					v[0] = r;
					v[1] = g;
					v[2] = b;
					v[3] = a;
					return qtrue;
				}
			}
		}
	}

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == ')')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	return qfalse;
}

/**
 * @brief CG_ParseHudComponent
 * @param[in] handle
 * @param[in] comp
 * @return
 */
static qboolean CG_ParseHudComponent(int handle, hudComponent_t *comp)
{
	//PC_Rect_Parse
	if (!CG_RectParse(handle, &comp->location))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->style))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->visible))
	{
		return qfalse;
	}

	// Optional scale and color
	pc_token_t token;
	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}
	if (token.type == TT_NUMBER)
	{
		trap_PC_UnReadToken(handle);
		if (!PC_Float_Parse(handle, &comp->scale))
		{
			return qfalse;
		}

		if (!CG_Vec4Parse(handle, comp->color))
		{
			return qfalse;
		}
	}
	else
	{
		trap_PC_UnReadToken(handle);
	}

	return qtrue;
}

static int QDECL CG_HudComponentSort(const void *a, const void *b)
{
	return ((*(hudComponent_t **) a)->offset - (*(hudComponent_t **) b)->offset);
}

/**
 * @brief CG_ParseHUD
 * @param[in] handle
 * @return
 */
static qboolean CG_ParseHUD(int handle)
{
	int           i, componentOffset = 0;
	pc_token_t    token;
	hudStucture_t temphud;
	hudStucture_t *hud;
	qboolean      loadDefaults = qtrue;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_HUD_ParseError(handle, "expected '{'");
	}

	if (!trap_PC_ReadToken(handle, &token))
	{
		return CG_HUD_ParseError(handle, "Error while parsing hud");
	}

	// if the first parameter in the hud definition is a "no-defaults" line then no default values are set
	// and the hud is plain (everything is hidden and no positions are set)
	if (!Q_stricmp(token.string, "no-defaults"))
	{
		loadDefaults = qfalse;
	}
	else
	{
		trap_PC_UnReadToken(handle);
	}

	// reset all the components, and set the offset value to 999 for sorting
	Com_Memset(&temphud, 0, sizeof(hudStucture_t));

	if (loadDefaults)
	{
		CG_setDefaultHudValues(&temphud);
	}
	else
	{
		for (i = 0; hudComponentFields[i].name; i++)
		{
			hudComponent_t *component = (hudComponent_t *)((char * )&temphud + hudComponentFields[i].offset);
			component->offset = 999;
		}
	}

	componentOffset = 0;
	while (qtrue)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "hudnumber"))
		{
			if (!PC_Int_Parse(handle, &temphud.hudnumber))
			{
				return CG_HUD_ParseError(handle, "expected integer value for hudnumber");
			}

			continue;
		}

		for (i = 0; hudComponentFields[i].name; i++)
		{
			if (!Q_stricmp(token.string, hudComponentFields[i].name))
			{
				hudComponent_t *component = (hudComponent_t *)((char * )&temphud + hudComponentFields[i].offset);
				component->offset = componentOffset++;
				if (!CG_ParseHudComponent(handle, component))
				{
					return CG_HUD_ParseError(handle, "expected %s", hudComponentFields[i].name);
				}
				break;
			}
		}

		if (!hudComponentFields[i].name)
		{
			return CG_HUD_ParseError(handle, "unexpected token: %s", token.string);
		}
	}

	// check that the hudnumber value was set
	if (!CG_isHudNumberAvailable(temphud.hudnumber))
	{
		return CG_HUD_ParseError(handle, "Invalid hudnumber value: %i", temphud.hudnumber);
	}

	hud = CG_getHudByNumber(temphud.hudnumber);

	if (!hud)
	{
		hud = CG_addHudToList(&temphud);
		Com_Printf("...properties for hud %i have been read.\n", temphud.hudnumber);
	}
	else
	{
		Com_Memcpy(hud, &temphud, sizeof(temphud));
		Com_Printf("...properties for hud %i have been updated.\n", temphud.hudnumber);
	}

	// setup component pointers to the components list
	for (i = 0, componentOffset = 0; hudComponentFields[i].name; i++)
	{
		if (hudComponentFields[i].isAlias)
		{
			continue;
		}
		hud->components[componentOffset++] = (hudComponent_t *)((char * )hud + hudComponentFields[i].offset);
	}
	// sort the components by their offset
	qsort(hud->components, sizeof(hud->components) / sizeof(hudComponent_t *), sizeof(hudComponent_t *), CG_HudComponentSort);

	return qtrue;
}

static qboolean CG_ReadHudFile(const char *filename)
{
	pc_token_t token;
	int        handle;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "hudDef"))
	{
		return CG_HUD_ParseError(handle, "expected 'hudDef'");
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_HUD_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "hud"))
		{
			if (!CG_ParseHUD(handle))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_HUD_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	trap_PC_FreeSource(handle);

	return qtrue;
}

/**
 * @brief CG_ReadHudScripts
 */
void CG_ReadHudScripts(void)
{
	if (!CG_ReadHudFile("ui/huds.hud"))
	{
		Com_Printf("^1ERROR while reading hud file\n");
	}

	// This needs to be a .dat file to go around the file extension restrictions of the engine.
	CG_ReadHudFile("hud.dat");

	Com_Printf("...hud count: %i\n", hudCount);
}

// HUD DRAWING FUNCTIONS BELLOW

/**
 * @brief CG_DrawPicShadowed
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] icon
 */
static void CG_DrawPicShadowed(float x, float y, float w, float h, qhandle_t icon)
{
	trap_R_SetColor(colorBlack);
	CG_DrawPic(x + 2, y + 2, w, h, icon);
	trap_R_SetColor(NULL);
	CG_DrawPic(x, y, w, h, icon);
}

/**
 * @brief CG_DrawPlayerStatusHead
 * @param[in] comp
 */
static void CG_DrawPlayerStatusHead(hudComponent_t comp)
{
	hudHeadAnimNumber_t anim           = cg.idleAnim;
	bg_character_t      *character     = CG_CharacterForPlayerstate(&cg.snap->ps);
	bg_character_t      *headcharacter = BG_GetCharacter(cgs.clientinfo[cg.snap->ps.clientNum].team, cgs.clientinfo[cg.snap->ps.clientNum].cls);
	qhandle_t           painshader     = 0;
	rectDef_t           *headRect      = &comp.location;

	if (!comp.visible)
	{
		return;
	}

	if (cg.weaponFireTime > 500)
	{
		anim = HD_ATTACK;
	}
	else if (cg.time - cg.lastFiredWeaponTime < 500)
	{
		anim = HD_ATTACK_END;
	}
	else if (cg.time - cg.painTime < (character->hudheadanimations[HD_PAIN].numFrames * character->hudheadanimations[HD_PAIN].frameLerp))
	{
		anim = HD_PAIN;
	}
	else if (cg.time > cg.nextIdleTime)
	{
		cg.nextIdleTime = cg.time + 7000 + rand() % 1000;
		if (cg.snap->ps.stats[STAT_HEALTH] < 40)
		{
			cg.idleAnim = (hudHeadAnimNumber_t)((rand() % (HD_DAMAGED_IDLE3 - HD_DAMAGED_IDLE2 + 1)) + HD_DAMAGED_IDLE2);
		}
		else
		{
			cg.idleAnim = (hudHeadAnimNumber_t)((rand() % (HD_IDLE8 - HD_IDLE2 + 1)) + HD_IDLE2);
		}

		cg.lastIdleTimeEnd = cg.time + character->hudheadanimations[cg.idleAnim].numFrames * character->hudheadanimations[cg.idleAnim].frameLerp;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] < 5)
	{
		painshader = cgs.media.hudDamagedStates[3];
	}
	else if (cg.snap->ps.stats[STAT_HEALTH] < 20)
	{
		painshader = cgs.media.hudDamagedStates[2];
	}
	else if (cg.snap->ps.stats[STAT_HEALTH] < 40)
	{
		painshader = cgs.media.hudDamagedStates[1];
	}
	else if (cg.snap->ps.stats[STAT_HEALTH] < 60)
	{
		painshader = cgs.media.hudDamagedStates[0];
	}

	if (cg.time > cg.lastIdleTimeEnd)
	{
		if (cg.snap->ps.stats[STAT_HEALTH] < 40)
		{
			cg.idleAnim = HD_DAMAGED_IDLE1;
		}
		else
		{
			cg.idleAnim = HD_IDLE1;
		}
	}

	CG_DrawPlayerHead(headRect, character, headcharacter, 180, 0, (cg.snap->ps.eFlags & EF_HEADSHOT) ? qfalse : qtrue, anim, painshader, cgs.clientinfo[cg.snap->ps.clientNum].rank, qfalse, cgs.clientinfo[cg.snap->ps.clientNum].team);
}

/**
 * @brief Get the current ammo and/or clip count of the holded weapon (if using ammo).
 * @param[out] ammo - the number of ammo left (in the current clip if using clip)
 * @param[out] clips - the total ammount of ammo in all clips (if using clip)
 * @param[out] akimboammo - the number of ammo left in the second pistol of akimbo (if using akimbo)
 */
void CG_PlayerAmmoValue(int *ammo, int *clips, int *akimboammo)
{
	centity_t     *cent;
	playerState_t *ps;
	weapon_t      weap;

	*ammo = *clips = *akimboammo = -1;

	if (cg.snap->ps.clientNum == cg.clientNum)
	{
		cent = &cg.predictedPlayerEntity;
	}
	else
	{
		cent = &cg_entities[cg.snap->ps.clientNum];
	}
	ps = &cg.snap->ps;

	weap = (weapon_t)cent->currentState.weapon;

	if (!IS_VALID_WEAPON(weap))
	{
		return;
	}

	// some weapons don't draw ammo count
	if (!GetWeaponTableData(weap)->useAmmo)
	{
		return;
	}

	if (BG_PlayerMounted(cg.snap->ps.eFlags))
	{
		return;
	}

	// total ammo in clips, grenade launcher is not a clip weapon but show the clip anyway
	if (GetWeaponTableData(weap)->useClip || (weap == WP_M7 || weap == WP_GPG40))
	{
		// current reserve
		*clips = cg.snap->ps.ammo[GetWeaponTableData(weap)->ammoIndex];

		// current clip
		*ammo = ps->ammoclip[GetWeaponTableData(weap)->clipIndex];
	}
	else
	{
		// some weapons don't draw ammo clip count text
		*ammo = ps->ammoclip[GetWeaponTableData(weap)->clipIndex] + cg.snap->ps.ammo[GetWeaponTableData(weap)->ammoIndex];
	}

	// akimbo ammo clip
	if (GetWeaponTableData(weap)->attributes & WEAPON_ATTRIBUT_AKIMBO)
	{
		*akimboammo = ps->ammoclip[GetWeaponTableData(GetWeaponTableData(weap)->akimboSideArm)->clipIndex];
	}
	else
	{
		*akimboammo = -1;
	}

	if (weap == WP_LANDMINE)
	{
		if (!cgs.gameManager)
		{
			*ammo = 0;
		}
		else
		{
			if (cgs.clientinfo[ps->clientNum].team == TEAM_AXIS)
			{
				*ammo = cgs.gameManager->currentState.otherEntityNum;
			}
			else
			{
				*ammo = cgs.gameManager->currentState.otherEntityNum2;
			}
		}
	}
}

vec4_t bgcolor = { 1.f, 1.f, 1.f, .3f };    // bars backgound

/**
 * @brief CG_DrawPlayerHealthBar
 * @param[in] rect
 */
static void CG_DrawPlayerHealthBar(rectDef_t *rect)
{
	vec4_t colour;
	int    flags = 1 | 4 | 16 | 64;
	float  frac;

	CG_ColorForHealth(colour);
	colour[3] = 0.5f;

	frac = cg.snap->ps.stats[STAT_HEALTH] / (float) cg.snap->ps.stats[STAT_MAX_HEALTH];

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, colour, NULL, bgcolor, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);
	CG_DrawPic(rect->x, rect->y + rect->h + 4, rect->w, rect->w, cgs.media.hudHealthIcon);
}

/**
 * @brief CG_DrawStaminaBar
 * @param[in] rect
 */
static void CG_DrawStaminaBar(rectDef_t *rect)
{
	vec4_t colour = { 0.1f, 1.0f, 0.1f, 0.5f };
	vec_t  *color = colour;
	int    flags  = 1 | 4 | 16 | 64;
	float  frac   = cg.snap->ps.stats[STAT_SPRINTTIME] / (float)SPRINTTIME;

	if (cg.snap->ps.powerups[PW_ADRENALINE])
	{
		if (cg.snap->ps.pm_flags & PMF_FOLLOW)
		{
			Vector4Average(colour, colorWhite, (float)sin(cg.time * .005), colour);
		}
		else
		{
			float msec = cg.snap->ps.powerups[PW_ADRENALINE] - cg.time;

			if (msec >= 0)
			{
				Vector4Average(colour, colorMdRed, (float)(.5 + sin(.2 * sqrt((double)msec) * M_TAU_F) * .5), colour);
			}
		}
	}
	else
	{
		color[0] = 1.0f - frac;
		color[1] = frac;
	}

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, color, NULL, bgcolor, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);
	CG_DrawPic(rect->x, rect->y + rect->h + 4, rect->w, rect->w, cgs.media.hudSprintIcon);
}

/**
 * @brief Draw the breath bar
 * @param[in] rect
 */
static void CG_DrawBreathBar(rectDef_t *rect)
{
	static vec4_t colour = { 0.1f, 0.1f, 1.0f, 0.5f };
	vec_t         *color = colour;
	int           flags  = 1 | 4 | 16 | 64;
	float         frac   = cg.snap->ps.stats[STAT_AIRLEFT] / (float)HOLDBREATHTIME;

	color[0] = 1.0f - frac;
	color[2] = frac;

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, color, NULL, bgcolor, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);
	CG_DrawPic(rect->x, rect->y + rect->h + 4, rect->w, rect->w, cgs.media.waterHintShader);
}

/**
 * @brief Draw weapon recharge bar
 * @param rect
 */
static void CG_DrawWeapRecharge(rectDef_t *rect)
{
	float    barFrac, chargeTime;
	int      flags  = 1 | 4 | 16;
	qboolean charge = qtrue;
	vec4_t   color;

	// Draw power bar
	switch (cg.snap->ps.stats[STAT_PLAYER_CLASS])
	{
	case PC_ENGINEER:
		chargeTime = cg.engineerChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_MEDIC:
		chargeTime = cg.medicChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_FIELDOPS:
		chargeTime = cg.fieldopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_COVERTOPS:
		chargeTime = cg.covertopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	default:
		chargeTime = cg.soldierChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	}

	// display colored charge bar if charge bar isn't full enough
	if (GetWeaponTableData(cg.predictedPlayerState.weapon)->attributes & WEAPON_ATTRIBUT_CHARGE_TIME)
	{
		int index = BG_IsSkillAvailable(cgs.clientinfo[cg.clientNum].skill,
		                                GetWeaponTableData(cg.predictedPlayerState.weapon)->skillBased,
		                                GetWeaponTableData(cg.predictedPlayerState.weapon)->chargeTimeSkill);

		float coeff = GetWeaponTableData(cg.predictedPlayerState.weapon)->chargeTimeCoeff[index];

		if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * coeff)
		{
			charge = qfalse;
		}
	}
	else if ((cg.predictedPlayerState.eFlags & EF_ZOOMING || cg.predictedPlayerState.weapon == WP_BINOCULARS)
	         && cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_FIELDOPS)
	{
		int index = BG_IsSkillAvailable(cgs.clientinfo[cg.clientNum].skill,
		                                GetWeaponTableData(WP_ARTY)->skillBased,
		                                GetWeaponTableData(WP_ARTY)->chargeTimeSkill);

		float coeff = GetWeaponTableData(WP_ARTY)->chargeTimeCoeff[index];

		if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * coeff)
		{
			charge = qfalse;
		}
	}

	barFrac = (cg.time - cg.snap->ps.classWeaponTime) / chargeTime; // FIXME: potential DIV 0 when charge times are set to 0!

	if (barFrac > 1.0f)
	{
		barFrac = 1.0f;
	}

	if (!charge)
	{
		color[0] = 1.0f;
		color[1] = color[2] = 0.1f;
		color[3] = 0.5f;
	}
	else
	{
		color[0] = color[1] = 1.0f;
		color[2] = barFrac;
		color[3] = 0.25f + barFrac * 0.5f;
	}

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, color, NULL, bgcolor, barFrac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);

	if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_FIELDOPS)
	{
		if (cg.snap->ps.ammo[WP_ARTY] & NO_AIRSTRIKE && cg.snap->ps.ammo[WP_ARTY] & NO_ARTILLERY)
		{
			trap_R_SetColor(colorRed);
		}
		else if (cg.snap->ps.ammo[WP_ARTY] & NO_AIRSTRIKE)
		{
			trap_R_SetColor(colorOrange);
		}
		else if (cg.snap->ps.ammo[WP_ARTY] & NO_ARTILLERY)
		{
			trap_R_SetColor(colorYellow);
		}
		CG_DrawPic(rect->x + (rect->w * 0.25f) - 1, rect->y + rect->h + 4, (rect->w * 0.5f) + 2, rect->w + 2, cgs.media.hudPowerIcon);
		trap_R_SetColor(NULL);
	}
	else
	{
		CG_DrawPic(rect->x + (rect->w * 0.25f) - 1, rect->y + rect->h + 4, (rect->w * 0.5f) + 2, rect->w + 2, cgs.media.hudPowerIcon);
	}
}

/**
 * @brief CG_DrawGunIcon
 * @param[in] location
 */
static void CG_DrawGunIcon(rectDef_t location)
{
	rectDef_t rect = location;

	// Draw weapon icon and overheat bar
	CG_DrawWeapHeat(&rect, HUD_HORIZONTAL);

	// drawn the common white icon, usage of mounted weapons don't change cg.snap->ps.weapon for real
	if (BG_PlayerMounted(cg.snap->ps.eFlags))
	{
		CG_DrawPlayerWeaponIcon(&rect, qtrue, ITEM_ALIGN_RIGHT, &activehud->weaponicon.color);
		return;
	}

	if (
#ifdef FEATURE_MULTIVIEW
		cg.mvTotalClients < 1 &&
#endif
		cg_drawWeaponIconFlash.integer == 0)
	{
		CG_DrawPlayerWeaponIcon(&rect, qtrue, ITEM_ALIGN_RIGHT, &activehud->weaponicon.color);
	}
	else
	{
		int ws =
#ifdef FEATURE_MULTIVIEW
			(cg.mvTotalClients > 0) ? cgs.clientinfo[cg.snap->ps.clientNum].weaponState :
#endif
			BG_simpleWeaponState(cg.snap->ps.weaponstate);

		CG_DrawPlayerWeaponIcon(&rect, (qboolean)(ws != WSTATE_IDLE), ITEM_ALIGN_RIGHT, ((ws == WSTATE_SWITCH || ws == WSTATE_RELOAD) ? &colorYellow : (ws == WSTATE_FIRE) ? &colorRed : &activehud->weaponicon.color));
	}
}

/**
 * @brief CG_DrawAmmoCount
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawAmmoCount(float x, float y)
{
	int   value, value2, value3;
	float scale = activehud->weaponammo.scale;
	char  buffer[16];

	// Draw ammo
	CG_PlayerAmmoValue(&value, &value2, &value3);

	// .25f
	if (value3 >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i|%i/%i", value3, value, value2);
		CG_Text_Paint_Ext(x - CG_Text_Width_Ext(buffer, scale, 0, &cgs.media.limboFont1), y, scale, scale, activehud->weaponammo.color, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
	else if (value2 >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i/%i", value, value2);
		CG_Text_Paint_Ext(x - CG_Text_Width_Ext(buffer, scale, 0, &cgs.media.limboFont1), y, scale, scale, activehud->weaponammo.color, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
	else if (value >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i", value);
		CG_Text_Paint_Ext(x - CG_Text_Width_Ext(buffer, scale, 0, &cgs.media.limboFont1), y, scale, scale, activehud->weaponammo.color, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
}

/**
 * @brief CG_DrawSkillBar
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] skillLvl
 */
static void CG_DrawSkillBar(float x, float y, float w, float h, int skillLvl, skillType_t skill)
{
	int    i;
	float  blockheight = (h - 4) / (float)(NUM_SKILL_LEVELS - 1);
	float  draw_y      = y + h - blockheight;
	vec4_t colour;
	float  x1, y1, w1, h1;

	for (i = 1; i < NUM_SKILL_LEVELS; i++)
	{

		if (GetSkillTableData(skill)->skillLevels[i] < 0)
		{
			Vector4Set(colour, 1.f, 0.f, 0.f, .15f);
		}
		else if (skillLvl >= i)
		{
			Vector4Set(colour, 0.f, 0.f, 0.f, .4f);
		}
		else
		{
			Vector4Set(colour, 1.f, 1.f, 1.f, .15f);
		}

		CG_FillRect(x, draw_y, w, blockheight, colour);

		// draw the star only if the skill is reach and available
		if (skillLvl >= i && GetSkillTableData(skill)->skillLevels[i] >= 0)
		{
			x1 = x;
			y1 = draw_y;
			w1 = w;
			h1 = blockheight;
			CG_AdjustFrom640(&x1, &y1, &w1, &h1);

			trap_R_DrawStretchPic(x1, y1, w1, h1, 0, 0, 1.f, 0.5f, cgs.media.limboStar_roll);
		}

		CG_DrawRect_FixedBorder(x, draw_y, w, blockheight, 1, colorBlack);
		draw_y -= (blockheight + 1);
	}
}

/**
 * @brief CG_ClassSkillForPosition
 * @param[in] ci
 * @param[in] pos
 * @return
 */
skillType_t CG_ClassSkillForPosition(clientInfo_t *ci, int pos)
{
	switch (pos)
	{
	case 0:
		return BG_ClassSkillForClass(ci->cls);
	case 1:
		return SK_BATTLE_SENSE;
	case 2:
		// draw soldier level if using a heavy weapon instead of light weapons icon
		if ((BG_PlayerMounted(cg.snap->ps.eFlags) || GetWeaponTableData(cg.snap->ps.weapon)->skillBased == SK_HEAVY_WEAPONS) && ci->cls != PC_SOLDIER)
		{
			return SK_HEAVY_WEAPONS;
		}
		return SK_LIGHT_WEAPONS;
	default:
		break;
	}

	return SK_BATTLE_SENSE;
}

/**
 * @brief CG_DrawPlayerHealth
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawPlayerHealth(float x, float y)
{
	const char *str  = va("%i", cg.snap->ps.stats[STAT_HEALTH]);
	float      scale = activehud->healthtext.scale;
	float      w     = CG_Text_Width_Ext(str, scale, 0, &cgs.media.limboFont1);
	vec4_t     color;

	if (cg_healthDynamicColor.integer)
	{
		int    clientNum = cg.snap->ps.clientNum;
		int    cls       = cg.snap->ps.stats[STAT_PLAYER_CLASS];
		team_t team      = cg.snap->ps.persistant[PERS_TEAM];
		int    maxHealth = CG_GetPlayerMaxHealth(clientNum, cls, team);
		CG_GetColorForHealth(cg.snap->ps.stats[STAT_HEALTH], color);
		color[3] = activehud->healthtext.color[3];
	}
	else
	{
		Vector4Copy(activehud->healthtext.color, color);
	}

	CG_Text_Paint_Ext(x - w, y, scale, scale, color, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + 2, y, scale - 0.05f, scale - 0.05f, activehud->healthtext.color, "HP", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawPlayerSprint
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawPlayerSprint(float x, float y)
{
	const char *str;
	const char *unit;
	float      scale = activehud->sprinttext.scale;
	float      w;

	if (cg.snap->ps.powerups[PW_ADRENALINE])
	{
		str  = va("%d", (cg.snap->ps.powerups[PW_ADRENALINE] - cg.time) / 1000);
		unit = "s";
	}
	else
	{
		str  = va("%.0f", (cg.snap->ps.stats[STAT_SPRINTTIME] / (float)SPRINTTIME) * 100);
		unit = "%";
	}

	w = CG_Text_Width_Ext(str, scale, 0, &cgs.media.limboFont1);

	CG_Text_Paint_Ext(x - w, y, scale, scale, activehud->sprinttext.color, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + 2, y, scale - 0.05f, scale - 0.05f, activehud->sprinttext.color, unit, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawPlayerBreath
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawPlayerBreath(float x, float y)
{
	const char *str  = va("%.0f", (cg.snap->ps.stats[STAT_AIRLEFT] / (float)HOLDBREATHTIME) * 100);
	float      scale = activehud->breathtext.scale;
	float      w     = CG_Text_Width_Ext(str, scale, 0, &cgs.media.limboFont1);

	CG_Text_Paint_Ext(x - w, y, scale, scale, activehud->breathtext.color, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + 2, y, scale - 0.05f, scale - 0.05f, activehud->breathtext.color, "%", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawWeaponCharge
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawWeaponCharge(float x, float y)
{
	const char *str;
	float      scale = activehud->weaponchargetext.scale;
	float      w;
	float      chargeTime;

	switch (cg.snap->ps.stats[STAT_PLAYER_CLASS])
	{
	case PC_ENGINEER:
		chargeTime = cg.engineerChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_MEDIC:
		chargeTime = cg.medicChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_FIELDOPS:
		chargeTime = cg.fieldopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_COVERTOPS:
		chargeTime = cg.covertopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	default:
		chargeTime = cg.soldierChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	}

	str = va("%.0f", MIN(((cg.time - cg.snap->ps.classWeaponTime) / chargeTime) * 100, 100));
	w   = CG_Text_Width_Ext(str, scale, 0, &cgs.media.limboFont1);


	CG_Text_Paint_Ext(x - w, y, scale, scale, activehud->weaponchargetext.color, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + 2, y, scale - 0.05f, scale - 0.05f, activehud->weaponchargetext.color, "%", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawSkills
 * @param[in] comp
 */
static void CG_DrawSkills(hudComponent_t comp)
{
	playerState_t *ps = &cg.snap->ps;
	clientInfo_t  *ci = &cgs.clientinfo[ps->clientNum];
	skillType_t   skill;
	int           i;
	float         temp;

	for (i = 0; i < 3; i++)
	{
		skill = CG_ClassSkillForPosition(ci, i);
		if (comp.style == STYLE_NORMAL)
		{
			CG_DrawSkillBar(i * SKILL_BAR_X_SCALE + SKILL_BAR_X, SCREEN_HEIGHT - (5 * SKILL_BAR_Y_SCALE) + SKILL_BAR_Y, SKILL_BAR_WIDTH, 4 * SKILL_ICON_SIZE, ci->skill[skill], skill);
			CG_DrawPic(i * SKILL_ICON_X_SCALE + SKILL_ICON_X, SCREEN_HEIGHT + SKILL_ICON_Y, SKILL_ICON_SIZE, SKILL_ICON_SIZE, cgs.media.skillPics[skill]);
		}
		else
		{
			int j        = 1;
			int skillLvl = 0;

			for (; j < NUM_SKILL_LEVELS; ++j)
			{
				if (BG_IsSkillAvailable(ci->skill, skill, j))
				{
					skillLvl++;
				}
			}

			temp = comp.location.y + (i * SKILL_ICON_SIZE * 1.7f);
			//CG_DrawPic
			CG_DrawPicShadowed(comp.location.x, temp, SKILL_ICON_SIZE, SKILL_ICON_SIZE, cgs.media.skillPics[skill]);
			CG_Text_Paint_Ext(comp.location.x + 3, temp + 24, comp.scale, comp.scale, comp.color, va("%i", skillLvl), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}
	}
}

/**
 * @brief CG_DrawXP
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawXP(float x, float y)
{
	const char *str;
	float      w, scale;
	vec_t      *clr;

	if (cg.time - cg.xpChangeTime < 1000)
	{
		clr = colorYellow;
	}
	else
	{
		clr = activehud->xptext.color;
	}

	str   = va("%i", cg.snap->ps.stats[STAT_XP]);
	scale = activehud->xptext.scale;
	w     = CG_Text_Width_Ext(str, scale, 0, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x - w, y, scale, scale, clr, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + 2, y, scale - 0.05f, scale - 0.05f, clr, "XP", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawRank
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawRank(float x, float y)
{
	const char    *str;
	float         w, scale;
	playerState_t *ps = &cg.snap->ps;

	str   = va("%s", GetRankTableData(cgs.clientinfo[ps->clientNum].team, cgs.clientinfo[ps->clientNum].rank)->miniNames);
	scale = activehud->ranktext.scale;
	w     = CG_Text_Width_Ext(str, scale, 0, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x - w, y, scale, scale, activehud->ranktext.color, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawPowerUps
 * @param[in] rect
 */
static void CG_DrawPowerUps(rectDef_t rect)
{
	playerState_t *ps = &cg.snap->ps;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR && !cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	// draw treasure icon if we have the flag
	if (ps->powerups[PW_REDFLAG] || ps->powerups[PW_BLUEFLAG])
	{
		trap_R_SetColor(NULL);
		CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveShader);
	}
	else if (ps->powerups[PW_OPS_DISGUISED])       // Disguised?
	{
		CG_DrawPic(rect.x, rect.y, rect.w, rect.h, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		// show the class to the client
		CG_DrawPic(rect.x + 9, rect.y + 9, 18, 18, cgs.media.skillPics[BG_ClassSkillForClass((cg_entities[ps->clientNum].currentState.powerups >> PW_OPS_CLASS_1) & 7)]);
	}
	else if (ps->powerups[PW_ADRENALINE] > 0)       // adrenaline
	{
		vec4_t color = { 1.0, 0.0, 0.0, 1.0 };
		color[3] *= 0.5 + 0.5 * sin(cg.time / 150.0);
		trap_R_SetColor(color);
		CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.hudAdrenaline);
		trap_R_SetColor(NULL);
	}
	else if (ps->powerups[PW_INVULNERABLE] && !(ps->pm_flags & PMF_LIMBO))       // spawn shield
	{
		CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.spawnInvincibleShader);
	}
}

/**
 * @brief CG_DrawObjectiveStatus
 * @param[in] rect
 */
static void CG_DrawObjectiveStatus(rectDef_t rect)
{
	playerState_t *ps = &cg.snap->ps;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR && !cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	// draw objective status icon
	if ((cg.flagIndicator & (1 << PW_REDFLAG) || cg.flagIndicator & (1 << PW_BLUEFLAG)) && (!cgs.clientinfo[cg.clientNum].shoutcaster || (cg.snap->ps.pm_flags & PMF_FOLLOW)))
	{
		// draw objective info icon (if teammates or enemies are carrying one)
		vec4_t color = { 1.f, 1.f, 1.f, 1.f };
		color[3] *= 0.67 + 0.33 * sin(cg.time / 200.0);
		trap_R_SetColor(color);

		if (cg.flagIndicator & (1 << PW_REDFLAG) && cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			if (cg.redFlagCounter > 0 && cg.blueFlagCounter > 0)
			{
				// both own and enemy flags stolen
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveBothTEShader);
			}
			else if (cg.redFlagCounter > 0 && !cg.blueFlagCounter)
			{
				// own flag stolen and enemy flag dropped
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.objectiveBothTDShader : cgs.media.objectiveBothDEShader);
			}
			else if (!cg.redFlagCounter && cg.blueFlagCounter > 0)
			{
				// own flag dropped and enemy flag stolen
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, ps->persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.objectiveBothTDShader : cgs.media.objectiveBothDEShader);
			}
			else
			{
				// both own and enemy flags dropped
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveDroppedShader);
			}
			trap_R_SetColor(NULL);

			// display team flag
			color[3] = 1.f;
			trap_R_SetColor(color);
			CG_DrawPic(rect.x + rect.w / 2 - 20, rect.y + 28, 12, 8, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.axisFlag : cgs.media.alliedFlag);
			CG_DrawPic(rect.x + rect.w / 2 + 8, rect.y + 28, 12, 8, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.alliedFlag : cgs.media.axisFlag);
		}
		else if (cg.flagIndicator & (1 << PW_REDFLAG))
		{
			if (cg.redFlagCounter > 0)
			{
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, ps->persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.objectiveTeamShader : cgs.media.objectiveEnemyShader);
			}
			else
			{
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveDroppedShader);
			}
			trap_R_SetColor(NULL);

			// display team flag
			color[3] = 1.f;
			trap_R_SetColor(color);
			CG_DrawPic(rect.x + rect.w / 2 + (ps->persistant[PERS_TEAM] == TEAM_AXIS ? 8 : -20), rect.y + 28, 12, 8, cgs.media.alliedFlag);
		}
		else if (cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			if (cg.blueFlagCounter > 0)
			{
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.objectiveTeamShader : cgs.media.objectiveEnemyShader);
			}
			else
			{
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveDroppedShader);
			}
			trap_R_SetColor(NULL);

			// display team flag
			color[3] = 1.f;
			trap_R_SetColor(color);
			CG_DrawPic(rect.x + rect.w / 2 + (ps->persistant[PERS_TEAM] == TEAM_ALLIES ? 8 : -20), rect.y + 28, 12, 8, cgs.media.axisFlag);
		}

		// display active flag counter
		if (cg.redFlagCounter > 1)
		{
			CG_Text_Paint_Ext(rect.x + rect.w / 2 + (ps->persistant[PERS_TEAM] == TEAM_ALLIES ? -16 : 12), rect.y + 38, 0.18, 0.18, colorWhite, va("%i", cg.redFlagCounter), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}
		if (cg.blueFlagCounter > 1)
		{
			CG_Text_Paint_Ext(rect.x + rect.w / 2 + (ps->persistant[PERS_TEAM] == TEAM_AXIS ? -16 : 12), rect.y + 38, 0.18, 0.18, colorWhite, va("%i", cg.blueFlagCounter), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}

		trap_R_SetColor(NULL);
	}
	else if (cgs.clientinfo[cg.clientNum].shoutcaster && !(cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		// simplified version for shoutcaster when not following players
		vec4_t color = { 1.f, 1.f, 1.f, 1.f };
		color[3] *= 0.67 + 0.33 * sin(cg.time / 200.0);
		trap_R_SetColor(color);

		if (cg.flagIndicator & (1 << PW_REDFLAG) && cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			if (cg.redFlagCounter > 0 && cg.blueFlagCounter > 0)
			{
				// both team stole an enemy flags
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveTeamShader);
			}
			else if ((cg.redFlagCounter > 0 && !cg.blueFlagCounter) || (!cg.redFlagCounter && cg.blueFlagCounter > 0))
			{
				// one flag stolen and the other flag dropped
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveTeamShader);
			}
			else
			{
				// both team dropped flags
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveDroppedShader);
			}
		}
		else if (cg.flagIndicator & (1 << PW_REDFLAG))
		{
			if (cg.redFlagCounter > 0)
			{
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveTeamShader);
			}
			else
			{
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveDroppedShader);
			}
		}
		else if (cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			if (cg.blueFlagCounter > 0)
			{
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveTeamShader);
			}
			else
			{
				CG_DrawPic(rect.x, rect.y, rect.w, rect.h, cgs.media.objectiveDroppedShader);
			}
		}
		trap_R_SetColor(NULL);

		// display team flag
		color[3] = 1.f;
		trap_R_SetColor(color);

		if (cg.flagIndicator & (1 << PW_REDFLAG))
		{
			CG_DrawPic(rect.x + rect.w / 2 + 8, rect.y + 28, 12, 8, cgs.media.alliedFlag);
		}

		if (cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			CG_DrawPic(rect.x + rect.w / 2 - 20, rect.y + 28, 12, 8, cgs.media.axisFlag);
		}

		// display active flag counter
		if (cg.redFlagCounter > 1)
		{
			CG_Text_Paint_Ext(rect.x + rect.w / 2 + 12, rect.y + 38, 0.18, 0.18, colorWhite, va("%i", cg.redFlagCounter), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}
		if (cg.blueFlagCounter > 1)
		{
			CG_Text_Paint_Ext(rect.x + rect.w / 2 - 16, rect.y + 38, 0.18, 0.18, colorWhite, va("%i", cg.blueFlagCounter), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}

		trap_R_SetColor(NULL);
	}
}

static int lastDemoScoreTime = 0;

/**
 * @brief CG_DrawDemoMessage
 */
void CG_DrawDemoMessage(void)
{
	char status[1024];
	char demostatus[128];
	char wavestatus[128];

	float         x, y, fontScale;
	hudStucture_t *hud;

	hud = CG_GetActiveHUD();

	fontScale = hud->demotext.scale;
	y         = hud->demotext.location.y;
	x         = hud->demotext.location.x;

	if (!hud->demotext.visible)
	{
		return;
	}

	if (!cl_demorecording.integer && !cl_waverecording.integer && !cg.demoPlayback)
	{
		return;
	}

	// poll for score
	if ((!lastDemoScoreTime || cg.time > lastDemoScoreTime) && !cg.demoPlayback)
	{
		trap_SendClientCommand("score");
		lastDemoScoreTime = cg.time + 5000; // 5 secs
	}

	if (hud->demotext.style == STYLE_NORMAL)
	{
		if (cl_demorecording.integer)
		{
			Com_sprintf(demostatus, sizeof(demostatus), __(" demo %s: %ik "), cl_demofilename.string, cl_demooffset.integer / 1024);
		}
		else
		{
			Q_strncpyz(demostatus, "", sizeof(demostatus));
		}

		if (cl_waverecording.integer)
		{
			Com_sprintf(wavestatus, sizeof(demostatus), __(" audio %s: %ik "), cl_wavefilename.string, cl_waveoffset.integer / 1024);
		}
		else
		{
			Q_strncpyz(wavestatus, "", sizeof(wavestatus));
		}
	}
	else
	{
		Q_strncpyz(demostatus, "", sizeof(demostatus));
		Q_strncpyz(wavestatus, "", sizeof(wavestatus));
	}

	Com_sprintf(status, sizeof(status), "%s%s%s", cg.demoPlayback ? __("REPLAY") : __("RECORD"), demostatus, wavestatus);

	CG_Text_Paint_Ext(x, y, fontScale, fontScale, cg.demoPlayback ? colorYellow : hud->demotext.color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
}

/**
 * @brief CG_DrawField
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] value
 * @param[in] charWidth
 * @param[in] charHeight
 * @param[in] dodrawpic
 * @param[in] leftAlign
 * @return
 */
int CG_DrawField(int x, int y, int width, int value, int charWidth, int charHeight, qboolean dodrawpic, qboolean leftAlign)
{
	char num[16], *ptr;
	int  l;
	int  frame;
	int  startx;

	if (width < 1)
	{
		return 0;
	}

	// draw number string
	if (width > 5)
	{
		width = 5;
	}

	switch (width)
	{
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf(num, sizeof(num), "%i", value);
	l = (int)strlen(num);
	if (l > width)
	{
		l = width;
	}

	if (!leftAlign)
	{
		x -= 2 + charWidth * (l);
	}

	startx = x;

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
		{
			frame = STAT_MINUS;
		}
		else
		{
			frame = *ptr - '0';
		}

		if (dodrawpic)
		{
			CG_DrawPic(x, y, charWidth, charHeight, cgs.media.numberShaders[frame]);
		}
		x += charWidth;
		ptr++;
		l--;
	}

	return startx;
}

/**
 * @brief CG_DrawLivesLeft
 * @param[in] comp - unused
 */
void CG_DrawLivesLeft(hudComponent_t comp)
{
	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] < 0)
	{
		return;
	}

	CG_DrawPic(4, 360, 48, 24, cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.hudAlliedHelmet : cgs.media.hudAxisHelmet);

	CG_DrawField(44, 360, 3, cg.snap->ps.persistant[PERS_RESPAWNS_LEFT], 14, 20, qtrue, qtrue);
}

static char statsDebugStrings[6][512];
static int  statsDebugTime[6];
static int  statsDebugTextWidth[6];
static int  statsDebugPos;

/**
 * @brief CG_InitStatsDebug
 */
void CG_InitStatsDebug(void)
{
	Com_Memset(&statsDebugStrings, 0, sizeof(statsDebugStrings));
	Com_Memset(&statsDebugTime, 0, sizeof(statsDebugTime));
	statsDebugPos = -1;
}

/**
 * @brief CG_StatsDebugAddText
 * @param[in] text
 */
void CG_StatsDebugAddText(const char *text)
{
	if (cg_debugSkills.integer)
	{
		statsDebugPos++;

		if (statsDebugPos >= 6)
		{
			statsDebugPos = 0;
		}

		Q_strncpyz(statsDebugStrings[statsDebugPos], text, 512);
		statsDebugTime[statsDebugPos]      = cg.time;
		statsDebugTextWidth[statsDebugPos] = CG_Text_Width_Ext(text, .15f, 0, &cgs.media.limboFont2);

		CG_Printf("%s\n", text);
	}
}

/**
 * @brief CG_GetCompassIcon
 * @param[in] ent
 * @param[in] drawAllVoicesChat get all icons voices chat, otherwise only request relevant icons voices chat (need medic/ammo ...)
 * @param[in] drawFireTeam draw fireteam members position
 * @param[in] drawPrimaryObj draw primary objective position
 * @param[in] drawSecondaryObj draw secondary objective position
 * @param[in] drawItemObj draw item objective position
 * @param[in] drawDynamic draw dynamic elements position (player revive, command map marker)
 * @return A valid compass icon handle otherwise 0
 */
qhandle_t CG_GetCompassIcon(entityState_t *ent, qboolean drawAllVoicesChat, qboolean drawFireTeam, qboolean drawPrimaryObj, qboolean drawSecondaryObj, qboolean drawItemObj, qboolean drawDynamic, char *name)
{
	centity_t *cent = &cg_entities[ent->number];

	if (!cent->currentValid)
	{
		return 0;
	}

	switch (ent->eType)
	{
	case ET_PLAYER:
	{
		qboolean sameTeam = cg.predictedPlayerState.persistant[PERS_TEAM] == cgs.clientinfo[ent->clientNum].team;

		if (!cgs.clientinfo[ent->clientNum].infoValid)
		{
			return 0;
		}

		if (sameTeam && cgs.clientinfo[ent->clientNum].powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
		{
			return cgs.media.objectiveShader;
		}

		if (ent->eFlags & EF_DEAD)
		{
			if (drawDynamic &&
			    ((cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_MEDIC &&
			      cg.predictedPlayerState.stats[STAT_HEALTH] > 0 && ent->number == ent->clientNum && sameTeam) ||
			     (!(cg.snap->ps.pm_flags & PMF_FOLLOW) && cgs.clientinfo[cg.clientNum].shoutcaster)))
			{
				return cgs.media.medicReviveShader;
			}

			return 0;
		}

		if (sameTeam && cent->voiceChatSpriteTime > cg.time &&
		    (drawAllVoicesChat ||
		     (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_MEDIC && cent->voiceChatSprite == cgs.media.medicIcon) ||
		     (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_FIELDOPS && cent->voiceChatSprite == cgs.media.ammoIcon)))
		{
			// FIXME: not the best place to reset it
			if (cgs.clientinfo[ent->clientNum].health <= 0)
			{
				// reset
				cent->voiceChatSpriteTime = cg.time;
				return 0;
			}

			return cent->voiceChatSprite;
		}

		if (drawFireTeam && (CG_IsOnSameFireteam(cg.clientNum, ent->clientNum) || cgs.clientinfo[cg.clientNum].shoutcaster))
		{
			// draw overlapping no-shoot icon if disguised and in same team
			if (ent->powerups & (1 << PW_OPS_DISGUISED) && cg.predictedPlayerState.persistant[PERS_TEAM] == cgs.clientinfo[ent->clientNum].team)
			{
				return cgs.clientinfo[ent->clientNum].selected ? cgs.media.friendShader : 0;
			}
			return cgs.clientinfo[ent->clientNum].selected ? cgs.media.buddyShader : 0;
		}
		break;
	}
	case ET_ITEM:
	{
		gitem_t *item;

		item = BG_GetItem(ent->modelindex);

		if (drawItemObj && !cg.flagIndicator && item && item->giType == IT_TEAM)
		{
			if ((item->giPowerUp == PW_BLUEFLAG && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
			    || (item->giPowerUp == PW_REDFLAG && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES))
			{
				return cgs.media.objectiveBlueShader;
			}

			return cgs.media.objectiveRedShader;
		}
		break;
	}
	case ET_EXPLOSIVE_INDICATOR:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(
				CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// draw explosives if an engineer
			if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_ENGINEER ||
			    (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS && ent->effect1Time == 1))
			{
				if (ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return 0;
				}

				if (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES)
				{
					return 0;
				}

				return cgs.media.destroyShader;
			}
		}
		break;
	}
	case ET_CONSTRUCTIBLE_INDICATOR:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// draw construction if an engineer
			if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_ENGINEER)
			{
				if (ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_AXIS)
				{
					return 0;
				}

				if (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_ALLIES)
				{
					return 0;
				}

				return cgs.media.constructShader;
			}
		}
		break;
	}
	case ET_TANK_INDICATOR:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// FIXME: show only when relevant
			if ((ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
			    || (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES))
			{
				return cgs.media.escortShader;
			}

			return cgs.media.destroyShader;
		}
		break;
	}
	case ET_TANK_INDICATOR_DEAD:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// FIXME: show only when relevant
			// draw repair if an engineer
			if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_ENGINEER && (
					(ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
					|| (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES)))
			{
				return cgs.media.constructShader;
			}
		}
		break;
	}
	case ET_TRAP:
	{
		if (drawSecondaryObj)
		{
			if (ent->frame == 0)
			{
				return cgs.media.regroupShader;
			}

			if (ent->frame == 4)
			{
				return cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.regroupShader : cgs.media.defendShader;
			}

			if (ent->frame == 3)
			{
				return cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.regroupShader : cgs.media.defendShader;
			}
		}
		break;
	}
	// FIXME: ET_COMMANDMAP_MARKER, ET_HEALER, ET_SUPPLIER
	//case ET_MG42_BARREL:
	//{
	//    return cgs.media.mg42HintShader;
	//}
	//case ET_CABINET_H:
	//{
	//	return cgs.media.healthHintShader;
	//}
	//caseET_CABINET_A:
	//{
	//	return cgs.media.ammoHintShader;
	//}
	default:
		break;
	}

	return 0;
}

/**
 * @brief CG_CompasMoveLocationCalc
 * @param[out] locationvalue
 * @param[in] directionplus
 * @param[in] animationout
 */
static void CG_CompasMoveLocationCalc(float *locationvalue, qboolean directionplus, qboolean animationout)
{
	if (animationout)
	{
		if (directionplus)
		{
			*locationvalue += ((cg.time - cgs.autoMapExpandTime) / 100.f) * 128.f;
		}
		else
		{
			*locationvalue -= ((cg.time - cgs.autoMapExpandTime) / 100.f) * 128.f;
		}
	}
	else
	{
		if (!directionplus)
		{
			*locationvalue += (((cg.time - cgs.autoMapExpandTime - 150.f) / 100.f) * 128.f) - 128.f;
		}
		else
		{
			*locationvalue -= (((cg.time - cgs.autoMapExpandTime - 150.f) / 100.f) * 128.f) - 128.f;
		}
	}
}

/**
 * @brief CG_CompasMoveLocation
 * @param[in] basex
 * @param[in] basey
 * @param[in] animationout
 */
static void CG_CompasMoveLocation(float *basex, float *basey, qboolean animationout)
{
	float x    = *basex;
	float y    = *basey;
	float cent = activehud->compass.location.w / 2;
	x += cent;
	y += cent;

	if (x < Ccg_WideX(320))
	{
		if (y < 240)
		{
			if (x < y)
			{
				//move left
				CG_CompasMoveLocationCalc(basex, qfalse, animationout);
			}
			else
			{
				//move up
				CG_CompasMoveLocationCalc(basey, qfalse, animationout);
			}
		}
		else
		{
			if (x < (SCREEN_HEIGHT - y))
			{
				//move left
				CG_CompasMoveLocationCalc(basex, qfalse, animationout);
			}
			else
			{
				//move down
				CG_CompasMoveLocationCalc(basey, qtrue, animationout);
			}
		}
	}
	else
	{
		if (y < 240)
		{
			if ((Ccg_WideX(SCREEN_WIDTH) - x) < y)
			{
				//move right
				CG_CompasMoveLocationCalc(basex, qtrue, animationout);
			}
			else
			{
				//move up
				CG_CompasMoveLocationCalc(basey, qfalse, animationout);
			}
		}
		else
		{
			if ((Ccg_WideX(SCREEN_WIDTH) - x) < (SCREEN_HEIGHT - y))
			{
				//move right
				CG_CompasMoveLocationCalc(basex, qtrue, animationout);
			}
			else
			{
				//move down
				CG_CompasMoveLocationCalc(basey, qtrue, animationout);
			}
		}
	}
}

/**
 * @brief CG_DrawNewCompass
 * @param location
 */
void CG_DrawNewCompass(rectDef_t location)
{
	float      basex = location.x, basey = location.y - 16, basew = location.w, baseh = location.h;
	snapshot_t *snap;

	if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport)
	{
		snap = cg.nextSnap;
	}
	else
	{
		snap = cg.snap;
	}

	if ((snap->ps.pm_flags & PMF_LIMBO && !cgs.clientinfo[cg.clientNum].shoutcaster)
#ifdef FEATURE_MULTIVIEW
	    || cg.mvTotalClients > 0
#endif
	    )
	{
		CG_DrawExpandedAutoMap();
		return;
	}

	if (!cg_altHud.integer)
	{
		if (cgs.autoMapExpanded)
		{
			if (cg.time - cgs.autoMapExpandTime < 100.f)
			{
				CG_CompasMoveLocation(&basex, &basey, qtrue);
			}
			else
			{
				CG_DrawExpandedAutoMap();
				return;
			}
		}
		else
		{
			if (cg.time - cgs.autoMapExpandTime <= 150.f)
			{
				CG_DrawExpandedAutoMap();
				return;
			}
			else if ((cg.time - cgs.autoMapExpandTime > 150.f) && (cg.time - cgs.autoMapExpandTime < 250.f))
			{
				CG_CompasMoveLocation(&basex, &basey, qfalse);
			}
		}
	}

	if ((snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && !cgs.clientinfo[cg.clientNum].shoutcaster) || !cg_drawCompass.integer)
	{
		return;
	}

	CG_DrawAutoMap(basex, basey, basew, baseh);
}
/**
 * @brief CG_DrawStatsDebug
 */
static void CG_DrawStatsDebug(void)
{
	int textWidth = 0;
	int i, x, y, w, h;

	if (!cg_debugSkills.integer)
	{
		return;
	}

	for (i = 0; i < 6; i++)
	{
		if (statsDebugTime[i] + 9000 > cg.time)
		{
			if (statsDebugTextWidth[i] > textWidth)
			{
				textWidth = statsDebugTextWidth[i];
			}
		}
	}

	w = textWidth + 6;
	h = 9;
	x = SCREEN_WIDTH - w;
	y = (SCREEN_HEIGHT - 5 * (12 + 2) + 6 - 4) - 6 - h;     // don't ask

	i = statsDebugPos;

	do
	{
		vec4_t colour;

		if (statsDebugTime[i] + 9000 <= cg.time)
		{
			break;
		}

		colour[0] = colour[1] = colour[2] = .5f;
		if (cg.time - statsDebugTime[i] > 5000)
		{
			colour[3] = .5f - .5f * ((cg.time - statsDebugTime[i] - 5000) / 4000.f);
		}
		else
		{
			colour[3] = .5f ;
		}
		CG_FillRect(x, y, w, h, colour);

		colour[0] = colour[1] = colour[2] = 1.f;
		if (cg.time - statsDebugTime[i] > 5000)
		{
			colour[3] = 1.f - ((cg.time - statsDebugTime[i] - 5000) / 4000.f);
		}
		else
		{
			colour[3] = 1.f ;
		}
		CG_Text_Paint_Ext(640.f - 3 - statsDebugTextWidth[i], y + h - 2, .15f, .15f, colour, statsDebugStrings[i], 0, 0, ITEM_TEXTSTYLE_NORMAL, &cgs.media.limboFont2);

		y -= h;

		i--;
		if (i < 0)
		{
			i = 6 - 1;
		}
	}
	while (i != statsDebugPos);
}

/*
===========================================================================================
  UPPER RIGHT CORNER
===========================================================================================
*/

#define UPPERRIGHT_X 634
#define UPPERRIGHT_W 52

/**
 * @brief CG_DrawSnapshot
 * @param y
 * @return
 */
static float CG_DrawSnapshot(float y)
{
	char *s = va("t:%i", cg.snap->serverTime);
	int  w  = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);
	int  w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;
	int  x  = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;

	CG_FillRect(x, y, w2 + 5, 36 + 2, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, 36 + 2, 1, HUD_Border);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, HUD_Text, s, 0, 0, 0, &cgs.media.limboFont1);
	s = va("sn:%i", cg.latestSnapshotNum);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 23, 0.19f, 0.19f, HUD_Text, s, 0, 0, 0, &cgs.media.limboFont1);
	s = va("cmd:%i", cgs.serverCommandSequence);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 35, 0.19f, 0.19f, HUD_Text, s, 0, 0, 0, &cgs.media.limboFont1);
	return y + 36 + 4;
}

/**
 * @brief CG_DrawSpeed
 * @param y
 * @return
 */
static float CG_DrawSpeed(float y)
{
	static vec_t  highestSpeed, speed;
	static int    lasttime;
	static vec4_t tclr = { 0.625f, 0.625f, 0.6f, 1.0f };
	char          *s, *s2 = NULL;
	int           w, w2, w3, w4;
	int           thistime;
	int           x, h;

	if (resetmaxspeed)
	{
		highestSpeed  = 0;
		resetmaxspeed = qfalse;
	}

	thistime = trap_Milliseconds();

	if (thistime > lasttime + 100)
	{
		speed = VectorLength(cg.predictedPlayerState.velocity);

		if (speed > highestSpeed)
		{
			highestSpeed = speed;
		}

		lasttime = thistime;
	}

	switch (cg_drawUnit.integer)
	{
	case 0:
		// Units per second
		s  = va("%.1f UPS", speed);
		s2 = va("%.1f MAX", highestSpeed);
		break;
	case 1:
		// Kilometers per hour
		s  = va("%.1f KPH", (speed / SPEED_US_TO_KPH));
		s2 = va("%.1f MAX", (highestSpeed / SPEED_US_TO_KPH));
		break;
	case 2:
		// Miles per hour
		s  = va("%.1f MPH", (speed / SPEED_US_TO_MPH));
		s2 = va("%.1f MAX", (highestSpeed / SPEED_US_TO_MPH));
		break;
	default:
		s  = "";
		s2 = "";
		break;
	}

	h = (cg_drawspeed.integer == 2) ? 24 : 12;

	w  = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);
	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;

	x = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;
	CG_FillRect(x, y, w2 + 5, h + 2, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, h + 2, 1, HUD_Border);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);

	// draw max speed on second line
	if (cg_drawspeed.integer == 2)
	{
		y  = y + 12;
		w3 = CG_Text_Width_Ext(s2, 0.19f, 0, &cgs.media.limboFont1);
		w4 = (UPPERRIGHT_W > w3) ? UPPERRIGHT_W : w3;
		CG_Text_Paint_Ext(x + ((w4 - w3) / 2) + 2, y + 11, 0.19f, 0.19f, tclr, s2, 0, 0, 0, &cgs.media.limboFont1);
	}

	return y + 12 + 4;
}

#define MAX_FPS_FRAMES  500

/**
 * @brief CG_DrawFPS
 * @param[in] y
 * @return
 */
static float CG_DrawFPS(float y)
{
	static int previousTimes[MAX_FPS_FRAMES];
	static int previous;
	static int index;
	static int oldSamples;
	const char *s;
	int        t;
	int        frameTime;
	int        samples = cg_drawFPS.integer;
	int        x, w, w2;

	t = trap_Milliseconds(); // don't use serverTime, because that will be drifting to correct for internet lag changes, timescales, timedemos, etc

	frameTime = t - previous;
	previous  = t;

	if (samples < 4)
	{
		samples = 4;
	}
	if (samples > MAX_FPS_FRAMES)
	{
		samples = MAX_FPS_FRAMES;
	}
	if (samples != oldSamples)
	{
		index = 0;
	}

	oldSamples                     = samples;
	previousTimes[index % samples] = frameTime;
	index++;

	if (index > samples)
	{
		int i, fps;
		// average multiple frames together to smooth changes out a bit
		int total = 0;

		for (i = 0 ; i < samples ; ++i)
		{
			total += previousTimes[i];
		}

		total = total ? total : 1;

		fps = 1000 * samples / total;

		s = va("%i FPS", fps);
	}
	else
	{
		s = "estimating";
	}

	w  = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);
	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;

	x = (int)(Ccg_WideX(UPPERRIGHT_X)) - w2 - 2;
	CG_FillRect(x, y, w2 + 5, 12 + 2, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, 12 + 2, 1, HUD_Border);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, HUD_Text, s, 0, 0, 0, &cgs.media.limboFont1);

	return y + 12 + 4;
}

/**
 * @brief CG_SpawnTimerText red colored spawn time text in reinforcement time HUD element.
 * @return red colored text or NULL when its not supposed to be rendered
*/
char *CG_SpawnTimerText()
{
	int msec = (cgs.timelimit * 60000.f) - (cg.time - cgs.levelStartTime);
	int seconds;
	int secondsThen;

	if (cg_spawnTimer_set.integer != -1 && cgs.gamestate == GS_PLAYING && !cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			int period = cg_spawnTimer_period.integer > 0 ? cg_spawnTimer_period.integer : (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS ? cg_bluelimbotime.integer / 1000 : cg_redlimbotime.integer / 1000);
			if (period > 0) // prevent division by 0 for weird cases like limbtotime < 1000
			{
				seconds     = msec / 1000;
				secondsThen = ((cgs.timelimit * 60000.f) - cg_spawnTimer_set.integer) / 1000;
				return va("%i", period + (seconds - secondsThen) % period);
			}
		}
	}
	else if (cg_spawnTimer_set.integer != -1 && cg_spawnTimer_period.integer > 0 && cgs.gamestate != GS_PLAYING)
	{
		// We are not playing and the timer is set so reset/disable it
		// this happens for example when custom period is set by timerSet and map is restarted or changed
		trap_Cvar_Set("cg_spawnTimer_set", "-1");
	}
	return NULL;
}

/**
 * @brief CG_SpawnTimersText
 * @param[out] respawn
 * @param[out] spawntimer
 * @return
 */
static qboolean CG_SpawnTimersText(char **s, char **rt)
{
	if (cgs.gamestate != GS_PLAYING)
	{
		int limbotimeOwn, limbotimeEnemy;
		if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS)
		{
			limbotimeOwn   = cg_redlimbotime.integer;
			limbotimeEnemy = cg_bluelimbotime.integer;
		}
		else
		{
			limbotimeOwn   = cg_bluelimbotime.integer;
			limbotimeEnemy = cg_redlimbotime.integer;
		}

		*rt = va("%2.0i", limbotimeEnemy / 1000);
		*s  = cgs.gametype == GT_WOLF_LMS ? va("%s", CG_TranslateString("WARMUP")) : va("%2.0i", limbotimeOwn / 1000);
		return qtrue;
	}
	else if (cgs.gametype != GT_WOLF_LMS && (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW)) && cg_drawReinforcementTime.integer > 0)
	{
		*s  = va("%2.0i", CG_CalculateReinfTime(qfalse));
		*rt = CG_SpawnTimerText();
	}
	return qfalse;
}


static char *CG_RoundTimerText()
{
	qtime_t qt;
	int     msec = CG_RoundTime(&qt);
	if (msec < 0 && cgs.timelimit > 0.0f)
	{
		return "0:00"; // round ended
	}

	char *seconds = qt.tm_sec > 9 ? va("%i", qt.tm_sec) : va("0%i", qt.tm_sec);
	char *minutes = qt.tm_min > 9 ? va("%i", qt.tm_min) : va("0%i", qt.tm_min);

	return va("%s:%s", minutes, seconds);
}

static char *CG_LocalTimeText()
{
	qtime_t  time;
	char     *s;
	qboolean pmtime = qfalse;

	//Fetch the local time
	trap_RealTime(&time);

	if (cg_drawTime.integer & LOCALTIME_SECOND)
	{
		if (cg_drawTime.integer & LOCALTIME_12HOUR)
		{
			if (time.tm_hour > 12)
			{
				pmtime = qtrue;
			}
			s = va("%i:%02i:%02i %s", (pmtime ? time.tm_hour - 12 : time.tm_hour), time.tm_min, time.tm_sec, (pmtime ? "PM" : "AM"));
		}
		else
		{
			s = va("%02i:%02i:%02i", time.tm_hour, time.tm_min, time.tm_sec);
		}
	}
	else
	{
		if (cg_drawTime.integer & LOCALTIME_12HOUR)
		{
			if (time.tm_hour > 12)
			{
				pmtime = qtrue;
			}
			s = va("%i:%02i %s", (pmtime ? time.tm_hour - 12 : time.tm_hour), time.tm_min, (pmtime ? "PM" : "AM"));
		}
		else
		{
			s = va("%02i:%02i", time.tm_hour, time.tm_min);
		}
	}
	return s;
}

/**
 * @brief CG_DrawTimersAlt
 * @param[in] respawn
 * @param[in] spawntimer
 * @param[in] localtime
 * @param[in] roundtimer
 */
static void CG_DrawTimersAlt(struct hudComponent_s *respawn, struct hudComponent_s *spawntimer, struct hudComponent_s *localtime, struct hudComponent_s *roundtimer)
{
	char     *s = NULL, *rt = NULL;
	int      w;
	vec4_t   color;
	qboolean blink = CG_SpawnTimersText(&s, &rt);
	float    blinkAlpha;

	if (blink)
	{
		blinkAlpha = fabs(sin(cg.time * 0.002));
	}

	Com_Memcpy(color, roundtimer->color, sizeof(vec4_t));
	color[3] = blink ? blinkAlpha : color[3];
	CG_Text_Paint_Ext(roundtimer->location.x, roundtimer->location.y, roundtimer->scale, roundtimer->scale, color, CG_RoundTimerText(), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

	if (s)
	{
		w = CG_Text_Width_Ext(s, respawn->scale, 0, &cgs.media.limboFont1);
		Com_Memcpy(color, respawn->color, sizeof(vec4_t));
		color[3] = blink ? blinkAlpha : color[3];
		CG_Text_Paint_Ext(respawn->location.x - w, respawn->location.y, respawn->scale, respawn->scale, color, s, 0, 0,
		                  ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}

	if (rt)
	{
		w = CG_Text_Width_Ext(s, spawntimer->scale, 0, &cgs.media.limboFont1);
		Com_Memcpy(color, spawntimer->color, sizeof(vec4_t));
		color[3] = blink ? blinkAlpha : color[3];
		CG_Text_Paint_Ext(spawntimer->location.x - w, spawntimer->location.y, spawntimer->scale, spawntimer->scale,
		                  color, rt, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}

	if (cg_drawTime.integer & LOCALTIME_ON)
	{
		CG_Text_Paint_Ext(localtime->location.x, localtime->location.y, localtime->scale, localtime->scale, localtime->color, CG_LocalTimeText(), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
}

/**
 * @brief CG_DrawTimerNormal
 * @param[in] y
 * @return
 */
static float CG_DrawTimerNormal(float y)
{
	char     *s = NULL, *rt = NULL, *mt = va("%s%s", "^7", CG_RoundTimerText());
	int      w, w2;
	int      x;
	float    blinkAlpha;
	qboolean blink = CG_SpawnTimersText(&s, &rt);

	if (blink)
	{
		blinkAlpha = fabs(sin(cg.time * 0.002));
	}

	if (s)
	{
		s = va("^$%s%s%s", s, " ", mt);
	}
	else
	{
		s = mt;
	}

	if (rt)
	{
		s = va("^1%s%s%s", rt, " ", s);
	}

	w  = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);
	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;

	x = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;
	CG_FillRect(x, y, w2 + 5, 12 + 2, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, 12 + 2, 1, HUD_Border);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, HUD_Text, s, 0, 0, 0, &cgs.media.limboFont1);

	return y + 12 + 4;
}

/**
 * @brief CG_DrawLocalTime
 * @param[in] y
 * @return
 */
static float CG_DrawLocalTime(float y)
{
	int  w, w2, x;
	char *s = CG_LocalTimeText();

	w  = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);
	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;

	x = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;
	CG_FillRect(x, y, w2 + 5, 12 + 2, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, 12 + 2, 1, HUD_Border);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, activehud->localtime.color, s, 0, 0, 0, &cgs.media.limboFont1);

	return y + 12 + 4;
}

/**
 * @brief Adds the current interpolate / extrapolate bar for this frame
 */
void CG_AddLagometerFrameInfo(void)
{
	lagometer.frameSamples[lagometer.frameCount & (LAG_SAMPLES - 1)] = cg.time - cg.latestSnapshotTime;
	lagometer.frameCount++;
}

/**
 * @brief Log the ping time, server framerate and number of dropped snapshots
 * before it each time a snapshot is received.
 * @param[in] snap
 */
void CG_AddLagometerSnapshotInfo(snapshot_t *snap)
{
	unsigned int index = lagometer.snapshotCount & (LAG_SAMPLES - 1);
	int          oldest;

	// dropped packet
	if (!snap)
	{
		lagometer.snapshotSamples[index] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	if (cg.demoPlayback)
	{
		static int lasttime = 0;

		snap->ping = (snap->serverTime - snap->ps.commandTime) - (1000 / cgs.sv_fps);

		// display snapshot time delta instead of ping
		lagometer.snapshotSamples[index] = snap->serverTime - lasttime;
		lasttime                         = snap->serverTime;
	}
	else
	{
		lagometer.snapshotSamples[index] = MAX(snap->ping - snap->ps.stats[STAT_ANTIWARP_DELAY], 0);
	}
	lagometer.snapshotAntiwarp[index] = snap->ping;  // TODO: check this for demoPlayback
	lagometer.snapshotFlags[index]    = snap->snapFlags;
	lagometer.snapshotCount++;

	// compute server framerate
	index = cgs.sampledStat.count;

	if (cgs.sampledStat.count < LAG_SAMPLES)
	{
		cgs.sampledStat.count++;
	}
	else
	{
		index -= 1;
	}

	cgs.sampledStat.samples[index].elapsed = snap->serverTime - cgs.sampledStat.lastSampleTime;
	cgs.sampledStat.samples[index].time    = snap->serverTime;

	if (cgs.sampledStat.samples[index].elapsed < 0)
	{
		cgs.sampledStat.samples[index].elapsed = 0;
	}

	cgs.sampledStat.lastSampleTime = snap->serverTime;

	cgs.sampledStat.samplesTotalElpased += cgs.sampledStat.samples[index].elapsed;

	oldest = snap->serverTime - PERIOD_SAMPLES;
	for (index = 0; index < cgs.sampledStat.count; index++)
	{
		if (cgs.sampledStat.samples[index].time > oldest)
		{
			break;
		}

		cgs.sampledStat.samplesTotalElpased -= cgs.sampledStat.samples[index].elapsed;
	}

	if (index)
	{
		memmove(cgs.sampledStat.samples, cgs.sampledStat.samples + index, sizeof(sample_t) * (cgs.sampledStat.count - index));
		cgs.sampledStat.count -= index;
	}

	cgs.sampledStat.avg = cgs.sampledStat.samplesTotalElpased > 0
	                      ? (int) (cgs.sampledStat.count / (cgs.sampledStat.samplesTotalElpased / 1000.0f) + 0.5f)
	                      : 0;
}

/**
 * @brief Draw disconnect icon for long lag
 * @param[in] y
 * @return
 */
static float CG_DrawDisconnect(float y)
{
	int        cmdNum, w, w2, x;
	usercmd_t  cmd;
	const char *s;

	// use same dimension as timer
	w  = CG_Text_Width_Ext("xx:xx:xx", 0.19f, 0, &cgs.media.limboFont1);
	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;
	x  = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;

	// dont draw if a demo and we're running at a different timescale
	if (cg.demoPlayback && cg_timescale.value != 1.0f)
	{
		return y + w2 + 13;
	}

	// don't draw if the server is respawning
	if (cg.serverRespawning)
	{
		return y + w2 + 13;
	}

	// don't draw if intermission is about to start
	if (cg.intermissionStarted)
	{
		return y + w2 + 13;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if (cmd.serverTime <= cg.snap->ps.commandTime
	    || cmd.serverTime > cg.time)        // special check for map_restart
	{
		return y + w2 + 13;
	}

	// also add text in center of screen
	s = CG_TranslateString("Connection Interrupted");
	w = CG_Text_Width_Ext(s, cg_fontScaleTP.value, 0, &cgs.media.limboFont2);
	CG_Text_Paint_Ext(Ccg_WideX(320) - w / 2, 100, cg_fontScaleTP.value, cg_fontScaleTP.value, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

	// blink the icon
	if ((cg.time >> 9) & 1)
	{
		return y + w2 + 13;
	}

	CG_DrawPic(x + 1, y + 1, w2 + 3, w2 + 3, cgs.media.disconnectIcon);
	return y + w2 + 13;
}

/**
 * @brief CG_DrawPing
 * @param[in] y
 * @return
 */
static float CG_DrawPing(float y)
{
	int  curPing = cg.snap->ping;
	int  w, w2, x;
	char *s;

	s = va("Ping %d", curPing < 999 ? curPing : 999);
	w = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);

	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;

	x = (int)(Ccg_WideX(UPPERRIGHT_X)) - w2 - 2;
	CG_FillRect(x, y, w2 + 5, 12 + 2, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, 12 + 2, 1, HUD_Border);
	CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, HUD_Text, s, 0, 0, 0, &cgs.media.limboFont1);

	return y + 12 + 4;
}

vec4_t colorAW = { 0, 0.5, 0, 0.5f };

/**
 * @brief CG_DrawLagometer
 * @param[in] y
 * @return
 */
static float CG_DrawLagometer(float y)
{
	int   a, w, w2, x, i;
	float v;
	float ax, ay, aw, ah, mid, range;
	int   color;
	float vscale;

	// use same dimension as timer
	w  = CG_Text_Width_Ext("xx:xx:xx", 0.19f, 0, &cgs.media.limboFont1);
	w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;
	x  = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;

	// draw the graph
	trap_R_SetColor(NULL);
	CG_FillRect(x, y, w2 + 5, w2 + 5, HUD_Background);
	CG_DrawRect_FixedBorder(x, y, w2 + 5, w2 + 5, 1, HUD_Border);

	ax = x;
	ay = y;
	aw = w2 + 4;
	ah = w2 + 4;
	CG_AdjustFrom640(&ax, &ay, &aw, &ah);

	color = -1;
	range = ah / 3;
	mid   = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for (a = 0 ; a < aw ; a++)
	{
		i  = (lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
		v  = lagometer.frameSamples[i];
		v *= vscale;
		if (v > 0)
		{
			if (color != 1)
			{
				color = 1;
				trap_R_SetColor(colorYellow);
			}
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0)
		{
			if (color != 2)
			{
				color = 2;
				trap_R_SetColor(colorBlue);
			}
			v = -v;
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	// draw the snapshot latency / drop graph
	range  = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for (a = 0 ; a < aw ; a++)
	{
		i = (lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if (v > 0)
		{
			// antiwarp indicator
			if (lagometer.snapshotAntiwarp[i] > 0)
			{
				w = lagometer.snapshotAntiwarp[i] * vscale;

				if (color != 6)
				{
					color = 6;
					trap_R_SetColor(colorAW);
				}

				if (w > range)
				{
					w = range;
				}
				trap_R_DrawStretchPic(ax + aw - a, ay + ah - w - 2, 1, w, 0, 0, 0, 0, cgs.media.whiteShader);
			}

			if (lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED)
			{
				if (color != 5)
				{
					color = 5;  // YELLOW for rate delay
					trap_R_SetColor(colorYellow);
				}
			}
			else
			{
				if (color != 3)
				{
					color = 3;
					trap_R_SetColor(colorGreen);
				}
			}
			v = v * vscale;
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0)
		{
			if (color != 4)
			{
				color = 4;      // RED for dropped snapshots
				trap_R_SetColor(colorRed);
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	trap_R_SetColor(NULL);

	if (cg_nopredict.integer
#ifdef ALLOW_GSYNC
	    || cg_synchronousClients.integer
#endif // ALLOW_GSYNC
	    )
	{
		CG_Text_Paint_Ext(ax, ay, cg_fontScaleTP.value, cg_fontScaleTP.value, colorWhite, "snc", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	}

	// don't draw if a demo and we're running at a different timescale
	if (!cg.demoPlayback)
	{
		CG_DrawDisconnect(y);
	}

	// add snapshots/s in top-right corner of meter
	{
		char   *result;
		vec4_t *clr;

		if (cgs.sampledStat.avg < cgs.sv_fps * 0.5f)
		{
			clr = &colorRed;
		}
		else if (cgs.sampledStat.avg < cgs.sv_fps * 0.75f)
		{
			clr = &colorYellow;
		}
		else
		{
			clr = &HUD_Text;
		}

		// FIXME: see warmup blinky blinky
		//if (cgs.gamestate != GS_PLAYING)
		//{
		//	color[3] = fabs(sin(cg.time * 0.002));
		//}

		// FIXME: we might do different views x/Y or in %
		//result = va("%i/%i", cgs.sampledStat.avg, cgs.sv_fps);
		result = va("%i", cgs.sampledStat.avg);

		w  = CG_Text_Width_Ext(result, 0.19f, 0, &cgs.media.limboFont1);
		w2 = (UPPERRIGHT_W > w) ? UPPERRIGHT_W : w;
		x  = Ccg_WideX(UPPERRIGHT_X) - w2 - 2;

		CG_Text_Paint_Ext(x + ((w2 - w) / 2) + 2, y + 11, 0.19f, 0.19f, *clr, result, 0, 0, 0, &cgs.media.limboFont1);
	}

	return y + w + 13;
}

/**
 * @brief Check if we are underwater
 * @details This check has changed to make it work for spectators following another player.
 * That's why ps.stats[STAT_AIRLEFT] has been added..
 *
 * While following high-pingers, You sometimes see the breathbar, even while they are not submerged..
 * So we check for underwater status differently when we are following others.
 * (It doesn't matter to do a more complex check for spectators.. they are not playing)
 * @return
 */
static qboolean CG_CheckPlayerUnderwater()
{
	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		vec3_t origin;

		VectorCopy(cg.snap->ps.origin, origin);
		origin[2] += 36;
		return (qboolean)(CG_PointContents(origin, cg.snap->ps.clientNum) & CONTENTS_WATER);
	}

	return cg.snap->ps.stats[STAT_AIRLEFT] < HOLDBREATHTIME;
}

/**
 * @brief Draw the player status
 */
static void CG_DrawPlayerStatus(void)
{
	if (activehud->weaponicon.visible)
	{
		CG_DrawGunIcon(activehud->weaponicon.location);
	}

	if (activehud->weaponammo.visible)
	{
		CG_DrawAmmoCount(activehud->weaponammo.location.x, activehud->weaponammo.location.y);
	}

	if (activehud->healthbar.visible)
	{
		CG_DrawPlayerHealthBar(&activehud->healthbar.location);
	}

	if (activehud->staminabar.visible)
	{
		if (CG_CheckPlayerUnderwater())
		{
			CG_DrawBreathBar(&activehud->breathbar.location);
		}
		else
		{
			CG_DrawStaminaBar(&activehud->staminabar.location);
		}
	}

	if (activehud->weaponchargebar.visible)
	{
		CG_DrawWeapRecharge(&activehud->weaponchargebar.location);
	}
}

/**
 * @brief Draw the player stats
 */
static void CG_DrawPlayerStats(void)
{
	if (activehud->healthtext.visible)
	{
		CG_DrawPlayerHealth(activehud->healthtext.location.x, activehud->healthtext.location.y);
	}

	if (CG_CheckPlayerUnderwater())
	{
		if (activehud->breathtext.visible)
		{
			CG_DrawPlayerBreath(activehud->breathtext.location.x, activehud->breathtext.location.y);
		}
	}
	else
	{
		if (activehud->sprinttext.visible)
		{
			CG_DrawPlayerSprint(activehud->sprinttext.location.x, activehud->sprinttext.location.y);
		}
	}

	if (activehud->weaponchargetext.visible)
	{
		CG_DrawWeaponCharge(activehud->weaponchargetext.location.x, activehud->weaponchargetext.location.y);
	}

	if (cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	if (activehud->statsdisplay.visible)
	{
		CG_DrawSkills(activehud->statsdisplay);
	}

	if (activehud->xptext.visible)
	{
		CG_DrawXP(activehud->xptext.location.x, activehud->xptext.location.y);
	}

	if (activehud->ranktext.visible && !(cg_altHudFlags.integer & FLAGS_REMOVE_RANKS))
	{
		CG_DrawRank(activehud->ranktext.location.x, activehud->ranktext.location.y);
	}
}

/**
 * @brief CG_Hud_Setup
 */
void CG_Hud_Setup(void)
{
	hudStucture_t hud1;
	hudStucture_t hud2;

	// Hud0 aka the Default hud
	CG_setDefaultHudValues(&hud0);
	activehud = &hud0;
	CG_addHudToList(&hud0);

	// Hud1
	hud1.hudnumber        = 1;
	hud1.compass          = CG_getComponent(44, SCREEN_HEIGHT - 87, 84, 84, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.staminabar       = CG_getComponent(4, 388, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.breathbar        = CG_getComponent(4, 388, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.healthbar        = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 36, 388, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.weaponchargebar  = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 16, 388, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.healthtext       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 60, SCREEN_HEIGHT - 65, 0, 0, qtrue, STYLE_NORMAL, 0.25f, colorWhite);
	hud1.xptext           = CG_getComponent(48, SCREEN_HEIGHT - 4, 0, 0, qtrue, STYLE_NORMAL, 0.25f, colorWhite);
	hud1.ranktext         = CG_getComponent(62, SCREEN_HEIGHT - 16, 0, 0, qtrue, STYLE_NORMAL, 0.2f, colorWhite);
	hud1.statsdisplay     = CG_getComponent(24, SCREEN_HEIGHT - 95, 0, 0, qtrue, STYLE_SIMPLE, 0.25f, colorWhite);
	hud1.weaponicon       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 82 - 20, SCREEN_HEIGHT - 56, 60, 32, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.weaponammo       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 22 - 20, SCREEN_HEIGHT - 1 * (16 + 2) + 12 - 4, 0, 0, qtrue, STYLE_NORMAL, 0.25f, colorWhite);
	hud1.fireteam         = CG_getComponent(Ccg_WideX(SCREEN_WIDTH), 10, 100, 100, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.popupmessages    = CG_getComponent(4, 100, 72, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.powerups         = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 40, SCREEN_HEIGHT - 136, 36, 36, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.objectives       = CG_getComponent(8, SCREEN_HEIGHT - 136, 36, 36, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.hudhead          = CG_getComponent(44, SCREEN_HEIGHT - 92, 62, 80, qfalse, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.cursorhints      = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) * .5f - 24, 260, 48, 48, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.weaponstability  = CG_getComponent(50, 208, 10, 64, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.livesleft        = CG_getComponent(0, 0, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.reinforcement    = CG_getComponent(95, SCREEN_HEIGHT - 12, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorLtBlue);
	hud1.roundtimer       = CG_getComponent(100, SCREEN_HEIGHT - 12, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.spawntimer       = CG_getComponent(95, SCREEN_HEIGHT - 2, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorRed);
	hud1.localtime        = CG_getComponent(100, SCREEN_HEIGHT - 2, 0, 0, qtrue, STYLE_NORMAL, 0.19f, HUD_Text);
	hud1.votetext         = CG_getComponent(8, 224, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.spectatortext    = CG_getComponent(8, 188, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.limbotext        = CG_getComponent(8, 164, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.followtext       = CG_getComponent(8, 164, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud1.demotext         = CG_getComponent(10, 9, 0.22f, 0.22f, qtrue, STYLE_SIMPLE, 0.22f, colorRed);
	hud1.missilecamera    = CG_getComponent(4, 120, 160, 120, qtrue, STYLE_NORMAL, 1, colorWhite);
	hud1.sprinttext       = CG_getComponent(20, 384, 0, 0, qfalse, STYLE_NORMAL, 0.25f, colorWhite);
	hud1.breathtext       = CG_getComponent(20, 384, 0, 0, qfalse, STYLE_NORMAL, 0.25f, colorWhite);
	hud1.weaponchargetext = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 16, 384, 0, 0, qfalse, STYLE_NORMAL, 0.25f, colorWhite);
	CG_addHudToList(&hud1);

	// Hud2
	hud2.hudnumber        = 2;
	hud2.compass          = CG_getComponent(64, SCREEN_HEIGHT - 87, 84, 84, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.staminabar       = CG_getComponent(4, 388, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.breathbar        = CG_getComponent(4, 388, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.healthbar        = CG_getComponent(24, 388, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.weaponchargebar  = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 16, 388, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.healthtext       = CG_getComponent(65, SCREEN_HEIGHT - 4, 0, 0, qtrue, STYLE_NORMAL, 0.25f, colorWhite);
	hud2.xptext           = CG_getComponent(132, SCREEN_HEIGHT - 4, 0, 0, qtrue, STYLE_NORMAL, 0.25f, colorWhite);
	hud2.ranktext         = CG_getComponent(146, SCREEN_HEIGHT - 16, 0, 0, qtrue, STYLE_NORMAL, 0.2f, colorWhite);
	hud2.statsdisplay     = CG_getComponent(44, SCREEN_HEIGHT - 95, 0, 0, qtrue, STYLE_SIMPLE, 0.25f, colorWhite);
	hud2.weaponicon       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 82, SCREEN_HEIGHT - 56, 60, 32, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.weaponammo       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 22, SCREEN_HEIGHT - 1 * (16 + 2) + 12 - 4, 0, 0, qtrue, STYLE_NORMAL, 0.25f, colorWhite);
	hud2.fireteam         = CG_getComponent(Ccg_WideX(SCREEN_WIDTH), 10, 100, 100, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.popupmessages    = CG_getComponent(4, 100, 72, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.powerups         = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 40, SCREEN_HEIGHT - 136, 36, 36, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.objectives       = CG_getComponent(8, SCREEN_HEIGHT - 136, 36, 36, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.hudhead          = CG_getComponent(44, SCREEN_HEIGHT - 92, 62, 80, qfalse, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.cursorhints      = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) * .5f - 24, 260, 48, 48, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.weaponstability  = CG_getComponent(50, 208, 10, 64, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.livesleft        = CG_getComponent(0, 0, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.reinforcement    = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 60, SCREEN_HEIGHT - 70, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorLtBlue);
	hud2.roundtimer       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 55, SCREEN_HEIGHT - 70, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.spawntimer       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 60, SCREEN_HEIGHT - 60, 0, 0, qtrue, STYLE_NORMAL, 0.19f, colorRed);
	hud2.localtime        = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 55, SCREEN_HEIGHT - 60, 0, 0, qtrue, STYLE_NORMAL, 0.19f, HUD_Text);
	hud2.votetext         = CG_getComponent(8, 224, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.spectatortext    = CG_getComponent(8, 188, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.limbotext        = CG_getComponent(8, 164, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.followtext       = CG_getComponent(8, 164, 0.22f, 0.22f, qtrue, STYLE_NORMAL, 0.19f, colorWhite);
	hud2.demotext         = CG_getComponent(10, 9, 0.22f, 0.22f, qtrue, STYLE_SIMPLE, 0.22f, colorRed);
	hud2.missilecamera    = CG_getComponent(4, 120, 160, 120, qtrue, STYLE_NORMAL, 1, colorWhite);
	hud2.sprinttext       = CG_getComponent(20, 384, 0, 0, qfalse, STYLE_NORMAL, 0.25f, colorWhite);
	hud2.breathtext       = CG_getComponent(20, 384, 0, 0, qfalse, STYLE_NORMAL, 0.25f, colorWhite);
	hud2.weaponchargetext = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 16, 384, 0, 0, qfalse, STYLE_NORMAL, 0.25f, colorWhite);
	CG_addHudToList(&hud2);

	// Read the hud files
	CG_ReadHudScripts();
}

#ifdef ETLEGACY_DEBUG

/**
 * @brief CG_PrintHudComponent
 * @param[in] name
 * @param[in] comp
 */
static void CG_PrintHudComponent(const char *name, hudComponent_t *comp)
{
	Com_Printf("%s location: X %.f Y %.f W %.f H %.f visible: %i\n", name, comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->visible);
}

/**
 * @brief CG_PrintHud
 * @param[in] hud
 */
static void CG_PrintHud(hudStucture_t *hud)
{
	int i;

	for (i = 0; hudComponentFields[i].name; i++)
	{
		if (!hudComponentFields[i].isAlias)
		{
			CG_PrintHudComponent(hudComponentFields[i].name, (hudComponent_t *)((char * )hud + hudComponentFields[i].offset));
		}
	}
}
#endif

/**
 * @brief CG_SetHud
 */
void CG_SetHud(void)
{
	if (cg_altHud.integer && activehud->hudnumber != cg_altHud.integer)
	{
		activehud = CG_getHudByNumber(cg_altHud.integer);
		if (!activehud)
		{
			Com_Printf("^1ERROR hud with number %i is not available, defaulting to 0\n", cg_altHud.integer);
			activehud = &hud0;
			trap_Cvar_Set("cg_altHud", "0");
			return;
		}

#ifdef ETLEGACY_DEBUG
		CG_PrintHud(activehud);
#endif

		Com_Printf("Setting hud to: %i\n", cg_altHud.integer);
	}
	else if (!cg_altHud.integer && activehud->hudnumber != hud0.hudnumber)
	{
		activehud = &hud0;
	}
}

/**
 * @brief CG_DrawActiveHud
 */
void CG_DrawActiveHud(void)
{
	if (cg.snap->ps.stats[STAT_HEALTH] > 0)
	{
		if (activehud->hudhead.visible)
		{
			CG_DrawPlayerStatusHead(activehud->hudhead);
		}
		CG_DrawPlayerStatus();
		CG_DrawPlayerStats();
	}

	CG_DrawLivesLeft(activehud->livesleft);

	// Cursor hint
	if (activehud->cursorhints.visible)
	{
		CG_DrawCursorhint(&activehud->cursorhints.location);
	}

	// Stability bar
	if (activehud->weaponstability.visible)
	{
		CG_DrawWeapStability(&activehud->weaponstability.location);
	}

	// Stats Debugging
	CG_DrawStatsDebug();
}

/**
 * @brief CG_DrawGlobalHud
 */
void CG_DrawGlobalHud(void)
{
	int style = cg_popupShadow.integer ? ITEM_TEXTSTYLE_SHADOWED : ITEM_TEXTSTYLE_NORMAL;

	if (cg_altHudFlags.integer & FLAGS_MOVE_POPUPS)
	{
		CG_DrawPMItems(activehud->popupmessages.location, style);
	}
	else
	{
		CG_DrawPMItems(hud0.popupmessages.location, style);
	}

	CG_DrawPMItemsBig(style);

#ifdef FEATURE_EDV
	if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
	{
		return;
	}
#endif

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		CG_DrawMinimap();
		CG_DrawShoutcastPowerups();
		return;
	}
	else
	{
		CG_DrawNewCompass(activehud->compass.location);
	}

	if (activehud->powerups.visible)
	{
		CG_DrawPowerUps(activehud->powerups.location);
	}

	if (activehud->objectives.visible)
	{
		CG_DrawObjectiveStatus(activehud->objectives.location);
	}
}

/**
 * @brief CG_DrawUpperRight
 */
void CG_DrawUpperRight(void)
{
	float y = 152; // 20 + 100 + 32;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		CG_DrawShoutcastTimer();
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		if (cg_drawFireteamOverlay.integer && CG_IsOnFireteam(cg.clientNum))
		{
			if (cg_altHudFlags.integer & FLAGS_MOVE_POPUPS)
			{
				CG_DrawFireTeamOverlay(&activehud->fireteam.location);
			}
			else
			{
				CG_DrawFireTeamOverlay(&hud0.fireteam.location);
			}
		}

		if (!(cg.snap->ps.pm_flags & PMF_LIMBO) && (cgs.autoMapExpanded || (cg.time - cgs.autoMapExpandTime < 250.f)) && !cg_altHud.integer)
		{
			return;
		}
	}

	if (cg_drawRoundTimer.integer)
	{
		if (cg_altHudFlags.integer & FLAGS_MOVE_TIMERS)
		{
			CG_DrawTimersAlt(&activehud->reinforcement, &activehud->spawntimer, &activehud->localtime, &activehud->roundtimer);
		}
		else
		{
			y = CG_DrawTimerNormal(y);
		}
	}

	if ((cg_drawTime.integer & LOCALTIME_ON) && !(cg_altHudFlags.integer & FLAGS_MOVE_TIMERS))
	{
		y = CG_DrawLocalTime(y);
	}

	if (cg_drawFPS.integer)
	{
		y = CG_DrawFPS(y);
	}

	if (cg_drawSnapshot.integer)
	{
		y = CG_DrawSnapshot(y);
	}

	if (cg_drawPing.integer)
	{
		y = CG_DrawPing(y);
	}

	if (cg_drawspeed.integer)
	{
		y = CG_DrawSpeed(y);
	}

	if (cg_lagometer.integer)
	{
		CG_DrawLagometer(y);
	}
	else
	{
		CG_DrawDisconnect(y);
	}
}
