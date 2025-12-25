/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2003 Iron Claw Interactive
Copyright (C) 2005-2013 Smokin' Guns

This file is part of Smokin' Guns.

Smokin' Guns is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Smokin' Guns is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Smokin' Guns; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#include "../ui/ui_shared.h"

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
// Smokin' Guns specific
menuDef_t *menuBuy = NULL;
menuDef_t *menuItem = NULL;
int menuItemCount = 0;
coord_t save_menuBuy;
coord_t save_menuItem;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];


int CG_Text_Width(const char *text, float scale, int limit) {
    int count, len;
    float out;
    glyphInfo_t *glyph;
    float useScale;
    // FIXME: see ui_main.c, same problem
    //	const unsigned char *s = text;
    const char *s = text;
    fontInfo_t *font = &cgDC.Assets.textFont;
    if (scale <= cg_smallFont.value) {
        font = &cgDC.Assets.smallFont;
    } else if (scale > cg_bigFont.value) {
        font = &cgDC.Assets.bigFont;
    }
    useScale = scale * font->glyphScale;
    out = 0;
    if (text) {
        len = strlen(text);
        if (limit > 0 && len > limit) {
            len = limit;
        }
        count = 0;
        while (s && *s && count < len) {
            if (Q_IsColorString(s)) {
                s += 2;
                continue;
            } else {
                glyph = &font->glyphs[(
                    int)*s]; // TTimo: FIXME: getting nasty warnings without the
                             // cast, hopefully this doesn't break the VM build
                out += glyph->xSkip;
                s++;
                count++;
            }
        }
    }
    return out * useScale;
}

int CG_Text_Height(const char *text, float scale, int limit) {
    int len, count;
    float max;
    glyphInfo_t *glyph;
    float useScale;
    // TTimo: FIXME
    //	const unsigned char *s = text;
    const char *s = text;
    fontInfo_t *font = &cgDC.Assets.textFont;
    if (scale <= cg_smallFont.value) {
        font = &cgDC.Assets.smallFont;
    } else if (scale > cg_bigFont.value) {
        font = &cgDC.Assets.bigFont;
    }
    useScale = scale * font->glyphScale;
    max = 0;
    if (text) {
        len = strlen(text);
        if (limit > 0 && len > limit) {
            len = limit;
        }
        count = 0;
        while (s && *s && count < len) {
            if (Q_IsColorString(s)) {
                s += 2;
                continue;
            } else {
                glyph = &font->glyphs[(
                    int)*s]; // TTimo: FIXME: getting nasty warnings without the
                             // cast, hopefully this doesn't break the VM build
                if (max < glyph->height) {
                    max = glyph->height;
                }
                s++;
                count++;
            }
        }
    }
    return max * useScale;
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale,
                       float s, float t, float s2, float t2,
                       qhandle_t hShader) {
    float w, h;
    w = width * scale;
    h = height * scale;
    CG_AdjustFrom640(&x, &y, &w, &h);
    trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color,
                   const char *text, float adjust, int limit, int style) {
    int len, count;
    vec4_t newColor;
    glyphInfo_t *glyph;
    float useScale;
    fontInfo_t *font = &cgDC.Assets.textFont;
    if (scale <= cg_smallFont.value) {
        font = &cgDC.Assets.smallFont;
    } else if (scale > cg_bigFont.value) {
        font = &cgDC.Assets.bigFont;
    }
    useScale = scale * font->glyphScale;
    if (text) {
        // TTimo: FIXME
        //		const unsigned char *s = text;
        const char *s = text;
        trap_R_SetColor(color);
        memcpy(&newColor[0], &color[0], sizeof(vec4_t));
        len = strlen(text);
        if (limit > 0 && len > limit) {
            len = limit;
        }
        count = 0;
        while (s && *s && count < len) {
            glyph = &font->glyphs[(
                int)*s]; // TTimo: FIXME: getting nasty warnings without the
                         // cast, hopefully this doesn't break the VM build
            // int yadj = Assets.textFont.glyphs[text[i]].bottom +
            // Assets.textFont.glyphs[text[i]].top; float yadj = scale *
            // (Assets.textFont.glyphs[text[i]].imageHeight -
            // Assets.textFont.glyphs[text[i]].height);
            if (Q_IsColorString(s)) {
                memcpy(newColor, g_color_table[ColorIndex(*(s + 1))],
                       sizeof(newColor));
                newColor[3] = color[3];
                trap_R_SetColor(newColor);
                s += 2;
                continue;
            } else {
                float yadj = useScale * glyph->top;
                if (style == ITEM_TEXTSTYLE_SHADOWED ||
                    style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
                    int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
                    colorBlack[3] = newColor[3];
                    // check if current color already is black, if yes make a
                    // white shadow
                    if (color[0] == 0 && color[1] == 0 && color[2] == 0)
                        trap_R_SetColor(colorWhite);
                    else
                        trap_R_SetColor(colorBlack);
                    CG_Text_PaintChar(x + ofs, y - yadj + ofs,
                                      glyph->imageWidth, glyph->imageHeight,
                                      useScale, glyph->s, glyph->t, glyph->s2,
                                      glyph->t2, glyph->glyph);
                    colorBlack[3] = 1.0;
                    trap_R_SetColor(newColor);
                }
                CG_Text_PaintChar(x, y - yadj, glyph->imageWidth,
                                  glyph->imageHeight, useScale, glyph->s,
                                  glyph->t, glyph->s2, glyph->t2, glyph->glyph);
                // CG_DrawPic(x, y - yadj, scale *
                // cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale *
                // cgDC.Assets.textFont.glyphs[text[i]].imageHeight,
                // cgDC.Assets.textFont.glyphs[text[i]].glyph);
                x += (glyph->xSkip * useScale) + adjust;
                s++;
                count++;
            }
        }
        trap_R_SetColor(NULL);
    }
}


void CG_Text_PaintCenter(float x, float y, float scale, vec4_t color,
                         const char *text, float adjust, int limit, int style) {
    int len = CG_Text_Width(text, scale, 0);

    CG_Text_Paint(x - len / 2, y, scale, color, text, adjust, limit, style);
}

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
void CG_DrawField(int x, int y, int width, int value) {
    char num[16], *ptr;
    int l;
    int frame;

    if (width < 1) {
        return;
    }

    // draw number string
    if (width > 5) {
        width = 5;
    }

    switch (width) {
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
    l = strlen(num);
    if (l > width)
        l = width;
    x += 2 + CHAR_WIDTH * (width - l);

    ptr = num;
    while (*ptr && l) {
        if (*ptr == '-')
            frame = STAT_MINUS;
        else
            frame = *ptr - '0';

        CG_DrawPic(x, y, CHAR_WIDTH, CHAR_HEIGHT,
                   cgs.media.numberShaders[frame]);
        x += CHAR_WIDTH;
        ptr++;
        l--;
    }
}

/*
==============
CG_DrawMoneyField

Draws little money numbers for status bar and powerups
==============
*/
#define MONEY_WIDTH 10
#define MONEY_HEIGHT 14

void CG_DrawMoneyField(int x, int y, int width, int value) {
    char num[16], *ptr;
    int l;
    int frame;

    if (width < 1) {
        return;
    }

    // draw number string
    if (width > 5) {
        width = 5;
    }

    switch (width) {
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
    l = strlen(num);
    if (l > width)
        l = width;
    x += 2 + MONEY_WIDTH * (width - l);

    ptr = num;
    while (*ptr && l) {
        if (*ptr == '-')
            frame = STAT_MINUS;
        else
            frame = *ptr - '0';

        CG_DrawPic(x, y, MONEY_WIDTH, MONEY_HEIGHT,
                   cgs.media.numbermoneyShaders[frame]);
        x += MONEY_WIDTH + MONEY_WIDTH / 5;
        ptr++;
        l--;
    }
}

#define SCORE_WIDTH2 10
#define SCORE_WIDTH3 8
#define SCORE_HEIGHT2 14
#define SCORE_HEIGHT3 12
void CG_DrawScoreField(int x, int y, int value) {
    char num[16], *ptr;
    int l, w, h;

    value = value < 0 ? 0 : value;
    value = value > 999 ? 999 : value;

    Com_sprintf(num, sizeof(num), "%i", value);
    l = strlen(num);
    x += 2;
    if (l > 2) {
        w = SCORE_WIDTH3;
        h = SCORE_HEIGHT3;
        y += (SCORE_HEIGHT2 - SCORE_HEIGHT3) / 2;
    } else {
        w = SCORE_WIDTH2;
        h = SCORE_HEIGHT2;
        x += w * (2 - l);
    }

    ptr = num;
    while (*ptr && l) {
        CG_DrawPic(x, y, w, h, cgs.media.numbermoneyShaders[*ptr - '0']);
        x += w;
        ptr++;
        l--;
    }
}

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model,
                    qhandle_t skin, vec3_t origin, vec3_t angles) {
    refdef_t refdef;
    refEntity_t ent;

    if (!cg_draw3dIcons.integer || !cg_drawIcons.integer) {
        return;
    }

    CG_AdjustFrom640(&x, &y, &w, &h);

    memset(&refdef, 0, sizeof(refdef));

    memset(&ent, 0, sizeof(ent));
    AnglesToAxis(angles, ent.axis);
    VectorCopy(origin, ent.origin);
    ent.hModel = model;
    ent.customSkin = skin;
    ent.renderfx = RF_NOSHADOW; // no stencil shadows

    refdef.rdflags = RDF_NOWORLDMODEL;

    AxisClear(refdef.viewaxis);

    refdef.fov_x = 30;
    refdef.fov_y = 30;

    refdef.x = x;
    refdef.y = y;
    refdef.width = w;
    refdef.height = h;

    refdef.time = cg.time;

    trap_R_ClearScene();
    trap_R_AddRefEntityToScene(&ent);
    trap_R_RenderScene(&refdef);
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead(float x, float y, float w, float h, int clientNum,
                 vec3_t headAngles) {
    clipHandle_t cm;
    clientInfo_t *ci;
    float len;
    vec3_t origin;
    vec3_t mins, maxs;

    ci = &cgs.clientinfo[clientNum];

    if (cg_draw3dIcons.integer) {
        cm = ci->headModel;
        if (!cm) {
            return;
        }

        // offset the origin y and z to center the head
        trap_R_ModelBounds(cm, mins, maxs);

        origin[2] = -0.5 * (mins[2] + maxs[2]);
        origin[1] = 0.5 * (mins[1] + maxs[1]);

        // calculate distance so the head nearly fills the box
        // assume heads are taller than wide
        len = 0.7 * (maxs[2] - mins[2]);
        origin[0] = len / 0.268; // len / tan( fov/2 )

        // allow per-model tweaking
        VectorAdd(origin, ci->headOffset, origin);

        CG_Draw3DModel(x, y, w, h, ci->headModel, ci->headSkin, origin,
                       headAngles);
    } else if (cg_drawIcons.integer) {
        CG_DrawPic(x, y, w, h, ci->modelIcon);
    }

    // if they are deferred, draw a cross out
    if (ci->deferred) {
        CG_DrawPic(x, y, w, h, cgs.media.deferShader);
    }
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel(float x, float y, float w, float h, int team,
                      qboolean force2D) {
    qhandle_t cm;
    float len;
    vec3_t origin, angles;
    vec3_t mins, maxs;
    qhandle_t handle;

    if (!force2D && cg_draw3dIcons.integer) {

        VectorClear(angles);

        cm = cgs.media.redFlagModel;

        // offset the origin y and z to center the flag
        trap_R_ModelBounds(cm, mins, maxs);

        origin[2] = -0.5 * (mins[2] + maxs[2]);
        origin[1] = 0.5 * (mins[1] + maxs[1]);

        // calculate distance so the flag nearly fills the box
        // assume heads are taller than wide
        len = 0.5 * (maxs[2] - mins[2]);
        origin[0] = len / 0.268; // len / tan( fov/2 )

        angles[YAW] = 60 * sin(cg.time / 2000.0);
        ;

        if (team == TEAM_RED) {
            handle = cgs.media.redFlagModel;
        } else if (team == TEAM_BLUE) {
            handle = cgs.media.blueFlagModel;
        } else if (team == TEAM_FREE) {
            handle = cgs.media.neutralFlagModel;
        } else {
            return;
        }
        CG_Draw3DModel(x, y, w, h, handle, 0, origin, angles);
    } else if (cg_drawIcons.integer) {
        return;
    }
}

/*
================
CG_DrawStatusBarHead

================
*/

/*
================
CG_DrawStatusBarFlag

================
*/

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground(int x, int y, int w, int h, float alpha, int team) {
    vec4_t hcolor;

    hcolor[3] = 0;
    if (team == TEAM_RED) {
        hcolor[0] = 1;
        hcolor[1] = 0;
        hcolor[2] = 0;
    } else if (team == TEAM_BLUE) {
        hcolor[0] = 0;
        hcolor[1] = 0;
        hcolor[2] = 1;
    } else {
        return;
    }
    trap_R_SetColor(hcolor);
    CG_DrawPic(x, y, w, h, cgs.media.teamStatusBar);
    trap_R_SetColor(NULL);
}

/*
================
CG_DrawStatusBar

================
*/

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
=====================
CG_DrawDeathMessages
by Spoon
=====================
*/
#define CHARMESS 9
#define MESSAGETIME 5000
#define MESSICON 28

static float CG_DrawDeathMessages(float y) {
    int length[2], width, n, i, x;
    qhandle_t weapon;
    qboolean iscolor;

    static float color[4] = {1, 1, 1, 1};

    for (n = 0; n < MAX_DEATH_MESSAGES; n++) {

        if (cg.messagetime[n] + MESSAGETIME > cg.time && cg.messagetime[n]) {
            gitem_t *item = BG_FindItemForWeapon(cg.mod[n]);

            if (item)
                width = MESSICON * item->xyrelation;
            else
                width = MESSICON;

            if (!cg_weapons[cg.mod[n]].weaponIcon) {
                cg.messagetime[n] = 0;
                continue;
            }

            weapon = cg_weapons[cg.mod[n]].weaponIcon;

            iscolor = qfalse;

            for (length[0] = 0, i = 0; cg.attacker[n][i] != 0; i++) {
                if (cg.attacker[n][i] == '^' && cg.attacker[n][i + 1] != '^') {
                    iscolor = qtrue;
                    continue;
                }
                if (iscolor) {
                    iscolor = qfalse;
                    continue;
                }

                length[0]++;
            }

            iscolor = qfalse;

            for (length[1] = 0, i = 0; cg.target[n][i] != 0; i++) {
                if (cg.target[n][i] == '^' && cg.target[n][i + 1] != '^') {
                    iscolor = qtrue;
                    continue;
                }
                if (iscolor) {
                    iscolor = qfalse;
                    continue;
                }

                length[1]++;
            }

            x = 640 - width - CHARMESS * (length[0] + length[1]);

            // attacker
            CG_DrawStringExt(x, y + MESSICON - CHARMESS, cg.attacker[n], color,
                             qfalse, qfalse, CHARMESS, CHARMESS, 32);

            x += CHARMESS * length[0];

            // weapon
            CG_DrawPic(x, y, width, MESSICON, weapon);

            x += width;

            // target
            CG_DrawStringExt(x, y + MESSICON - CHARMESS, cg.target[n], color,
                             qfalse, qfalse, CHARMESS, CHARMESS, 32);

            y += MESSICON + MESSICON / 5;
        } else
            cg.messagetime[n] = 0;
    }

    return y;
}

/*
================
CG_DrawAttacker

================
*/

/*
================
CG_DrawSpeedMeter
By TheDoctor
================
*/
static float CG_DrawSpeedMeter(float y) {
    char *s;
    int w;
    vec_t *vel;
    int speed;

    /* speed meter can get in the way of the scoreboard */
    if (cg.scoreBoardShowing) {
        return y;
    }

    vel = cg.snap->ps.velocity;
    /* ignore vertical component of velocity */
    speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);

    s = va("%iu/s", speed);

    w = CG_DrawStrlen(s) * 11;

    CG_Text_Paint(640 - w, y + 5 + BIGCHAR_HEIGHT, 0.4f, colorWhite, s, 0, 0,
                  3);
    return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot(float y) {
    char *s;
    int w;

    s = va("time:%i snap:%i cmd:%i", cg.snap->serverTime, cg.latestSnapshotNum,
           cgs.serverCommandSequence);
    w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

    CG_DrawBigString(635 - w, y + 2, s, 1.0F);

    return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define FPS_FRAMES 4
static float CG_DrawFPS(float y) {
    char *s;
    static int previousTimes[FPS_FRAMES];
    static int index;
    int i, total;
    int fps;
    static int previous;
    int t, frameTime;

    // don't use serverTime, because that will be drifting to
    // correct for internet lag changes, timescales, timedemos, etc
    t = trap_Milliseconds();
    frameTime = t - previous;
    previous = t;

    previousTimes[index % FPS_FRAMES] = frameTime;
    index++;
    if (index > FPS_FRAMES) {
        float color[] = {1.0f, 1.0f, 1.0f, 1.0f};
        // average multiple frames together to smooth changes out a bit
        total = 0;
        for (i = 0; i < FPS_FRAMES; i++) {
            total += previousTimes[i];
        }
        if (!total) {
            total = 1;
        }
        fps = 1000 * FPS_FRAMES / total;

        s = va("%ifps", fps);
        CG_Text_Paint(585, y + 5 + BIGCHAR_HEIGHT, 0.4f, color, s, 0, 0, 3);
    }

    return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer(float y) {
    char *s;
    int w;
    int mins, seconds, tens;
    int msec;
    float color[] = {1.0f, 1.0f, 1.0f, 1.0f};

    msec = cg.time - cgs.levelStartTime;

    seconds = msec / 1000;
    mins = seconds / 60;
    seconds -= mins * 60;
    tens = seconds / 10;
    seconds -= tens * 10;

    s = va("%i:%i%i", mins, tens, seconds);
    w = CG_DrawStrlen(s) * 11;

    CG_Text_Paint(640 - w, y + 5 + BIGCHAR_HEIGHT, 0.4f, color, s, 0, 0, 3);

    return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTeamOverlay
=================
*/

/*
==================
CG_DrawDebugCvars
by: hika
2007-01-07
==================
*/
static float CG_DrawDebugCvars(float y) {
    char *s;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float X = 445.0f;

    s = va("[cg_boostfps]: %s", cg_boostfps.integer ? "On" : "Off");
    CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.3f, color, s, 0, 0, 3);
    y += TINYCHAR_HEIGHT + 5;

    return y;
}

/*
==================
CG_DrawStatusEntities
by: hika
2006-12-30

Show entities visibility status
on the right
==================
*/
static float CG_DrawStatusEntities(float y) {
    char *s;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float X = 445.0f;
    snapshot_t *snap;
    int i, j;
    int eType[ET_EVENTS + 3]
             [4]; // visible, solid, visible + solid, total + ET_EVENTS + EV_*
    int max_eType = ET_EVENTS + 2;
    centity_t *cent;
    localEntity_t *active, *le, *next;
    int leType[LE_SHOWREFENTITY + 2][2]; // local entities
    int max_leType = LE_SHOWREFENTITY + 1;

    if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport) {
        snap = cg.nextSnap;
    } else {
        snap = cg.snap;
    }

    memset(eType, 0, sizeof(eType));
    memset(leType, 0, sizeof(leType));

    // Fill status informations for entities
    for (i = 0; i < snap->numEntities; i++) {
        cent = &cg_entities[snap->entities[i].number];
        if (cent->currentState.eType > ET_EVENTS)
            j = max_eType - 1; // It is an event entity
        else
            j = cent->currentState.eType;

        eType[j][0]++;
        eType[max_eType][0]++;
        if (CG_IsEntityVisible(cent, 0)) {
            eType[j][1]++; // visible
            eType[max_eType][1]++;
            if (cent->nextState.solid) {
                eType[j][2]++; // solid
                eType[j][3]++; // visible and solid

                eType[max_eType][2]++;
                eType[max_eType][3]++;
            }
        } else if (cent->nextState.solid) {
            eType[j][2]++; // solid but not visible
            eType[max_eType][2]++;
        }
    }

    if (eType[max_eType][0] > 0) {
        s = va("=== Entities ===");
        CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0, 0, 3);
        y += TINYCHAR_HEIGHT + 4;

        // Then, display for each entity type, the total number in the snapshot
        // and the total number in the player's field of view.
        for (i = 0; i < max_eType; i++) {
            if (eType[i][0] > 0) {
                char *s_eType = NULL;
                switch (i) {
                case ET_GENERAL:
                    s_eType = "ET_GENERAL";
                    break;
                case ET_PLAYER:
                    s_eType = "ET_PLAYER";
                    break;
                case ET_ITEM:
                    s_eType = "ET_ITEM";
                    break;
                case ET_MISSILE:
                    s_eType = "ET_MISSILE";
                    break;
                case ET_MOVER:
                    s_eType = "ET_MOVER";
                    break;
                case ET_BEAM:
                    s_eType = "ET_BEAM";
                    break;
                case ET_PORTAL:
                    s_eType = "ET_PORTAL";
                    break;
                case ET_SPEAKER:
                    s_eType = "ET_SPEAKER";
                    break;
                case ET_PUSH_TRIGGER:
                    s_eType = "ET_PUSH_TRIGGER";
                    break;
                case ET_TELEPORT_TRIGGER:
                    s_eType = "ET_TELEPORT_TRIGGER";
                    break;
                case ET_INVISIBLE:
                    s_eType = "ET_INVISIBLE";
                    break;
                case ET_FLY:
                    s_eType = "ET_FLY";
                    break;
                case ET_GRAPPLE:
                    s_eType = "ET_GRAPPLE";
                    break;
                case ET_TEAM:
                    s_eType = "ET_TEAM";
                    break;
                case ET_BREAKABLE:
                    s_eType = "ET_BREAKABLE";
                    break;
                case ET_INTERMISSION:
                    s_eType = "ET_INTERMISSION";
                    break;
                case ET_FLARE:
                    s_eType = "ET_FLARE";
                    break;
                case ET_SMOKE:
                    s_eType = "ET_SMOKE";
                    break;
                case ET_TURRET:
                    s_eType = "ET_TURRET";
                    break;
                case ET_ESCAPE:
                    s_eType = "ET_ESCAPE";
                    break;
                case ET_EVENTS:
                    s_eType = "ET_EVENTS";
                    break;
                default:
                    s_eType = "EV_";
                    break;
                }
                if (s_eType != NULL) {
                    s = va("[%s]: %i/%i entities, %i/%i solids", s_eType,
                           eType[i][1], eType[i][0], eType[i][3], eType[i][2]);
                    CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0,
                                  0, 3);
                    y += TINYCHAR_HEIGHT + 4;
                }
            }
        }

        // Show total entities
        s = va("TOTAL: %i/%i entities, %i/%i solids", eType[i][1], eType[i][0],
               eType[i][3], eType[i][2]);
        CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0, 0, 3);
        y += TINYCHAR_HEIGHT + 4;
    }

    // Fill status information for local entities
    active = CG_GetActiveLocalEntities();
    // walk the list backwards
    le = active->prev;
    for (i = 0; le != active; le = next, i++) {
        int endTime = le->endTime;
        j = le->leType;
        // grab next now, so if the local entity is freed we
        // still have it
        next = le->prev;
        if (cg.time >= endTime)
            continue;

        if (j < max_leType) {
            leType[j][0]++;
            leType[max_leType][0]++; // Fill total local entities
            if (CG_IsLocalEntityVisible(le, 0)) {
                // Visible
                leType[j][1]++;
                leType[max_leType][1]++;
            }
        }
    }

    if (leType[max_leType][0] > 0) {
        s = va("=== Local Entities ===");
        CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0, 0, 3);
        y += TINYCHAR_HEIGHT + 4;

        // Then, display for each locale entity type
        for (i = 0; i < max_leType; i++) {
            if (leType[i][0] > 0) {
                char *s_leType = NULL;
                switch (i) {
                case LE_MARK:
                    s_leType = "LE_MARK";
                    break;
                case LE_SMOKE:
                    s_leType = "LE_SMOKE";
                    break;
                case LE_SPRITE_EXPLOSION:
                    s_leType = "LE_SPRITE_EXPLOSION";
                    break;
                case LE_EXPLOSION:
                    s_leType = "LE_EXPLOSION";
                    break;
                case LE_FRAGMENT: // gibs and brass
                    s_leType = "LE_FRAGMENT";
                    break;
                case LE_MOVE_SCALE_FADE: // water bubbles
                    s_leType = "LE_MOVE_SCALE_FADE";
                    break;
                case LE_FADE_RGB: // teleporters, railtrails
                    s_leType = "LE_FADE_RGB";
                    break;
                case LE_FALL_SCALE_FADE: // gib blood trails
                    s_leType = "LE_FALL_SCALE_FADE";
                    break;
                case LE_SCALE_FADE: // rocket trails
                    s_leType = "LE_SCALE_FADE";
                    break;
                }

                if (s_leType != NULL) {
                    s = va("[%s]: %i/%i local entities", s_leType, leType[i][1],
                           leType[i][0]);
                    CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0,
                                  0, 3);
                    y += TINYCHAR_HEIGHT + 4;
                }
            }
        }

        // Show total local entities
        s = va("TOTAL: %i/%i local entities", leType[i][1], leType[i][0]);
        CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0, 0, 3);
        y += TINYCHAR_HEIGHT + 4;
    }

    return y;
}

/*
==================
CG_DrawCameraInfo
by: hika
2007-01-06

==================
*/
static float CG_DrawCameraInfo(float y) {
    char *s;
    float color[] = {1, 1, 1, 1};
    float X = 445.0f;
    int i;

    s = va("=== Camera Origin ===");
    CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0, 0, 3);
    y += TINYCHAR_HEIGHT + 5;

    s = va("[ %f, %f, %f ]", cg.refdef.vieworg[0], cg.refdef.vieworg[1],
           cg.refdef.vieworg[2]);
    CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0, 0, 3);
    y += TINYCHAR_HEIGHT + 5;

    s = va("=== Camera Axis ===");
    CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0, 0, 3);
    y += TINYCHAR_HEIGHT + 5;

    for (i = 0; i < 3; i++) {
        s = va("[ %f, %f, %f ]", cg.refdef.viewaxis[i][0],
               cg.refdef.viewaxis[i][1], cg.refdef.viewaxis[i][2]);
        CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0, 0, 3);
        y += TINYCHAR_HEIGHT + 5;
    }

    s = va("=== Frustum Plane%s ===", (sa_engine_inuse ? " (SG engine)" : ""));
    CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0, 0, 3);
    y += TINYCHAR_HEIGHT + 5;
    for (i = 0; i < 4; i++) {
        s = va("[%i].normal : [ %f, %f, %f ]", i, cg_frustum[i].normal[0],
               cg_frustum[i].normal[1], cg_frustum[i].normal[2]);
        CG_Text_Paint(X, y + 5 + TINYCHAR_HEIGHT, 0.2f, color, s, 0, 0, 3);
        y += TINYCHAR_HEIGHT + 5;
    }

    return y;
}

/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight(stereoFrame_t stereoFrame) {
    float y;

    y = 0;

    if (cg_drawSnapshot.integer) {
        y = CG_DrawSnapshot(y);
    }
    if (cg_drawFPS.integer &&
        (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT)) {
        y = CG_DrawFPS(y);
    }
    if (cg_drawTimer.integer) {
        y = CG_DrawTimer(y);
    }
    if (cg_drawspeed.integer) {
        y = CG_DrawSpeedMeter(y);
    }
    if (cg_drawdebug.integer) {
        // Draw debug info on the upper right
        y = CG_DrawDebugCvars(y);
        y = CG_DrawCameraInfo(y);
        y = CG_DrawStatusEntities(y);
    }

    y = CG_DrawDeathMessages(y);
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/

/*
================
CG_DrawPowerups
================
*/

/*
=====================
CG_DrawLowerRight

=====================
*/

/*
===================
CG_DrawPickupItem
===================
*/
static int CG_DrawPickupItem(int y) {
    int value;
    int quantity;
    float *fadeColor;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    float size = ICON_SIZE;

    if (cg.snap->ps.stats[STAT_HEALTH] <= 0) {
        return y;
    }
    value = cg.itemPickup;
    quantity = cg.itemPickupQuant;

    if (bg_itemlist[value].xyrelation > 3) {
        size = ICON_SIZE * 3 / bg_itemlist[value].xyrelation;
    }

    y = 480 - 62 - 10 - size;

    if (value) {
        fadeColor = CG_FadeColor(cg.itemPickupTime, 3000);

        if (fadeColor) {
            qhandle_t pic = cg_items[value].icon;

            if (!Q_stricmp(bg_itemlist[value].classname, "pickup_money")) {

                if (quantity < COINS)
                    pic = cgs.media.coins_pic;
                else if (quantity < BILLS)
                    pic = cgs.media.bills_pic;
            }

            color[3] = fadeColor[3];
            CG_RegisterItemVisuals(value);
            trap_R_SetColor(fadeColor);
            CG_DrawPic(320 - 143, y, size * bg_itemlist[value].xyrelation, size,
                       pic);
            trap_R_SetColor(NULL);

            if (!Q_stricmp(bg_itemlist[value].classname, "pickup_money")) {
                CG_Text_Paint(320 - 143 + size * bg_itemlist[value].xyrelation +
                                  10,
                              y + (size / 2) + 10, 0.35f, color,
                              va("Money: %i$", quantity), 0, -1, 3);
            } else {
                CG_Text_Paint(320 - 143 + size * bg_itemlist[value].xyrelation +
                                  10,
                              y + (size / 2) + 10, 0.35f, color,
                              bg_itemlist[value].pickup_name, 0, -1, 3);
            }
        }
    }

    return y;
}

/*
=====================
CG_DrawLowerLeft

=====================
*/
static void CG_DrawLowerLeft(void) {
    float y;

    y = 480 - ICON_SIZE;


    y = CG_DrawPickupItem(y);
}

//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
static void CG_DrawChatMessages(void) {
    int i, x, y;
    int w = CHATCHAR_WIDTH;
    int h = CHATCHAT_HEIGHT;
    vec4_t hcolor = {1.0f, 1.0f, 1.0f, 1.0f};
    int chatHeight;

    if (cg_chatHeight.integer < CHAT_HEIGHT)
        chatHeight = cg_chatHeight.integer;
    else
        chatHeight = CHAT_HEIGHT;
    if (chatHeight <= 0)
        return; // disabled

    if (cgs.LastChatPos != cgs.ChatPos) {
        if (cg.time - cgs.ChatMsgTimes[cgs.LastChatPos % chatHeight] >
            cg_chatTime.integer) {
            cgs.LastChatPos++;
        }

        // Set position on main screen
        switch (cg_chatPosition.integer) {
        case 0:
        default:
            x = 0;
            y = 330;
            break;
        case 1:
            x = 0;
            y = 160;
            break;
        case 2:
            x = 180;
            y = 410;
            break;
        }

        // Update char size
        if (cg_chatBigText.integer) {
            w = 2 * CHATCHAR_WIDTH;
            h = 2 * CHATCHAT_HEIGHT;
        }

        for (i = cgs.ChatPos - 1; i >= cgs.LastChatPos; i--) {
            CG_DrawStringExt(x + w, y - (cgs.ChatPos - i) * h,
                             cgs.ChatMsgs[i % chatHeight], hcolor, qfalse,
                             qfalse, w, h, 0);
        }
    }
}

/*
===================
CG_DrawHoldableItem
===================
*/


/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward(void) {
    float *color;
    int i, count;
    float x, y;
    char buf[32];

    if (!cg_drawRewards.integer) {
        return;
    }

    color = CG_FadeColor(cg.rewardTime, REWARD_TIME);
    if (!color) {
        if (cg.rewardStack > 0) {
            for (i = 0; i < cg.rewardStack; i++) {
                cg.rewardSound[i] = cg.rewardSound[i + 1];
                cg.rewardShader[i] = cg.rewardShader[i + 1];
                cg.rewardCount[i] = cg.rewardCount[i + 1];
            }
            cg.rewardTime = cg.time;
            cg.rewardStack--;
            color = CG_FadeColor(cg.rewardTime, REWARD_TIME);
            trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
        } else {
            return;
        }
    }

    trap_R_SetColor(color);

    /*
    count = cg.rewardCount[0]/10;				// number of big
    rewards to draw

    if (count) {
            y = 4;
            x = 320 - count * ICON_SIZE;
            for ( i = 0 ; i < count ; i++ ) {
                    CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4,
    cg.rewardShader[0] ); x += (ICON_SIZE*2);
            }
    }

    count = cg.rewardCount[0] - count*10;		// number of small
    rewards to draw
    */

    if (cg.rewardCount[0] >= 10) {
        y = 56;
        x = 320 - ICON_SIZE / 2;
        CG_DrawPic(x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader[0]);
        Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
        x = (SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen(buf)) / 2;
        CG_DrawStringExt(x, y + ICON_SIZE, buf, color, qfalse, qtrue,
                         SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
    } else {

        count = cg.rewardCount[0];

        y = 56;
        x = 320 - count * ICON_SIZE / 2;
        for (i = 0; i < count; i++) {
            CG_DrawPic(x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader[0]);
            x += ICON_SIZE;
        }
    }
    trap_R_SetColor(NULL);
}

#define IND_SIZE 32.0f
void CG_DrawEscapeIndicators(void) {
    vec3_t dir, angles, viewangles;
    float angle;
    qhandle_t pic;
    float x, y;
    float height, width;

    if (!cg_showescape.integer)
        return;

    if (!cg.snap->ps.powerups[PW_GOLD] || cgs.gametype != GT_BR)
        return;

    // angle to the escapepoint
    VectorSubtract(cg.escapePoint, cg.refdef.vieworg, dir);
    vectoangles(dir, angles);
    angles[YAW] = AngleNormalize180(angles[YAW]);

    // angle of the player
    VectorCopy(cg.refdefViewAngles, viewangles);
    viewangles[YAW] = AngleNormalize180(viewangles[YAW]);

    // difference
    angle = AngleNormalize180(viewangles[YAW] - angles[YAW]);

    // see where we have to draw the indicator
    if (angle >= -45 && angle <= 45) { // forward
        pic = cgs.media.indicate_fw;
        y = 0;
        x = (angle + 45.0f) / 90.0f * 640.0f - IND_SIZE / 2.0f;
        width = IND_SIZE;
        height = IND_SIZE / 2.0f;
    } else if (angle >= -135 && angle < -45) { // left
        pic = cgs.media.indicate_lf;
        x = 0;
        y = (1.0f - (angle + 135.0f) / 90.0f) * 480.0f - IND_SIZE / 2.0f;
        width = IND_SIZE / 2.0f;
        height = IND_SIZE;
    } else if (angle > 45 && angle <= 135) { // left
        pic = cgs.media.indicate_rt;
        y = (angle - 45.0f) / 90.0f * 480.0f - IND_SIZE / 2.0f;
        width = IND_SIZE / 2.0f;
        height = IND_SIZE;
        x = 640 - width;
    } else { // back
        pic = cgs.media.indicate_bk;

        if (angle < 0)
            x = (1.0f - (angle + 180.0f + 45.0f) / 90.0f) * 640.0f -
                IND_SIZE / 2.0f;
        else
            x = (1.0f - (angle - 135.0f) / 90.0f) * 640.0f - IND_SIZE / 2.0f;

        width = IND_SIZE;
        height = IND_SIZE / 2.0f;
        y = 480 - height;
    }

    // draw the indicator
    trap_R_SetColor(NULL);
    CG_DrawPic(x, y, width, height, pic);
}

/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_SAMPLES 128

typedef struct {
    int frameSamples[LAG_SAMPLES];
    int frameCount;
    int snapshotFlags[LAG_SAMPLES];
    int snapshotSamples[LAG_SAMPLES];
    int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo(void) {
    int offset;

    offset = cg.time - cg.latestSnapshotTime;
    lagometer.frameSamples[lagometer.frameCount & (LAG_SAMPLES - 1)] = offset;
    lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo(snapshot_t *snap) {
    // dropped packet
    if (!snap) {
        lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] =
            -1;
        lagometer.snapshotCount++;
        return;
    }

    // add this snapshot's info
    lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] =
        snap->ping;
    lagometer.snapshotFlags[lagometer.snapshotCount & (LAG_SAMPLES - 1)] =
        snap->snapFlags;
    lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect(void) {
    float x, y;
    int cmdNum;
    usercmd_t cmd;
    const char *s;
    int w;

    // draw the phone jack if we are completely past our buffers
    cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
    trap_GetUserCmd(cmdNum, &cmd);
    if (cmd.serverTime <= cg.snap->ps.commandTime ||
        cmd.serverTime > cg.time) { // special check for map_restart
        return;
    }

    // also add text in center of screen
    s = "Connection Interrupted";
    w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
    CG_DrawBigString(320 - w / 2, 100, s, 1.0F);

    // blink the icon
    if ((cg.time >> 9) & 1) {
        return;
    }

    x = 640 - 48;
    y = 480 - 48;

    CG_DrawPic(x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga"));
}

#define MAX_LAGOMETER_PING 900
#define MAX_LAGOMETER_RANGE 300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer(void) {
    int a, x, y, i;
    float v;
    float ax, ay, aw, ah, mid, range;
    int color;
    float vscale;

    // unlagged - misc
    if (!cg_lagometer.integer) {
// unlagged - misc
        CG_DrawDisconnect();
        return;
    }

    if (!cg.scoreBoardShowing && cg_lagometer.integer != 2)
        return;

    //
    // draw the graph
    //
    if (cg_lagometer.integer == 2) {
        x = 640 - 162;
        y = 480 - 48;
    } else {
        x = 640 - 48;
        y = 480 - 48;
    }

    trap_R_SetColor(NULL);
    if (cg_lagometer.integer == 2) {
        vec4_t lagocolor = {1.0f, 1.0f, 1.0f, 0.33f};
        trap_R_SetColor(lagocolor);
    }
    CG_DrawPic(x, y, 48, 48, cgs.media.lagometerShader);

    ax = x;
    ay = y;
    aw = 48;
    ah = 48;
    CG_AdjustFrom640(&ax, &ay, &aw, &ah);

    color = -1;
    range = ah / 3;
    mid = ay + range;

    vscale = range / MAX_LAGOMETER_RANGE;

    // draw the frame interpoalte / extrapolate graph
    for (a = 0; a < aw; a++) {
        i = (lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
        v = lagometer.frameSamples[i];
        v *= vscale;
        if (v > 0) {
            if (color != 1) {
                color = 1;
                trap_R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
            }
            if (v > range) {
                v = range;
            }
            trap_R_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0,
                                  cgs.media.whiteShader);
        } else if (v < 0) {
            if (color != 2) {
                color = 2;
                trap_R_SetColor(g_color_table[ColorIndex(COLOR_BLUE)]);
            }
            v = -v;
            if (v > range) {
                v = range;
            }
            trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0,
                                  cgs.media.whiteShader);
        }
    }

    // draw the snapshot latency / drop graph
    range = ah / 2;
    vscale = range / MAX_LAGOMETER_PING;

    for (a = 0; a < aw; a++) {
        i = (lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
        v = lagometer.snapshotSamples[i];
        if (v > 0) {
            if (lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED) {
                if (color != 5) {
                    color = 5; // YELLOW for rate delay
                    trap_R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
                }
            } else {
                if (color != 3) {
                    color = 3;
                    trap_R_SetColor(g_color_table[ColorIndex(COLOR_GREEN)]);
                }
            }
            v = v * vscale;
            if (v > range) {
                v = range;
            }
            trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0,
                                  cgs.media.whiteShader);
        } else if (v < 0) {
            if (color != 4) {
                color = 4; // RED for dropped snapshots
                trap_R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
            }
            trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0,
                                  0, 0, cgs.media.whiteShader);
        }
    }

    trap_R_SetColor(NULL);

    if (cg_nopredict.integer || cg_synchronousClients.integer) {
        CG_DrawBigString(x, y, "snc", 1.0);
    }

    CG_DrawDisconnect();
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint(const char *str, int y, int charWidth) {
    char *s;

    // play sound (new)
    trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER,
                      cgs.media.bang[rand() % 3]);

    Q_strncpyz(cg.centerPrint, str, sizeof(cg.centerPrint));

    cg.centerPrintTime = cg.time;
    cg.centerPrintY = y;
    cg.centerPrintCharWidth = charWidth;

    // count the number of lines for centering
    cg.centerPrintLines = 1;
    s = cg.centerPrint;
    while (*s) {
        if (*s == '\n')
            cg.centerPrintLines++;
        s++;
    }
}

/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString(void) {
    int w;
    vec4_t color2 = {0.85f, 0.69f, 0.04f, 1.00f};
    float scale;
    float *color;

    if (!cg.centerPrintTime) {
        return;
    }

    color = CG_FadeColor(cg.centerPrintTime, 1000 * cg_centertime.value);
    if (!color) {
        return;
    }

    color2[3] = color[3];

#define FIRST_PRINT 50
#define PRINT_SCALE 150

    if (cg.time - cg.centerPrintTime < FIRST_PRINT)
        scale = 1.25f;
    else if (cg.time - cg.centerPrintTime < FIRST_PRINT + PRINT_SCALE) {
        scale =
            (float)(cg.time - cg.centerPrintTime - FIRST_PRINT) / PRINT_SCALE;
        scale *= 0.5;
        scale = 1.25f - scale;
    } else
        scale = 0.75f;

    w = CG_DrawStrlen(cg.centerPrint) * 20 * scale;
    UI_DrawProportionalString2(320 - w / 2, 160, cg.centerPrint, color2, scale,
                               cgs.media.charsetProp);
    trap_R_SetColor(NULL);
    return;

}

/*
================================================================================

CROSSHAIR

================================================================================
*/

/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(qboolean changeCrosshair, qboolean isPlayer)
{
    float w, h;
    qhandle_t hShader;
    float f;
    float x, y;
    int ca;

    vec4_t colorActivate = {1.0f, 0.75f, 0.0f, 1.0f};
    qboolean drawFriendWarning = qfalse;

    if (cg.snap->ps.persistant[PERS_TEAM] >= TEAM_SPECTATOR) {
        return;
    }

    if (cg.renderingThirdPerson) {
        return;
    }

    if (cg.snap->ps.stats[STAT_WP_MODE] == 1 &&
        cg.snap->ps.weapon == WP_SHARPS) {
        float x, y, w, h;
        qhandle_t hShader = cgs.media.scopeShader;

        x = 0;
        y = 0;
        w = 640;
        h = 480;

        CG_DrawPic(x, y, w, h, hShader);

        y = 38;
        x = 118;
        h = 480 - 2 * y;
        w = 640 - 2 * x;

        if (changeCrosshair) {
            if (isPlayer) {
                trap_R_SetColor(colorRed);
            } else {
                trap_R_SetColor(colorActivate);
            }
        }

        CG_DrawPic(x, y, w, h, cgs.media.scopecrossShader);

        // Zoom
        CG_ZoomDown_f();
        return;
    } // modified by Spoon

    CG_ZoomUp_f();

    if (!cg_drawCrosshair.integer) {
        return;
    }

    // if a pickable item is in front of the player
    if (cgs.gametype == GT_DUEL && cg.introend - DU_INTRO_DRAW <= cg.time &&
        cg.introend + DU_CROSSHAIR_FADE >= cg.time) {
        vec4_t colorFade = {1.0f, 1.0f, 1.0f, 0.0f};

        if (cg.introend + DU_CROSSHAIR_START <= cg.time) {
            colorFade[3] = (float)(cg.time - cg.introend - DU_CROSSHAIR_START) /
                           DU_CROSSHAIR_FADE;
        }

        trap_R_SetColor(colorFade);
    } else if (changeCrosshair) {
        if (isPlayer) {
            // change crosshair to indicate that target is a teammate
            drawFriendWarning = qtrue;
        } else {
            trap_R_SetColor(colorActivate);
        }
    }

    w = h = cg_crosshairSize.value;

    // pulse the size of the crosshair when picking up items
    f = cg.time - cg.itemPickupBlendTime;
    if (f > 0 && f < ITEM_BLOB_TIME) {
        f /= ITEM_BLOB_TIME;
        w *= (1 + f);
        h *= (1 + f);
    }

    x = cg_crosshairX.integer;
    y = cg_crosshairY.integer;
    CG_AdjustFrom640(&x, &y, &w, &h);

    ca = cg_drawCrosshair.integer;
    if (ca < 0) {
        ca = 0;
    }
    hShader = cgs.media.crosshairShader[ca % NUM_CROSSHAIRS];

    trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - w),
                          y + cg.refdef.y + 0.5 * (cg.refdef.height - h), w, h,
                          0, 0, 1, 1, hShader);

    if (drawFriendWarning) {
        trap_R_SetColor(colorRed);
        hShader = cgs.media.crosshairFriendShader;
        CG_DrawPic(x + 320 - w / 2, y + 240 - h / 2, w, h, hShader);
    }

    trap_R_SetColor(NULL);
}

/*
=================
CG_DrawCrosshair3D
=================
*/
static void CG_DrawCrosshair3D(void) {
    float w;
    qhandle_t hShader;
    float f;
    int ca;

    trace_t trace;
    vec3_t endpos;
    float stereoSep, zProj, maxdist, xmax;
    char rendererinfos[128];
    refEntity_t ent;

    if (!cg_drawCrosshair.integer) {
        return;
    }

    if (cg.snap->ps.persistant[PERS_TEAM] >= TEAM_SPECTATOR) {
        return;
    }

    if (cg.renderingThirdPerson) {
        return;
    }

    w = cg_crosshairSize.value;

    // pulse the size of the crosshair when picking up items
    f = cg.time - cg.itemPickupBlendTime;
    if (f > 0 && f < ITEM_BLOB_TIME) {
        f /= ITEM_BLOB_TIME;
        w *= (1 + f);
    }

    ca = cg_drawCrosshair.integer;
    if (ca < 0) {
        ca = 0;
    }
    hShader = cgs.media.crosshairShader[ca % NUM_CROSSHAIRS];

    // Use a different method rendering the crosshair so players don't see two
    // of them when focusing their eyes at distant objects with high stereo
    // separation We are going to trace to the next shootable object and place
    // the crosshair in front of it.

    // first get all the important renderer information
    trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos,
                                   sizeof(rendererinfos));
    zProj = atof(rendererinfos);
    trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos,
                                   sizeof(rendererinfos));
    stereoSep = zProj / atof(rendererinfos);

    xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);

    // let the trace run through until a change in stereo separation of the
    // crosshair becomes less than one pixel.
    maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
    VectorMA(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
    CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, endpos, 0, MASK_SHOT);

    memset(&ent, 0, sizeof(ent));
    ent.reType = RT_SPRITE;
    ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;

    VectorCopy(trace.endpos, ent.origin);

    // scale the crosshair so it appears the same size for all distances
    ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
    ent.customShader = hShader;

    trap_R_AddRefEntityToScene(&ent);
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity(qboolean *changeCrosshair,
                                      qboolean *isPlayer) {
    trace_t trace;
    vec3_t start, end;
    int content;

    VectorCopy(cg.refdef.vieworg, start);
    VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

    CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
             cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY);
    // Tequila comment: In SG, we want to check through windows entities and
    // activable entities, so we don't return early

    // if the player is in fog, don't show it
    content = CG_PointContents(trace.endpos, 0);
    if (content & CONTENTS_FOG) {
        return;
    }

    // can spot teammates through windows
    while (trace.surfaceFlags & SURF_GLASS) {
        VectorCopy(trace.endpos, start);
        CG_Trace(&trace, start, vec3_origin, vec3_origin, end, trace.entityNum,
                 CONTENTS_SOLID | CONTENTS_BODY);

        // try to avoid creating an endless loop,
        // this can happen with some lamps and fake windows
        if (VectorCompare(trace.endpos, start))
            break;
    }

    // update the fade timer
    cg.crosshairClientNum = trace.entityNum;
    cg.crosshairClientTime = cg.time;

    *changeCrosshair = qfalse;
    *isPlayer = qfalse;
    if (trace.entityNum < MAX_CLIENTS &&
        cg_entities[trace.entityNum].currentState.eType == ET_PLAYER) {
        *isPlayer = qtrue;

        if (cgs.gametype >= GT_TEAM &&
            cg.snap->ps.persistant[PERS_TEAM] < TEAM_SPECTATOR &&
            cgs.clientinfo[trace.entityNum].team ==
                cg.snap->ps.persistant[PERS_TEAM]) {
            // player on same team
            *changeCrosshair = qtrue;
        }
    } else if (cg_entities[trace.entityNum].currentState.eType == ET_MOVER &&
               cg_entities[trace.entityNum].currentState.angles2[0] == -1000 &&
               Distance(cg.snap->ps.origin, trace.endpos) < ACTIVATE_RANGE) {
        // activatable entities, doors
        *changeCrosshair = qtrue;
    } else if (cg_entities[trace.entityNum].currentState.eType == ET_TURRET &&
               Distance(cg.snap->ps.origin,
                        cg_entities[trace.entityNum].currentState.pos.trBase) <
                   ACTIVATE_RANGE) {
        // gatling gun
        *changeCrosshair = qtrue;
    }
}

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames(qboolean isPlayer) {
    float *color;
    char *name;
    float w;

    if (!isPlayer) {
        return;
    }
    if (!cg_drawCrosshair.integer) {
        return;
    }
    if (!cg_drawCrosshairNames.integer) {
        return;
    }
    if (cg.renderingThirdPerson) {
        return;
    }

    if (cg.snap->ps.pm_type == PM_CHASECAM &&
        cg.crosshairClientNum == cg.snap->ps.stats[CHASECLIENT]) {
        return;
    }

    if (Distance(cg.snap->ps.origin,
                 cg_entities[cg.crosshairClientNum].lerpOrigin) > 250)
        return;

    // draw the name of the player being looked at
    color = CG_FadeColor(cg.crosshairClientTime, 1000);
    if (!color) {
        trap_R_SetColor(NULL);
        return;
    }

    name = cgs.clientinfo[cg.crosshairClientNum].name;
    color[3] *= 0.5f;
    w = CG_Text_Width(name, 0.3f, 0);
    CG_Text_Paint(320 - w / 2, 190, 0.3f, color, name, 0, 0,
                  ITEM_TEXTSTYLE_SHADOWED);
    trap_R_SetColor(NULL);
}

//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
    const char *info;

    if (cg.scoreBoardShowing)
        return;

    info = CG_ConfigString(CS_SERVERINFO);

    if (cg.snap->ps.persistant[PERS_TEAM] >= TEAM_SPECTATOR) {

        if (cgs.gametype == GT_DUEL) {
            CG_DrawSmallString(
                5, 445,
                "Press \"FIRE\" to change to the next player in this mappart",
                1.0F);
            CG_DrawSmallString(
                5, 460, "Press \"FIRE2\" to change to the next mappart", 1.0F);
            if (cg.snap->ps.pm_flags & PMF_FOLLOW)
                CG_DrawSmallString(
                    5, 475, "Press \"JUMP\" to get to the free spectator mode",
                    1.0F);
        } else {
            CG_DrawSmallString(
                5, 445, "Press \"FIRE\" to change to the next player", 1.0F);
            CG_DrawSmallString(
                5, 460, "Press \"FIRE2\" to change to the previous player",
                1.0F);
            if ((!(atoi(Info_ValueForKey(info, "g_chaseonly")) &&
                   atoi(Info_ValueForKey(info, "g_gametype")) >= GT_RTP) &&
                 (cg.snap->ps.pm_flags & PMF_FOLLOW)))
                CG_DrawSmallString(
                    5, 475, "Press \"JUMP\" to get to the free spectator mode",
                    1.0F);
        }
    } else if (cgs.gametype == GT_DUEL &&
               !(cg.snap->ps.pm_flags & PMF_FOLLOW)) {
        float color[] = {1.0f, 0.75f, 0.15f, 1.0f};

        if (cg.introend >= cg.time)
            CG_Text_PaintCenter(320, 320, 0.3f, color,
                                "Use the buy-menu to buy new pistols.", 0, 0,
                                3);

        if (cg.snap->ps.stats[STAT_FLAGS] & SF_DU_WON)
            CG_Text_PaintCenter(320, 320, 0.3f, color,
                                "Press \"FIRE\" to watch the remaining duels",
                                0, 0, 3);
    }
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
    char *s;
    int sec;

    if (!cgs.voteTime) {
        return;
    }

    // play a talk beep whenever it is modified
    if (cgs.voteModified) {
        cgs.voteModified = qfalse;
        CG_PlayTalkSound();
    }

    sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
    if (sec < 0) {
        sec = 0;
    }
    s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes,
           cgs.voteNo);
    CG_DrawSmallString(0, 58, s, 1.0F);
    s = "or press ESC then click Vote";
    CG_DrawSmallString(0, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F);
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
    char *s;
    int sec, cs_offset;

    if (cgs.clientinfo[cg.clientNum].team == TEAM_RED)
        cs_offset = 0;
    else if (cgs.clientinfo[cg.clientNum].team == TEAM_BLUE)
        cs_offset = 1;
    else
        return;

    if (!cgs.teamVoteTime[cs_offset]) {
        return;
    }

    // play a talk beep whenever it is modified
    if (cgs.teamVoteModified[cs_offset]) {
        cgs.teamVoteModified[cs_offset] = qfalse;
        CG_PlayTalkSound();
    }

    sec = (VOTE_TIME - (cg.time - cgs.teamVoteTime[cs_offset])) / 1000;
    if (sec < 0) {
        sec = 0;
    }
    s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
           cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset]);
    CG_DrawSmallString(0, 90, s, 1.0F);
}

void Menu_UpdatePosition(menuDef_t *menu);

static qboolean CG_DrawScoreboard(void) {
    static qboolean firstTime = qtrue;

    if (menuScoreboard) {
        menuScoreboard->window.flags &= ~WINDOW_FORCED;
    }
    if (cg_paused.integer) {
        cg.deferredPlayerLoading = 0;
        firstTime = qtrue;
        return qfalse;
    }

    // should never happen in Team Arena
    if (cgs.gametype == GT_SINGLE_PLAYER &&
        cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
        cg.deferredPlayerLoading = 0;
        firstTime = qtrue;
        return qfalse;
    }

    // don't draw scoreboard during death while warmup up
    if (cg.warmup && !cg.showScores) {
        return qfalse;
    }

    if (cg.showScores ||
        (cg.predictedPlayerState.pm_type == PM_DEAD &&
         !(cg.snap->ps.pm_flags & PMF_FOLLOW)) ||
        cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
    } else {
        if (!CG_FadeColor(cg.scoreFadeTime, FADE_TIME)) {
            // next time scoreboard comes up, don't print killer
            cg.deferredPlayerLoading = 0;
            cg.killerName[0] = 0;
            firstTime = qtrue;
            return qfalse;
        }
    }

    if (menuScoreboard == NULL) {
        if (cgs.gametype >= GT_TEAM) {
            menuScoreboard = Menus_FindByName("teamscore_menu");
        } else {
            menuScoreboard = Menus_FindByName("score_menu");
        }
    }

    if (menuScoreboard) {
        if (firstTime) {
            CG_SetScoreSelection(menuScoreboard);
            firstTime = qfalse;
        }
        menuScoreboard->window.rect.x = 0;

        menuScoreboard->window.rect.y = (cgs.gametype >= GT_TEAM) ? -20 : 0;
        Menu_UpdatePosition(menuScoreboard);
        Menu_Paint(menuScoreboard, qtrue);
    }

    // load any models that have been deferred
    if (++cg.deferredPlayerLoading > 10) {
        CG_LoadDeferredPlayers();
    }

    return qtrue;
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission(void) {
//	int key;
    // if (cg_singlePlayer.integer) {
    //	CG_DrawCenterString();
    //	return;
    // }
    cg.scoreFadeTime = cg.time;
    cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow(void) {
    vec4_t color, colorrect;
    const char *name;
    char health[4];
    int x, w;

    if (!(cg.snap->ps.pm_flags & PMF_FOLLOW)) {
        return qfalse;
    }
    color[0] = 1.0f;
    color[1] = 1.0f;
    color[2] = 1.0f;
    color[3] = 1.0f;

    colorrect[0] = 0.0f;
    colorrect[1] = 0.0f;
    colorrect[2] = 0.0f;
    colorrect[3] = 0.3f;

    if (cg.snap->ps.pm_type == PM_CHASECAM) {
        name = cgs.clientinfo[cg.snap->ps.stats[CHASECLIENT]].name;
        CG_FillRect(0, 410, 640, 70, colorrect);
        CG_DrawStringExt(120, 415, name, color, qtrue, qfalse, 14, 14, 0);
        CG_DrawStringExt(370, 415, "health: ", color, qtrue, qfalse, 14, 14, 0);

        Com_sprintf(health, sizeof(health), "%i",
                    cg.snap->ps.stats[STAT_HEALTH]);

        CG_DrawStringExt(467, 415, health, color, qtrue, qfalse, 14, 14, 0);
    } else {
        name = cgs.clientinfo[cg.snap->ps.clientNum].name;

        x = 0.5 * (640 - 10 * CG_DrawStrlen("Following"));
        CG_DrawStringExt(x, 340, "Following", color, qfalse, qfalse, 10, 10, 0);

        w = CG_Text_Width(name, 0.5f, 0);
        CG_Text_Paint(320 - w / 2, 375, 0.5f, color, name, 0, 0,
                      ITEM_TEXTSTYLE_SHADOWED);
    }

    return qtrue;
}

/*
=================
CG_DrawAmmoWarning
=================
*/


/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup(void) {
    int w;
    int sec;
    int i;
    float scale;
    clientInfo_t *ci1, *ci2;
    const char *s;
    vec4_t color = {0.85f, 0.69f, 0.04f, 1.0f};

    sec = cg.warmup;
    if (!sec) {
        return;
    }

    if (sec < 0) {
        s = "Waiting for players";
        w = CG_DrawStrlen(s) * 20 * 0.75f;
        UI_DrawProportionalString2(320 - w / 2, 24, s, color, 0.75f,
                                   cgs.media.charsetProp);
        cg.warmupCount = 0;
        return;
    }

    if (cgs.gametype == GT_DUEL) {
        // find the two active players
        ci1 = NULL;
        ci2 = NULL;
        for (i = 0; i < cgs.maxclients; i++) {
            if (cgs.clientinfo[i].infoValid &&
                cgs.clientinfo[i].team == TEAM_FREE) {
                if (!ci1) {
                    ci1 = &cgs.clientinfo[i];
                } else {
                    ci2 = &cgs.clientinfo[i];
                }
            }
        }

        if (ci1 && ci2) {
            s = va("%s vs %s", ci1->name, ci2->name);
            w = CG_Text_Width(s, 0.6f, 0);
            CG_Text_Paint(320 - w / 2, 60, 0.6f, colorWhite, s, 0, 0,
                          ITEM_TEXTSTYLE_SHADOWEDMORE);
        }
    } else {
        if (cgs.gametype == GT_FFA) {
            s = "Free For All";
        } else if (cgs.gametype == GT_TEAM) {
            s = "Team Deathmatch";
        } else {
            s = "";
        }
        w = CG_Text_Width(s, 0.6f, 0);
        CG_Text_Paint(320 - w / 2, 90, 0.6f, colorWhite, s, 0, 0,
                      ITEM_TEXTSTYLE_SHADOWEDMORE);
    }

    sec = (sec - cg.time) / 1000;
    if (sec < 0) {
        cg.warmup = 0;
        sec = 0;
    }
    s = va("Starts in: %i", sec + 1);
    if (sec != cg.warmupCount) {
        cg.warmupCount = sec;
        trap_S_StartLocalSound(cgs.media.bang[rand() % 3], CHAN_ANNOUNCER);
    }

    switch (cg.warmupCount) {
    case 0:
        scale = 0.54f;
        break;
    case 1:
        scale = 0.51f;
        break;
    case 2:
        scale = 0.48f;
        break;
    default:
        scale = 0.45f;
        break;
    }

    w = CG_Text_Width(s, scale, 0);
    CG_Text_Paint(320 - w / 2, 125, scale, colorWhite, s, 0, 0,
                  ITEM_TEXTSTYLE_SHADOWEDMORE);
}

//==================================================================================
/*
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus(void) {
    if (cg.voiceTime) {
        int t = cg.time - cg.voiceTime;
        if (t > 2500) {
            Menus_CloseByName("voiceMenu");
            trap_Cvar_Set("cl_conXOffset", "0");
            cg.voiceTime = 0;
        }
    }
}

// maximum of 6 menu items
menu_items_t menu_items[MAX_MENU_ITEMS];

void CG_ClearFocusses(void) {
    int i;

    for (i = 0; i < menuBuy->itemCount; i++) {
        menuBuy->items[i]->window.flags &= ~WINDOW_HASFOCUS;
    }
    for (i = 0; i < menuItem->itemCount; i++) {
        menuItem->items[i]->window.flags &= ~WINDOW_HASFOCUS;
    }
}

static qboolean CG_CursorIsInRect(rectDef_t *rect) {
    if (cgs.cursorX >= rect->x && cgs.cursorX <= rect->x + rect->w &&
        cgs.cursorY >= rect->y && cgs.cursorY <= rect->y + rect->h)
        return qtrue;

    return qfalse;
}

// movement control disabled
static qboolean Menu_IsKeyDown(char *cmd) {
    int i;

    if (!Q_stricmp(cmd, "mouse1")) {

        // check if buy menu is open
        if (cg.menu == MENU_BUY) {
            int j;
            qboolean abort = qfalse;

            // stats
            if (cgs.gametype != GT_DUEL) {
                for (j = 0; j < menuBuy->itemCount; j++) {
                    if (menuBuy->items[j]->window.flags & WINDOW_HASFOCUS) {
                        int oldmenustat = cg.menustat;

                        // goto next menu if mousebutton is pressed
                        if (trap_Key_IsDown(K_MOUSE1) &&
                            CG_CursorIsInRect(
                                &menuBuy->items[j]->window.rect)) {
                            cg.menustat = menuBuy->items[j]->window.ownerDraw -
                                          CG_BUTTON_PISTOLS + 1;

                            // ammo -> buy the ammo
                            if (cg.menustat >= 6) {
                                return qtrue;
                            } else if ((!cg.menuitem ||
                                        oldmenustat != cg.menustat)) {
                                cg.menumove = qfalse;
                                cg.menuitem = 1;
                                CG_ClearFocusses();
                                menuBuy->items[j]->window.flags |=
                                    WINDOW_HASFOCUS;
                            }
                            cg.oldbutton = qtrue;
                            return qfalse;
                        }
                        abort = qtrue;
                        break;
                    }
                }
            }
            // items
            for (j = 0; j < menuItem->itemCount && cg.menuitem; j++) {
                if (menuItem->items[j]->window.flags & WINDOW_HASFOCUS) {
                    int olditem = cg.menuitem;
                    cg.menuitem =
                        menuItem->items[j]->window.ownerDraw - CG_BUTTON1 + 1;

                    if ((cg.menuitem > 3 || cg.menuitem < 1) &&
                        cgs.gametype == GT_DUEL) {
                        cg.menuitem = olditem;
                        return qfalse;
                    }

                    // if cursor is in the button
                    if (CG_CursorIsInRect(&menuItem->items[j]->window.rect))
                        abort = qfalse;
                    else
                        abort = qtrue;
                    break;
                }
            }
            if (!abort && trap_Key_IsDown(K_MOUSE1))
                return qtrue;
        }
    }

    // we don't need other buttons for the buy menu
    if (cg.menu == MENU_BUY)
        return qfalse;

    if (!Q_stricmp(cmd, "+attack")) {
        for (i = 0; i < 2; i++)
            if (trap_Key_IsDown(cg_button_attack[i].integer))
                return qtrue;
    } else if (!Q_stricmp(cmd, "+button6")) {
        for (i = 0; i < 2; i++) {
            if (trap_Key_IsDown(cg_button_altattack[i].integer))
                return qtrue;
        }
    } /* else if(!Q_stricmp(cmd, "+forward")){
             for(i=0; i<2; i++){
                     if(trap_Key_IsDown(cg_button_forward[i].integer))
                             return qtrue;
             }
     } else if(!Q_stricmp(cmd, "+back")){
             for(i=0; i<2; i++){
                     if(trap_Key_IsDown(cg_button_back[i].integer))
                             return qtrue;
             }
     } else if(!Q_stricmp(cmd, "+moveleft")){
             for(i=0; i<2; i++){
                     if(trap_Key_IsDown(cg_button_moveleft[i].integer))
                             return qtrue;
             }
     } else if(!Q_stricmp(cmd, "+moveright")){
             for(i=0; i<2; i++){
                     if(trap_Key_IsDown(cg_button_moveright[i].integer))
                             return qtrue;
             }
     } else if(!Q_stricmp(cmd, "+left")){
             for(i=0; i<2; i++){
                     if(trap_Key_IsDown(cg_button_turnleft[i].integer))
                             return qtrue;
             }
     } else if(!Q_stricmp(cmd, "+right")){
             for(i=0; i<2; i++){
                     if(trap_Key_IsDown(cg_button_turnright[i].integer))
                             return qtrue;
             }
     }*/
    return qfalse;
}

float color[6][4] = {
    {1.0f, 1.0f, 1.0f, 1.0f},  {1.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 0.5f},  {1.0f, 1.0f, 1.0f, 0.9f},
    {0.0f, 0.0f, 0.0f, 0.75f}, {1.0f, 0.8f, 0.0f, 1.0f},
};

// set up the item-menu
static void CG_SetupItemMenu(void) {
    gitem_t *item;
    int i;
    int count = 0;
    qboolean clone = qfalse; // second "cloned" pistol

    item = bg_itemlist + 1;

    for (i = 1; item->classname; item++) {
        int belt = 1;

        if (cg.snap->ps.powerups[PW_BELT])
            belt = 2;

        if (item->weapon_sort != cg.menustat) {
            continue;
        };

        Q_strncpyz(menu_items[count].string, item->pickup_name, 64);

        if ((item->giTag == WP_DYNAMITE || item->giTag == WP_KNIFE ||
             item->giTag == WP_MOLOTOV) &&
            item->giType == IT_WEAPON) {
            gitem_t *ammoitem;
            ammoitem = BG_FindItemForAmmo(item->giTag);
            Q_strncpyz(menu_items[count].string,
                       va("%s(%i)", ammoitem->pickup_name, ammoitem->quantity),
                       64);
        }

        // check if clone
        if ((cg.snap->ps.stats[STAT_WEAPONS] & (1 << item->giTag)) &&
            item->giType == IT_WEAPON &&
            !(cg.snap->ps.stats[STAT_FLAGS] & SF_SEC_PISTOL) &&
            bg_weaponlist[item->giTag].wp_sort == WPS_PISTOL) {
            clone = qtrue;
        }

        // different colors depending on money, the player has
        //  yellow
        if (!clone &&
            (((cg.snap->ps.stats[STAT_WEAPONS] & (1 << item->giTag)) &&
              item->giType == IT_WEAPON && item->giTag != WP_DYNAMITE &&
              item->giTag != WP_KNIFE && item->giTag != WP_MOLOTOV) ||
             (item->giTag == PW_BELT && cg.snap->ps.powerups[PW_BELT] &&
              item->giType == IT_POWERUP) ||
             (item->giType == IT_ARMOR && cg.snap->ps.stats[STAT_ARMOR]) ||
             (item->giType == IT_POWERUP && item->giTag == PW_SCOPE &&
              cg.snap->ps.powerups[PW_SCOPE]) ||
             (((item->giTag == WP_DYNAMITE || item->giTag == WP_KNIFE ||
                item->giTag == WP_MOLOTOV) &&
               item->giType == IT_WEAPON) &&
              cg.snap->ps.ammo[item->giTag] >=
                  bg_weaponlist[item->giTag].maxAmmo))) {
            menu_items[count].inventory = qtrue;
        } else
            menu_items[count].inventory = qfalse;

        // red
        if (cg.snap->ps.stats[STAT_MONEY] < item->prize) {
            menu_items[count].money = qfalse;
        } else
            menu_items[count].money = qtrue;

        count++;
        i++;
    }

    menuItemCount = count;
}

static void CG_RefreshItemMenu(void) {
    int i = 0;

    // cycle through items and make them all visible
    for (i = 0; i < menuItem->itemCount; i++) {
        menuItem->items[i]->window.flags |= WINDOW_VISIBLE;
    }

    menuItem->window.rect.y = (cg.menustat - 1) * 24;
    Menu_UpdatePosition(menuItem);
    CG_SetupItemMenu();

    // make unnessecary buttons invisible, also place the endrectangle
    for (i = 0; i < menuItem->itemCount; i++) {
        int ownerDraw = menuItem->items[i]->window.ownerDraw;

        if (ownerDraw >= CG_BUTTON1 && ownerDraw <= CG_BUTTON6 &&
            ownerDraw - CG_BUTTON1 + 1 > menuItemCount) {
            menuItem->items[i]->window.flags &= ~WINDOW_VISIBLE;
        }

        if (ownerDraw == CG_BUY_END) {
            menuItem->items[i]->window.rect.y =
                menuItem->window.rect.y + 28 + menuItemCount * 24;
        }
    }
}

static void CG_ClearItemMenu(void) {
    int i;

    menuItemCount = 0;
    cg.menuitem = 0;

    for (i = 0; i < MAX_MENU_ITEMS; i++) {
        menu_items[i].inventory = qfalse;
        menu_items[i].money = qfalse;
        strcpy(menu_items[i].string, "");
    }
}

void CG_CloseBuyMenu(void) {
    cg.menu = MENU_NONE;
    cg.menustat = 0;
    cg.menuitem = 0;
    cg.oldbutton = qtrue;

    CG_ClearItemMenu();
    CG_ClearFocusses();

    Menus_CloseByName("buymenu");
    Menus_CloseByName("buymenu_items");

    trap_Key_SetCatcher(0);

    // Notify the server we close the buy menu.
    // The "client" engine code will reset BUTTON_BUYMENU to usercmd_t.buttons .
    trap_SendConsoleCommand("-button8");
}

static qboolean menu_exit(void) {

    // Esc closes menu
    if (!(trap_Key_GetCatcher() & KEYCATCH_CGAME)) {
        CG_CloseBuyMenu();
        return qtrue;
    }

    if ((trap_Key_IsDown(K_MOUSE2) || (trap_Key_IsDown(48) && !cg.menuitem)) &&
        !cg.oldbutton) {
        CG_CloseBuyMenu();
        return qtrue;
    }

    if (cg.introend < cg.time && cgs.gametype == GT_DUEL) {
        CG_CloseBuyMenu();
        return qtrue;
    }

    return qfalse;
}

// ITEMBOX
static gitem_t *CG_GetBuyItem(void) {
    gitem_t *item;
    int j;

    item = bg_itemlist + 1;

    for (j = 0; item->classname; item++) {
        if (item->weapon_sort != cg.menustat)
            continue;
        j++;

        if (j != cg.menuitem)
            continue;

        break;
    }

    return item;
}

// this has to be done because of a bug in the menu system
static void CG_CheckFocusses(void) {
    int i;

    // if(cg.menuitem && cgs.gametype != GT_DUEL)
    //	return;

    if (cgs.gametype != GT_DUEL) {
        for (i = 0; i < menuBuy->itemCount; i++) {
            if (CG_CursorIsInRect(&menuBuy->items[i]->window.rect) &&
                Menu_IsKeyDown("mouse1")) {
                CG_ClearFocusses();
                menuBuy->items[i]->window.flags |= WINDOW_HASFOCUS;
                break;
            }
        }
    } else {
        for (i = 0; i < menuItem->itemCount; i++) {
            if (CG_CursorIsInRect(&menuItem->items[i]->window.rect)) {
                CG_ClearFocusses();
                menuItem->items[i]->window.flags |= WINDOW_HASFOCUS;
                break;
            }
        }
    }
}

static void CG_DisableStatMenu(void) {
    int i;

    for (i = 0; i < menuBuy->itemCount; i++) {
        float color[] = {0.5, 0.5, 0.5, 1.0};
        menuBuy->items[i]->window.flags |= (WINDOW_GREY | WINDOW_DECORATION);
        Vector4Copy(color, menuBuy->items[i]->window.foreColor);
    }
}
/*
=================
CG_DrawBuyMenu
by Spoon
=================
*/
static void CG_DrawBuyMenu(void) {
    int count;
    int i;
    gitem_t *item;
    qboolean numpressed = qfalse, ammo_numpressed = qfalse;
    qboolean duel = (cgs.gametype == GT_DUEL);

    if (cg_buydebug.integer) {
        if (trap_Key_IsDown('p') && !cg.oldbutton) {
            trap_SendConsoleCommand("screenshot\n");
            cg.oldbutton = qtrue;
        }
    }

    if (menu_exit())
        return;

    CG_CheckFocusses();

    if (duel)
        CG_DisableStatMenu();

    // key handling
    if (!duel) {
        if (cg.menumove) {
            // cycle through the number keys (0-9)
            for (i = 1; i <= 9; i++) {

                if (trap_Key_IsDown(i + 48) &&
                    !cg.oldbutton) { // value of '0' is dec 48
                    cg.menustat = i;
                    if (cg.menustat <= 5)
                        cg.menumove = qfalse;
                    cg.menuitem = 1;
                    cg.oldbutton = qtrue;
                    ammo_numpressed = qtrue;
                    CG_ClearFocusses();
                    break;
                }
            }

            // movement keys
            if (Menu_IsKeyDown("+back") && !cg.oldbutton) {
                if (cg.menustat < 7) {
                    cg.menustat++;
                    cg.menumove = qtrue;
                    cg.oldbutton = qtrue;
                    CG_ClearFocusses();
                }
            } else if (Menu_IsKeyDown("+forward") && !cg.oldbutton) {
                if (cg.menustat > 1) {
                    cg.menustat--;
                    cg.menumove = qtrue;
                    cg.oldbutton = qtrue;
                    CG_ClearFocusses();
                }
            } else if ((Menu_IsKeyDown("+moveright") ||
                        Menu_IsKeyDown("+right")) &&
                       !cg.oldbutton && cg.menustat < 6) {
                cg.menumove = qfalse;
                cg.oldbutton = qtrue;
                cg.menuitem = 1;
                CG_ClearFocusses();
            }
        }

        /*
        ====================================
        //prepare next menu screen
        ====================================
        */

        if (!cg.menustat || cg.menumove || cg.menustat > 5) {

            // check if ammunition is required
            if (((Menu_IsKeyDown("mouse1") && !cg.oldbutton) ||
                 ammo_numpressed) &&
                cg.menustat <= 7) {

                if (cg.menustat == 6)
                    item = BG_FindItemForAmmo(WP_BULLETS_CLIP);
                else
                    item = BG_FindItemForAmmo(WP_CART_CLIP);

                if (item->classname) {
                    trap_SendConsoleCommand(va("cg_buy %s\n", item->classname));
                    cg.oldbutton = qtrue;

                    if (ammo_numpressed) {
                        CG_CloseBuyMenu();
                    }
                }
            }
            // delete item menu data, item menu will not show up
            CG_ClearItemMenu();
            return;
        }
    }

    // is there anything to be updated?
    CG_RefreshItemMenu();

    count = menuItemCount;

    /*
    ====================================
    //draw item box
    ====================================
    */

    for (i = 1; i <= count; i++) {

        if (trap_Key_IsDown(i + 48) && !cg.oldbutton) { // value of '0' is dec
                                                        // 48
            cg.menuitem = i;
            cg.oldbutton = qtrue;
            numpressed = qtrue;
            CG_ClearFocusses();
            break;
        }
    }
    if (Menu_IsKeyDown("+back") && !cg.oldbutton) {
        if (cg.menuitem < count) {
            cg.menuitem++;
            cg.menumove = qfalse;
            cg.oldbutton = qtrue;
            CG_ClearFocusses();
        }
    } else if (Menu_IsKeyDown("+forward") && !cg.oldbutton) {
        if (cg.menuitem > 1) {
            cg.menuitem--;
            cg.menumove = qfalse;
            cg.oldbutton = qtrue;
            CG_ClearFocusses();
        }
    } else if ((Menu_IsKeyDown("+moveleft") || Menu_IsKeyDown("+left") ||
                trap_Key_IsDown(0 + 48)) &&
               !cg.oldbutton && !duel) {
        cg.menumove = qtrue;
        cg.menuitem = 0;
        cg.oldbutton = qtrue;
        CG_ClearFocusses();
    } else if (trap_Key_IsDown(0 + 48) && duel && !cg.oldbutton) {
        CG_CloseBuyMenu();
        return;
    }

    if (!cg.menuitem)
        return;

    if ((Menu_IsKeyDown("mouse1") && !cg.oldbutton) || numpressed) {
        qboolean gatling = (cg.snap->ps.weapon == WP_GATLING &&
                            cg.snap->ps.stats[STAT_GATLING_MODE]);

        item = CG_GetBuyItem();

        if (cg.snap->ps.stats[STAT_MONEY] < item->prize) {
            // TheDoctor: if we cannot buy, don't get stuck in the menu
            trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER,
                              cgs.media.bang[1]);
            cg.oldbutton = qtrue;
        } else {
            // don't buy other special weapons if handling a gatling
            if (item->giType == IT_WEAPON &&
                bg_weaponlist[item->giTag].wp_sort == WPS_GUN && gatling) {
                CG_Printf(
                    "Can't buy special weapons when using a gatling gun.\n");
                return;
            }
            if (item->giType == IT_WEAPON) {
                if (bg_weaponlist[item->giTag].wp_sort == WPS_PISTOL) {

                    if (cg.snap->ps.stats[STAT_WEAPONS] & (1 << item->giTag) &&
                        (cg.snap->ps.stats[STAT_FLAGS] & SF_SEC_PISTOL))
                        return;

                } else if ((cg.snap->ps.stats[STAT_WEAPONS] &
                            (1 << item->giTag)) &&
                           item->weapon_sort != WS_MISC) {
                    return;
                }
            } else {
                if (item->giTag == PW_BELT && cg.snap->ps.powerups[PW_BELT])
                    return;
                if (item->giType == IT_ARMOR && cg.snap->ps.stats[STAT_ARMOR])
                    return;
                if (item->giTag == PW_SCOPE && cg.snap->ps.powerups[PW_SCOPE])
                    return;
            }

            trap_SendConsoleCommand(va("cg_buy %s\n", item->classname));
            //			trap_S_StartSound(NULL, cg.snap->ps.clientNum,
            //CHAN_WEAPON, cgs.media.buySound);
            cg.oldbutton = qtrue;

            if (item->giType == IT_WEAPON && item->weapon_sort != WS_MISC &&
                cgs.gametype != GT_DUEL) {
                // don't change to the weapon bought when handling a gatling
                if (!gatling) {
                    cg.lastusedweapon = cg.weaponSelect;
                    cg.weaponSelect = item->giTag;
                }
            }
        }

        if (numpressed) {
            CG_CloseBuyMenu();
        }
    }
}

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(stereoFrame_t stereoFrame) {
    qboolean isPlayer = qfalse;
    qboolean changeCrosshair = qfalse;

    if (cgs.orderPending && cg.time > cgs.orderTime) {
        CG_CheckOrderPending();
    }
    // if we are taking a levelshot for the menu, don't draw anything
    if (cg.levelShot) {
        return;
    }

    if (cg_draw2D.integer == 0) {
        return;
    }

    if (cg.snap->ps.pm_type == PM_INTERMISSION) {
        CG_DrawIntermission();
        return;
    }

    /*
            if (cg.cameraMode) {
                    return;
            }
    */

    // Draw chat messages
    CG_DrawChatMessages();

    if (!CG_DrawFollow()) {
        if (cg_warmupmessage.integer)
            CG_DrawWarmup();
    }

    CG_DrawSpectator();

    CG_ScanForCrosshairEntity(&changeCrosshair, &isPlayer);

    if (cg.snap->ps.persistant[PERS_TEAM] >= TEAM_SPECTATOR) {
        if (stereoFrame == STEREO_CENTER)
            CG_DrawCrosshair(changeCrosshair, isPlayer);
        CG_DrawCrosshairNames(isPlayer);
    } else {
        // don't draw any status if dead or the scoreboard is being explicitly
        // shown
        if (cg.snap->ps.stats[STAT_HEALTH] > 0 && !cg.introstart) {

            if (stereoFrame == STEREO_CENTER)
                CG_DrawCrosshair(changeCrosshair, isPlayer);
            CG_DrawCrosshairNames(isPlayer);
            CG_DrawWeaponSelect();
            CG_DrawReward();
        }

        if (cg_drawStatus.integer) {
            Menu_PaintAll();
            CG_DrawTimedMenus();
            // Added for Smokin' Guns
            CG_DrawEscapeIndicators();
        }
    }

    CG_DrawVote();
    CG_DrawTeamVote();

    CG_DrawLagometer();

    if (!cg_paused.integer) {
        CG_DrawUpperRight(stereoFrame);
    }

    CG_DrawLowerLeft();


    // don't draw center string if scoreboard is up
    cg.scoreBoardShowing = CG_DrawScoreboard();
    CG_DrawCenterString();

    // draw menu in every case
    if (cg.menu == MENU_BUY) {
        // intialize the positions
        menuBuy->window.rect.x = save_menuBuy.x;
        menuBuy->window.rect.y = save_menuBuy.y;
        Menu_UpdatePosition(menuBuy);
        menuItem->window.rect.x = save_menuItem.x;
        menuItem->window.rect.y = save_menuItem.y;
        Menu_UpdatePosition(menuItem);

        // initialize the whole menu
        CG_DrawBuyMenu();

        // paint
        Menu_Paint(menuBuy, qtrue);
        if (cg.menuitem) {
            Menu_Paint(menuItem, qtrue);
        }

        // draw the cursor
        CG_DrawPic(cgs.cursorX - 16, cgs.cursorY - 16, 32, 32,
                   cgDC.Assets.cursor);
    }
}


static void CheckWeaponChange(void) {
    playerState_t *ps = &cg.snap->ps;
    static int delay;
    int cmdNum;
    usercmd_t cmd;

    // weapon change
    if (cg.markedweapon) {
        // fix for the fire-button-dead bug
        // Tequila comment: see
        // http://www.quake3world.com/forum/viewtopic.php?f=16&t=17815
        if (delay & 1) {
            cmdNum = trap_GetCurrentCmdNumber();
            trap_GetUserCmd(cmdNum, &cmd);
            if (cmd.buttons & BUTTON_CHOOSE_MODE) {
                trap_SendConsoleCommand("-button14");
                // CG_Printf("CheckWeaponChange: BUTTON_CHOOSE_MODE FIX
                // (delay=%d)\n", delay);
            }
        }
        delay++;

        if (Menu_IsKeyDown("+attack")) {
            if (cg.markedweapon != cg.weaponSelect)
                cg.lastusedweapon = cg.weaponSelect;

            if (ps->stats[STAT_FLAGS] & SF_SEC_PISTOL &&
                cg.markedweapon != WP_AKIMBO &&
                bg_weaponlist[cg.markedweapon].wp_sort == WPS_PISTOL &&
                cg.markedfirstpistol) {
                // Player has two same pistols and change to one pistol,
                // the one in the left holster.

                if (cg.markedweapon == cg.weaponSelect) {
                    // Last selected weapon was the other pistol of the same
                    // type. Before the change, the current one will become the
                    // secondary pistol and will be in the left holster.
                    cg.lastusedweapon = WP_SEC_PISTOL;
                }

                // Keep the selected weapon type.
                // cg.weaponSelect will be restored in
                // -> ./cgame/cg_view.c, CG_DrawActiveFrame()
                cg._weaponSelect = cg.markedweapon;
                cg.weaponSelect = WP_SEC_PISTOL;
            } else
                cg.weaponSelect = cg.markedweapon;

            cg.weaponSelectTime = cg.time;
            cg.markedweapon = 0;

            trap_Cvar_Set("cl_menu", "0");
        }

        if (trap_Key_IsDown(K_ESCAPE)) {
            cg.markedweapon = 0;
            trap_Cvar_Set("cl_menu", "0");
            // signal BUTTON_CHOOSE_CANCEL to the server
            trap_SendConsoleCommand("+button13; wait; -button13");
        }

        if ((Menu_IsKeyDown("+button6") || ps->stats[STAT_GATLING_MODE])) {
            cg.markedweapon = 0;
            trap_Cvar_Set("cl_menu", "0");
        }
    } else {
        delay = 0;
    }

    // scope check
    if (cg.oldscopestat != ps->stats[STAT_WP_MODE] &&
        ps->stats[STAT_WP_MODE] < 0 && ps->stats[STAT_WP_MODE] != -3 &&
        ps->weapon == WP_SHARPS && ps->powerups[PW_SCOPE]) {

        cg.scopetime = cg.time;
        cg.scopedeltatime = SCOPE_TIME;

        if (ps->stats[STAT_WP_MODE] == -1)
            cg.scopedeltatime *= -1;
    }
    cg.oldscopestat = ps->stats[STAT_WP_MODE];

    // reload sound check
    /*if(cg.snap->ps.weapon2state != WEAPON_RELOADING &&
            cg.snap->ps.weaponstate != WEAPON_RELOADING){

            cg.r_weapon = qfalse;

    } else if(!cg.r_weapon){

            if(cg.snap->ps.weapon2state == WEAPON_RELOADING &&
                    !(cg.snap->ps.stats[STAT_FLAGS] & SF_RELOAD2)){ // <-akimbos
                    CG_PlayReloadSound(cg.snap->ps.weapon2,
    &cg_entities[cg.snap->ps.clientNum]); cg.r_weapon = qtrue; } else
    if(cg.snap->ps.weaponstate == WEAPON_RELOADING &&
                    !(cg.snap->ps.stats[STAT_FLAGS] & SF_RELOAD)){ // <-akimbos
                    CG_PlayReloadSound(cg.snap->ps.weapon,
    &cg_entities[cg.snap->ps.clientNum]); cg.r_weapon = qtrue;
            }
    }*/
}

static void CG_AddNumber(refEntity_t *re, int number, int size) {
    vec3_t origin, dir, vec, up = {0, 0, 1};
    int i, digits[10], numdigits, negative;
    float offset;

    VectorCopy(re->origin, origin);

    VectorSubtract(cg.refdef.vieworg, origin, dir);
    CrossProduct(dir, up, vec);
    VectorNormalize(vec);

    negative = qfalse;
    if (number < 0) {
        negative = qtrue;
        number = -number;
    }

    for (numdigits = 0; !(numdigits && !number); numdigits++) {
        digits[numdigits] = number % 10;
        number = number / 10;
    }

    if (negative) {
        digits[numdigits] = 10;
        numdigits++;
    }

    offset = ((float)((numdigits - 1) * size)) * 0.5f;
    if (offset)
        VectorMA(origin, offset, vec, origin);
    for (i = 0; i < numdigits; i++) {
        VectorMA(origin, (float)(-i * size), vec, re->origin);
        re->customShader = cgs.media.numberShaders[digits[numdigits - 1 - i]];
        trap_R_AddRefEntityToScene(re);
    }
}

static void CG_DrawNodes(void) {
    refEntity_t node;
    int i;

    memset(&node, 0, sizeof(node));

    for (i = 0; i < ai_nodepointer; i++) {
        vec3_t angles;

        node.customShader = 0;
        node.hModel = cgs.media.ai_node;
        VectorCopy(ai_nodes[i], node.origin);
        VectorCopy(ai_nodes[i], node.lightingOrigin);
        node.renderfx = 0;
        node.radius = 0.0f;
        vectoangles(ai_angles[i], angles);
        AnglesToAxis(angles, node.axis);
        node.reType = RT_MODEL;
        trap_R_AddRefEntityToScene(&node);

        // Draw node number
        node.hModel = 0;
#define AINODE_NUMBERS_SIZE 5
        node.radius = AINODE_NUMBERS_SIZE * 0.75f;
        node.renderfx = RF_DEPTHHACK;
        node.reType = RT_SPRITE;
        node.origin[2] += 15.0f;
        CG_AddNumber(&node, i, AINODE_NUMBERS_SIZE);
    }
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive(stereoFrame_t stereoView) {
    // optionally draw the info screen instead
    if (!cg.snap) {
        CG_DrawInformation();
        return;
    }

    CheckWeaponChange();

    CG_PlayMusic();

    if (cg_cheats)
        CG_DrawNodes();

    // clear around the rendered view if sized down
    CG_TileClear();

    if (stereoView != STEREO_CENTER)
        CG_DrawCrosshair3D();

    // draw 3D view
    trap_R_RenderScene(&cg.refdef);

    // draw status bar and other floating elements
    CG_Draw2D(stereoView);
}
