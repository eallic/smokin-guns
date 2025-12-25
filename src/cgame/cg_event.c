/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2003 Iron Claw Interactive
Copyright (C) 2005-2012 Smokin' Guns

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
// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"

// for the voice chats
#include "../ui/ui_menudef.h"
//==========================================================================

/*
===================
CG_PlaceString

Also called by scoreboard drawing
===================
*/
const char *CG_PlaceString(int rank) {
    static char str[64];
    char *s, *t;

    if (rank & RANK_TIED_FLAG) {
        rank &= ~RANK_TIED_FLAG;
        t = "Tied for ";
    } else {
        t = "";
    }

    if (rank == 1) {
        s = S_COLOR_BLUE "1st" S_COLOR_BLACK; // draw in blue
    } else if (rank == 2) {
        s = S_COLOR_RED "2nd" S_COLOR_BLACK; // draw in red
    } else if (rank == 3) {
        s = S_COLOR_YELLOW "3rd" S_COLOR_BLACK; // draw in yellow
    } else if (rank == 11) {
        s = S_COLOR_WHITE "11th" S_COLOR_BLACK;
    } else if (rank == 12) {
        s = S_COLOR_WHITE "12th" S_COLOR_BLACK;
    } else if (rank == 13) {
        s = S_COLOR_WHITE "13th" S_COLOR_BLACK;
    } else if (rank % 10 == 1) {
        s = va("%s%ist%s", S_COLOR_WHITE, rank, S_COLOR_BLACK);
    } else if (rank % 10 == 2) {
        s = va("%s%ind%s", S_COLOR_WHITE, rank, S_COLOR_BLACK);
    } else if (rank % 10 == 3) {
        s = va("%s%ird%s", S_COLOR_WHITE, rank, S_COLOR_BLACK);
    } else {
        s = va("%s%ith%s", S_COLOR_WHITE, rank, S_COLOR_BLACK);
    }

    Com_sprintf(str, sizeof(str), "%s%s", t, s);
    return str;
}

static void CG_Hit_Far_Message(float ratio, int otherEntityNum) {
    if (otherEntityNum < 0 || otherEntityNum >= MAX_CLIENTS)
        return;

    if (!cg_hitmsg.value || ratio < cg_hitmsg.value)
        return;

    CG_Printf("^xAt this range, your weapon's damage was reduced to %.1f%%\n",
              100 - ratio);
}

static void CG_Hit_Message(entityState_t *ent) {
    const char *info;
    char name[32];
    clientInfo_t *ci;
    int target = ent->otherEntityNum, shooter = ent->otherEntityNum2;
    int nameNum;
    int count = 0;
    int i;

    if (!cg_hitmsg.integer && shooter == cg.snap->ps.clientNum)
        return;

    if (!cg_ownhitmsg.integer && target == cg.snap->ps.clientNum)
        return;

    // is this client affected by the shoot?
    if (shooter != cg.snap->ps.clientNum && target != cg.snap->ps.clientNum)
        return;

    // name must be the other player
    if (target == cg.snap->ps.clientNum)
        nameNum = shooter;
    else
        nameNum = target;

    if (nameNum < 0 || nameNum >= MAX_CLIENTS) {
        return;
    }
    ci = &cgs.clientinfo[nameNum];

    info = CG_ConfigString(CS_PLAYERS + nameNum);
    if (!info) {
        return;
    }
    Q_strncpyz(name, Info_ValueForKey(info, "n"), sizeof(name) - 2);
    strcat(name, S_COLOR_WHITE);

    // if we have to print several locations
    for (i = 0; i < 3; i++) {
        int j;

        if (ent->angles2[i] != -1) {
            qboolean del = qfalse;

            // check if this location is already stored
            for (j = 0; j < i; j++) {
                if (ent->angles2[i] == ent->angles2[j]) {

                    // front
                    if (ent->weapon) {
                        if (!Q_stricmp(
                                hit_info[(int)ent->angles2[i]].forename,
                                hit_info[(int)ent->angles2[j]].forename)) {
                            del = qtrue;
                        }
                    } else if (!Q_stricmp(
                                   hit_info[(int)ent->angles2[i]].backname,
                                   hit_info[(int)ent->angles2[j]].backname)) {
                        del = qtrue;
                    }
                }
            }

            // delete this location if nessecary
            if (del) {
                for (j = i; j < 2; j++) {
                    ent->angles2[j] = ent->angles2[j + 1];
                }
                ent->angles2[2] = -1;
                continue;
            }

            count++;
        }
    }

    if (count) {
        char part[3][10];

        for (i = 0; i < count; i++) {

            if ((int)ent->angles2[i] < 0 ||
                (int)ent->angles2[i] > NUM_HIT_LOCATIONS)
                return;

            if (ent->weapon)
                strcpy(part[i], hit_info[(int)ent->angles2[i]].forename);
            else
                strcpy(part[i], hit_info[(int)ent->angles2[i]].backname);
        }

        if (target == cg.snap->ps.clientNum) {
            if (count == 3)
                CG_Printf("^1Your %s, your %s and your %s are hit.\n", part[0],
                          part[1], part[2]);
            else if (count == 2)
                CG_Printf("^1Your %s and your %s are hit.\n", part[0], part[1]);
            else
                CG_Printf("^1Your %s is hit.\n", part[0]);
        } else {
            if (count == 3)
                CG_Printf("^2You hit ^7%s^2's %s, %s and %s.\n", name, part[0],
                          part[1], part[2]);
            else if (count == 2)
                CG_Printf("^2You hit ^7%s^2's %s and %s.\n", name, part[0],
                          part[1]);
            else
                CG_Printf("^2You hit ^7%s^2's %s.\n", name, part[0]);
        }
    } else {

        if (ent->frame < 0 || ent->frame > NUM_HIT_LOCATIONS)
            return;

        if (target == cg.snap->ps.clientNum) {
            if (ent->weapon)
                CG_Printf("^1Your %s is hit.\n", hit_info[ent->frame].forename);
            else
                CG_Printf("^1Your %s is hit.\n", hit_info[ent->frame].backname);
        } else {
            if (ent->weapon)
                CG_Printf("^2You hit ^7%s^2's %s.\n", name,
                          hit_info[ent->frame].forename);
            else
                CG_Printf("^2You hit ^7%s^2's %s.\n", name,
                          hit_info[ent->frame].backname);
        }
    }
}

/*
=============
CG_Obituary
=============
*/
static void CG_Obituary(entityState_t *ent) {
    int mod;
    int target, attacker;
    char *message;
    char *message2;
    const char *targetInfo;
    const char *attackerInfo;
    char targetName[32];
    char attackerName[32];
    gender_t gender;
    clientInfo_t *ci;

    target = ent->otherEntityNum;
    attacker = ent->otherEntityNum2;
    mod = ent->eventParm;

    if (target < 0 || target >= MAX_CLIENTS) {
        CG_Error("CG_Obituary: target out of range");
    }
    ci = &cgs.clientinfo[target];

    if (attacker < 0 || attacker >= MAX_CLIENTS) {
        attacker = ENTITYNUM_WORLD;
        attackerInfo = NULL;
    } else {
        attackerInfo = CG_ConfigString(CS_PLAYERS + attacker);
    }

    targetInfo = CG_ConfigString(CS_PLAYERS + target);
    if (!targetInfo) {
        return;
    }
    Q_strncpyz(targetName, Info_ValueForKey(targetInfo, "n"),
               sizeof(targetName) - 2);
    strcat(targetName, S_COLOR_WHITE);

    message2 = "";

    // check for single client messages

    switch (mod) {
    case MOD_SUICIDE:
        message = "couldn't take it anymore";
        break;
    case MOD_FALLING:
        gender = ci->gender;
        if (gender == GENDER_FEMALE)
            message = "fell to her death";
        else if (gender == GENDER_NEUTER)
            message = "fell to its death";
        else
            message = "fell to his death";
        break;
    case MOD_WATER:
        message = "sleeps with the fish";
        break;
    case MOD_CRUSH:
    case MOD_SLIME:
    case MOD_LAVA:
    case MOD_TARGET_LASER:
    case MOD_TRIGGER_HURT:
        message = "had an accident";
        break;
    default:
        message = NULL;
        break;
    }

    if (attacker == target) {
        gender = ci->gender;
        switch (mod) {
        case MOD_DYNAMITE:
            if (gender == GENDER_FEMALE)
                message = "blew herself to smithereens";
            else if (gender == GENDER_NEUTER)
                message = "blew itself to smithereens";
            else
                message = "blew himself to smithereens";
            break;
        case MOD_MOLOTOV:
            if (gender == GENDER_FEMALE)
                message = "played with matches";
            else if (gender == GENDER_NEUTER)
                message = "played with matches";
            else
                message = "played with matches";
            break;
        default:
            if (gender == GENDER_FEMALE)
                message = "killed herself";
            else if (gender == GENDER_NEUTER)
                message = "killed itself";
            else
                message = "killed himself";
            break;
        }
    }

    if (message) {
        CG_Printf("%s %s.\n", targetName, message);
        return;
    }


    // check for double client messages
    if (!attackerInfo) {
        attacker = ENTITYNUM_WORLD;
        strcpy(attackerName, "noname");
    } else {
        Q_strncpyz(attackerName, Info_ValueForKey(attackerInfo, "n"),
                   sizeof(attackerName) - 2);
        strcat(attackerName, S_COLOR_WHITE);
        // check for kill messages about the current clientNum
        if (target == cg.snap->ps.clientNum) {
            Q_strncpyz(cg.killerName, attackerName, sizeof(cg.killerName));
        }
    }

    if (mod == MOD_TELEFRAG) {
        message = "tried to invade";
        message2 = "'s personal space";
        CG_Printf("%s %s %s%s.\n", attackerName, message, targetName, message2);
        return;
    }

    if (attacker != ENTITYNUM_WORLD) {
        int i, weapon = 0;
        // drag the old fields
        for (i = MAX_DEATH_MESSAGES - 1; i; i--) {
            if (cg.messagetime[i - 1]) {
                strcpy(cg.attacker[i], cg.attacker[i - 1]);
                strcpy(cg.target[i], cg.target[i - 1]);
                cg.messagetime[i] = cg.messagetime[i - 1];
                cg.mod[i] = cg.mod[i - 1];
            }
        }
        // the new field
        strcpy(cg.attacker[0], attackerName);
        strcpy(cg.target[0], targetName);
        cg.messagetime[0] = cg.time;
        cg.mod[0] = mod;

        if (mod > 0 && mod < WP_NUM_WEAPONS)
            weapon = mod;

        if (weapon) {
            if (ent->eFlags & EF_SAME_TEAM)
                CG_Printf("%s ^1teamkilled^7 %s with %s.\n", attackerName,
                          targetName, bg_weaponlist[weapon].name);
            else if (cg_killmsg.integer == 1)
                // TheDoctor: optionally disable kill messages in chat window.
                // They're still shown in the top right corner.
                CG_Printf("%s killed %s with %s.\n", attackerName, targetName,
                          bg_weaponlist[weapon].name);
            return;
        }
    }

    // we don't know what has happened
    CG_Printf("%s bit the dust\n", targetName);
}

//==========================================================================

/*
===============
CG_UseItem
===============
*/
static void CG_UseItem(centity_t *cent) {
}

/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
static void CG_ItemPickup(int itemNum) {
    cg.itemPickup = itemNum;
    cg.itemPickupTime = cg.time;
    cg.itemPickupBlendTime = cg.time;
    // see if it should be the grabbed weapon
}

/*
================
CG_WaterLevel

Returns waterlevel for entity origin
================
*/
int CG_WaterLevel(centity_t *cent) {
    vec3_t point;
    int contents, sample1, sample2, anim, waterlevel;

    // get waterlevel, accounting for ducking
    waterlevel = 0;
    VectorCopy(cent->lerpOrigin, point);
    point[2] += MINS_Z + 1;
    anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;

    if (anim == LEGS_CROUCH_WALK || anim == LEGS_CROUCH_BACK ||
        anim == LEGS_CROUCHED_IDLE) {
        point[2] += CROUCH_VIEWHEIGHT;
    } else {
        point[2] += DEFAULT_VIEWHEIGHT;
    }

    contents = CG_PointContents(point, -1);

    if (contents & MASK_WATER) {
        sample2 = point[2] - MINS_Z;
        sample1 = sample2 / 2;
        waterlevel = 1;
        point[2] = cent->lerpOrigin[2] + MINS_Z + sample1;
        contents = CG_PointContents(point, -1);

        if (contents & MASK_WATER) {
            waterlevel = 2;
            point[2] = cent->lerpOrigin[2] + MINS_Z + sample2;
            contents = CG_PointContents(point, -1);

            if (contents & MASK_WATER) {
                waterlevel = 3;
            }
        }
    }

    return waterlevel;
}

/*
================
CG_PainEvent

Also called by playerstate transition
================
*/
void CG_PainEvent(centity_t *cent, int health) {
    char *snd;

    // don't do more than two pain sounds a second
    if (cg.time - cent->pe.painTime < 500) {
        return;
    }

    if (health < 25) {
        snd = "*pain25_1.wav";
    } else if (health < 50) {
        snd = "*pain50_1.wav";
    } else if (health < 75) {
        snd = "*pain75_1.wav";
    } else {
        snd = "*pain100_1.wav";
    }
    // play a gurp sound instead of a normal pain sound
    if (CG_WaterLevel(cent) >= 1) {
        if (rand() & 1) {
            trap_S_StartSound(NULL, cent->currentState.number, CHAN_VOICE,
                              CG_CustomSound(cent->currentState.number,
                                             "sound/player/gurp1.wav"));
        } else {
            trap_S_StartSound(NULL, cent->currentState.number, CHAN_VOICE,
                              CG_CustomSound(cent->currentState.number,
                                             "sound/player/gurp2.wav"));
        }
    } else {
        trap_S_StartSound(NULL, cent->currentState.number, CHAN_VOICE,
                          CG_CustomSound(cent->currentState.number, snd));
    }
    // save pain time for programitic twitch animation
    cent->pe.painTime = cg.time;
    cent->pe.painDirection ^= 1;
}

/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
#define DEBUGNAME(x)                                                           \
    if (cg_debugEvents.integer) {                                              \
        CG_Printf(x "\n");                                                     \
    }
void CG_EntityEvent(centity_t *cent, vec3_t position) {
    entityState_t *es;
    int event;
    vec3_t dir;
    const char *s;
    int clientNum;
    clientInfo_t *ci;
    vec3_t dirs[ALC_COUNT];
    int weapon = 0;
    int anim = 0;

    es = &cent->currentState;
    event = es->event & ~EV_EVENT_BITS;

    if (cg_debugEvents.integer) {
        CG_Printf("ent:%3i  event:%3i ", es->number, event);
    }

    if (!event) {
        DEBUGNAME("ZEROEVENT");
        return;
    }

    clientNum = es->clientNum;
    if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
        clientNum = 0;
    }
    ci = &cgs.clientinfo[clientNum];

    switch (event) {
    //
    // movement generated events
    //
    case EV_FOOTSTEP:
        DEBUGNAME("EV_FOOTSTEP");
        if (cg_footsteps.integer) {
            trap_S_StartSound(NULL, es->number, CHAN_BODY,
                              cgs.media.footsteps[ci->footsteps][rand() & 3]);
        }
        break;
    case EV_FOOTSTEP_SNOW:
        DEBUGNAME("EV_FOOTSTEP_SNOW");
        if (cg_footsteps.integer) {
            trap_S_StartSound(NULL, es->number, CHAN_BODY,
                              cgs.media.footsteps[FOOTSTEP_SNOW][rand() & 3]);
        }
        break;
    case EV_FOOTSTEP_SAND:
        DEBUGNAME("EV_FOOTSTEP_SAND");
        if (cg_footsteps.integer) {
            trap_S_StartSound(NULL, es->number, CHAN_BODY,
                              cgs.media.footsteps[FOOTSTEP_SAND][rand() & 3]);
        }
        break;
    case EV_FOOTSTEP_CLOTH:
        DEBUGNAME("EV_FOOTSTEP_CLOTH");
        if (cg_footsteps.integer) {
            trap_S_StartSound(NULL, es->number, CHAN_BODY,
                              cgs.media.footsteps[FOOTSTEP_CLOTH][rand() & 3]);
        }
        break;
    case EV_FOOTSTEP_GRASS:
        DEBUGNAME("EV_FOOTSTEP_GRASS");
        if (cg_footsteps.integer) {
            trap_S_StartSound(NULL, es->number, CHAN_BODY,
                              cgs.media.footsteps[FOOTSTEP_GRASS][rand() & 3]);
        }
        break;
    case EV_FOOTSTEP_METAL:
        DEBUGNAME("EV_FOOTSTEP_METAL");
        if (cg_footsteps.integer) {
            trap_S_StartSound(NULL, es->number, CHAN_BODY,
                              cgs.media.footsteps[FOOTSTEP_METAL][rand() & 3]);
        }
        break;
    case EV_FOOTSPLASH:
        DEBUGNAME("EV_FOOTSPLASH");
        if (cg_footsteps.integer) {
            trap_S_StartSound(NULL, es->number, CHAN_BODY,
                              cgs.media.footsteps[FOOTSTEP_SPLASH][rand() & 3]);
        }
        break;
    case EV_FOOTWADE:
        DEBUGNAME("EV_FOOTWADE");
        if (cg_footsteps.integer) {
            trap_S_StartSound(NULL, es->number, CHAN_BODY,
                              cgs.media.footsteps[FOOTSTEP_SPLASH][rand() & 3]);
        }
        break;
    case EV_SWIM:
        DEBUGNAME("EV_SWIM");
        if (cg_footsteps.integer) {
            trap_S_StartSound(NULL, es->number, CHAN_BODY,
                              cgs.media.footsteps[FOOTSTEP_SPLASH][rand() & 3]);
        }
        break;

    case EV_FALL_VERY_SHORT:
        DEBUGNAME("EV_FALL_VERY_SHORT");
        if (cg_footsteps.integer) {
            trap_S_StartSound(NULL, es->number, CHAN_BODY,
                              cgs.media.footsteps[ci->footsteps][rand() & 3]);
        }
        if (clientNum == cg.predictedPlayerState.clientNum) {
            // smooth landing z changes
            cg.landChange = -4;
            cg.landTime = cg.time;
            cg.landAngleChange = 0;
        }
        break;

    case EV_FALL_SHORT:
        DEBUGNAME("EV_FALL_SHORT");
        trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.landSound);
        if (clientNum == cg.predictedPlayerState.clientNum) {
            // smooth landing z changes
            cg.landChange = -4;
            cg.landTime = cg.time;
            cg.landAngleChange = 0;
        }
        break;
    case EV_FALL_MEDIUM:
        DEBUGNAME("EV_FALL_MEDIUM");
        // use normal pain sound
        trap_S_StartSound(NULL, es->number, CHAN_VOICE,
                          CG_CustomSound(es->number, "*pain100_1.wav"));
        if (clientNum == cg.predictedPlayerState.clientNum) {
            // smooth landing z changes
            cg.landChange = -16;
            cg.landTime = cg.time;
            switch (rand() % 2) {
            case 0:
                cg.landAngleChange = 10;
                break;
            case 1:
            default:
                cg.landAngleChange = -10;
                break;
            }
        }
        // launch blood particle
        VectorSet(dir, 0, 0, -1);
        CG_LaunchImpactParticle(es->pos.trBase, dir, -1, -1, -1, qtrue);
        break;
    case EV_FALL_FAR:
        DEBUGNAME("EV_FALL_FAR");
        // use normal pain sound
        trap_S_StartSound(NULL, es->number, CHAN_VOICE,
                          CG_CustomSound(es->number, "*pain100_1.wav"));
        if (clientNum == cg.predictedPlayerState.clientNum) {
            // smooth landing z changes
            cg.landChange = -20;
            cg.landTime = cg.time;

            switch (rand() % 2) {
            case 0:
                cg.landAngleChange = 20;
                break;
            case 1:
            default:
                cg.landAngleChange = -20;
                break;
            }
        }
        // launch blood particle
        VectorSet(dir, 0, 0, -1);
        CG_LaunchImpactParticle(es->pos.trBase, dir, -1, -1, -1, qtrue);
        break;
    case EV_FALL_VERY_FAR:
        DEBUGNAME("EV_FALL_VERY_FAR");
        trap_S_StartSound(NULL, es->number, CHAN_AUTO,
                          CG_CustomSound(es->number, "*fall1.wav"));
        cent->pe.painTime = cg.time; // don't play a pain sound right after this
        if (clientNum == cg.predictedPlayerState.clientNum) {
            // smooth landing z changes
            cg.landChange = -24;
            cg.landTime = cg.time;
            switch (rand() % 2) {
            case 0:
                cg.landAngleChange = 30;
                break;
            case 1:
            default:
                cg.landAngleChange = -30;
                break;
            }
        }
        // launch blood particle
        VectorSet(dir, 0, 0, -1);
        CG_LaunchImpactParticle(es->pos.trBase, dir, -1, -1, -1, qtrue);
        break;

    case EV_STEP_4:
    case EV_STEP_8:
    case EV_STEP_12:
    case EV_STEP_16: // smooth out step up transitions
        DEBUGNAME("EV_STEP");
        {
            float oldStep;
            int delta;
            int step;

            if (clientNum != cg.predictedPlayerState.clientNum) {
                break;
            }
            // if we are interpolating, we don't need to smooth steps
            if (cg.demoPlayback ||
                cg_nopredict.integer || cg_synchronousClients.integer) {
                break;
            }
            // check for stepping up before a previous step is completed
            delta = cg.time - cg.stepTime;
            if (delta < STEP_TIME) {
                oldStep = cg.stepChange * (STEP_TIME - delta) / STEP_TIME;
            } else {
                oldStep = 0;
            }

            // add this amount
            step = 4 * (event - EV_STEP_4 + 1);
            cg.stepChange = oldStep + step;
            if (cg.stepChange > MAX_STEP_CHANGE) {
                cg.stepChange = MAX_STEP_CHANGE;
            }
            cg.stepTime = cg.time;
            break;
        }

    case EV_JUMP_PAD:
        DEBUGNAME("EV_JUMP_PAD");
//		CG_Printf( "EV_JUMP_PAD w/effect #%i\n", es->eventParm );
        break;

    case EV_JUMP:
        DEBUGNAME("EV_JUMP");
        trap_S_StartSound(NULL, es->number, CHAN_VOICE,
                          CG_CustomSound(es->number, "*jump1.wav"));
        break;
    case EV_TAUNT:
        DEBUGNAME("EV_TAUNT");
        if (bg_weaponlist[cent->currentState.weapon].wp_sort == WPS_PISTOL)
            trap_S_StartSound(NULL, es->number, CHAN_VOICE, cgs.media.taunt[1]);
        else
            trap_S_StartSound(NULL, es->number, CHAN_VOICE, cgs.media.taunt[0]);
        break;
    case EV_WATER_TOUCH:
        DEBUGNAME("EV_WATER_TOUCH");
        trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrInSound);
        break;
    case EV_WATER_LEAVE:
        DEBUGNAME("EV_WATER_LEAVE");
        trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound);
        break;
    case EV_WATER_UNDER:
        DEBUGNAME("EV_WATER_UNDER");
        trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound);
        break;
    case EV_WATER_CLEAR:
        DEBUGNAME("EV_WATER_CLEAR");
        trap_S_StartSound(NULL, es->number, CHAN_AUTO,
                          CG_CustomSound(es->number, "*gasp.wav"));
        break;

    case EV_MARKED_ITEM:
        DEBUGNAME("EV_MARKED_ITEM");
        cg.marked_item_index = es->eventParm;
        VectorCopy(es->origin, cg.marked_item_mins);
        VectorCopy(es->origin2, cg.marked_item_maxs);
        cg.marked_item_time = cg.time;
        break;

    case EV_MONEY_PICKUP:
        DEBUGNAME("EV_MONEY_PICKUP");
        {
            gitem_t *item;

            int quantity;
            int index = 1;
            quantity = es->eventParm; // player predicted

            item = bg_itemlist + 1;
            for (; item->classname; item++) {
                if (!Q_stricmp(item->classname, "pickup_money"))
                    break;
                index++;
            }

            trap_S_StartSound(NULL, es->number, CHAN_AUTO,
                              trap_S_RegisterSound(item->pickup_sound, qfalse));

            // show icon and name on status bar
            if (es->number == cg.snap->ps.clientNum) {
                cg.itemPickupQuant = quantity;
                cg.itemPickup = index;
                cg.itemPickupTime = cg.time;
                cg.itemPickupBlendTime = cg.time;
            }
        }
        break;

    case EV_ITEM_PICKUP:
        DEBUGNAME("EV_ITEM_PICKUP");
        {
            gitem_t *item;
            int index;

            index = es->eventParm; // player predicted

            if (index < 1 || index >= bg_numItems) {
                break;
            }
            item = &bg_itemlist[index];

            // powerups and team items will have a separate global sound, this
            // one will be played at prediction time
            trap_S_StartSound(NULL, es->number, CHAN_AUTO,
                              trap_S_RegisterSound(item->pickup_sound, qfalse));

            // show icon and name on status bar
            if (es->number == cg.snap->ps.clientNum) {
                CG_ItemPickup(index);
            }
        }
        break;


    //
    // weapon events
    //
    case EV_NOAMMO:
        DEBUGNAME("EV_NOAMMO");
        //		trap_S_StartSound (NULL, es->number, CHAN_AUTO,
        //cgs.media.noAmmoSound );
        if (es->number == cg.snap->ps.clientNum) {
            CG_OutOfAmmoChange();
        }
        break;
    case EV_CHANGE_WEAPON:
        DEBUGNAME("EV_CHANGE_WEAPON");
        if (cg.introend < cg.time)
            trap_S_StartSound(NULL, es->number, CHAN_AUTO,
                              cgs.media.selectSound);
        break;

    case EV_CHANGETO_SOUND:
        DEBUGNAME("EV_CHANGETO_SOUND");
        if (cg.introend < cg.time) {
            if (bg_weaponlist[es->eventParm].wp_sort == WPS_PISTOL)
                trap_S_StartSound(NULL, es->number, CHAN_AUTO,
                                  cgs.media.snd_pistol_raise);
            else {
                switch (es->eventParm) {
                case WP_WINCHESTER66:
                    trap_S_StartSound(NULL, es->number, CHAN_AUTO,
                                      cgs.media.snd_winch66_raise);
                    break;
                case WP_LIGHTNING:
                    trap_S_StartSound(NULL, es->number, CHAN_AUTO,
                                      cgs.media.snd_lightn_raise);
                    break;
                case WP_SHARPS:
                    trap_S_StartSound(NULL, es->number, CHAN_AUTO,
                                      cgs.media.snd_sharps_raise);
                    break;
                case WP_REMINGTON_GAUGE:
                    trap_S_StartSound(NULL, es->number, CHAN_AUTO,
                                      cgs.media.snd_shotgun_raise);
                    break;
                case WP_WINCH97:
                    trap_S_StartSound(NULL, es->number, CHAN_AUTO,
                                      cgs.media.snd_winch97_raise);
                    break;
                }
            }
        }
        break;

    case EV_CHANGE_TO_WEAPON:
        DEBUGNAME("EV_CHANGE_TO_WEAPON");
        if (cent->currentState.clientNum == cg.snap->ps.clientNum)
            cg.weaponSelect = es->eventParm;
        break;

    case EV_CHOOSE_WEAPON:
        /*DEBUGNAME("EV_CHOOSE_WEAPON");
        if(cent->currentState.clientNum == cg.snap->ps.clientNum){
                cg.weaponSelect = cg.markedweapon;
                cg.weaponSelectTime = cg.time;
                cg.markedweapon = 0;
        }*/
        // trap_SendConsoleCommand("wp_choose_off\n");
        break;

    case EV_RELOAD:
        DEBUGNAME("EV_RELOAD");

        // if(cg.snap->ps.clientNum == cent->currentState.clientNum)
        //	break;

        CG_PlayReloadSound(es->eventParm, cent, qfalse);
        break;

    case EV_RELOAD2:
        DEBUGNAME("EV_RELOAD2");

        CG_PlayReloadSound(es->eventParm, cent, qtrue);
        break;

    case EV_LIGHT_DYNAMITE:
        DEBUGNAME("EV_LIGHT_DYNAMIT");

        trap_S_StartSound(NULL, cent->currentState.number, CHAN_WEAPON,
                          cgs.media.dynamitezundan);
        break;

    case EV_ALT_FIRE_WEAPON:
        DEBUGNAME("EV_ALT_FIRE_WEAPON");

        anim = cent->currentState.eventParm;

        if (cent->currentState.clientNum == cg.snap->ps.clientNum &&
            cent->currentState.weapon != WP_KNIFE) {
            cg.shootAngleChange = 2;
            cg.shootTime = cg.time;
            cg.shootChange = 0;

            // cent->pe.weapon.weaponAnim = ( ( cent->pe.weapon.weaponAnim &
            // ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) 		| anim;
        }

        // cg.firingtime = cg.time + BG_AnimLength(anim, cg.snap->ps.weapon);
        CG_FireWeapon(cent, qtrue, cent->currentState.weapon);
        break;

    case EV_FIRE_WEAPON2:
    case EV_FIRE_WEAPON:
        DEBUGNAME("EV_FIRE_WEAPON");
        if (cg.markedweapon)
            return;

        anim = cent->currentState.eventParm;

        if (event == EV_FIRE_WEAPON2)
            weapon = cent->currentState.frame;
        else
            weapon = cent->currentState.weapon;

        if (cent->currentState.clientNum == cg.snap->ps.clientNum) {

            if (bg_weaponlist[cent->currentState.weapon].wp_sort ==
                WPS_PISTOL) {
                cg.shootAngleChange = 5;
                cg.shootTime = cg.time;
                cg.shootChange = 0;
            } else if (cent->currentState.weapon == WP_SHARPS) {
                cg.shootAngleChange = 6;
                cg.shootTime = cg.time;
                cg.shootChange = 0;
            } else if (cent->currentState.weapon != WP_KNIFE &&
                       cent->currentState.weapon < WP_GATLING) {
                cg.shootAngleChange = 4;
                cg.shootTime = cg.time;
                cg.shootChange = 0;
            }
        }

        CG_FireWeapon(cent, (event == EV_FIRE_WEAPON2), weapon);
        break;

    // 2nd Pistol
    case EV_FIRE_WEAPON_DELAY:
        DEBUGNAME("EV_FIRE_WEAPON_DELAY");
        break;

    case EV_ALT_WEAPON_MODE:
        break;

    case EV_GATLING_NOTPLANAR:
        if (cent->currentState.clientNum == cg.snap->ps.clientNum)
            CG_Printf("The ground is not flat enough.\n");
        break;

    case EV_GATLING_NOSPACE:
        if (cent->currentState.clientNum == cg.snap->ps.clientNum)
            CG_Printf("Not enough room for the gatling here.\n");
        break;

    case EV_GATLING_TOODEEP:
        if (cent->currentState.clientNum == cg.snap->ps.clientNum)
            CG_Printf("The water is too deep.\n");
        break;

    case EV_BUILD_TURRET:
        trap_S_StartSound(NULL, es->number, CHAN_WEAPON,
                          cgs.media.gatling_build);
        break;

    case EV_DISM_TURRET:
        trap_S_StartSound(NULL, es->number, CHAN_WEAPON,
                          cgs.media.gatling_dism);
        break;

    case EV_SCOPE_PUT:
        trap_S_StartSound(NULL, es->number, CHAN_WEAPON,
                          cgs.media.scopePutSound);
        break;

    case EV_UNDERWATER:
        DEBUGNAME("EV_UNDERWATER");
        if (cent->currentState.clientNum == cg.snap->ps.clientNum)
            trap_S_AddLoopingSound(es->number, cent->lerpOrigin, vec3_origin,
                                   cgs.media.underwater);
        break;

    // Spoon Sounds, ok perhaps not mine ;) hoho that's so funny! asshole!
    case EV_DBURN:
        DEBUGNAME("EV_DBURN");
        trap_S_AddLoopingSound(es->number, cent->lerpOrigin, vec3_origin,
                               cgs.media.dynamiteburn);
        break;

    case EV_NOAMMO_CLIP:
        DEBUGNAME("EV_WEAPONBLOCK");
        trap_S_StartSound(NULL, es->number, CHAN_WEAPON, cgs.media.noAmmoSound);
        break;

    case EV_USE_ITEM0:
        DEBUGNAME("EV_USE_ITEM0");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM1:
        DEBUGNAME("EV_USE_ITEM1");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM2:
        DEBUGNAME("EV_USE_ITEM2");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM3:
        DEBUGNAME("EV_USE_ITEM3");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM4:
        DEBUGNAME("EV_USE_ITEM4");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM5:
        DEBUGNAME("EV_USE_ITEM5");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM6:
        DEBUGNAME("EV_USE_ITEM6");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM7:
        DEBUGNAME("EV_USE_ITEM7");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM8:
        DEBUGNAME("EV_USE_ITEM8");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM9:
        DEBUGNAME("EV_USE_ITEM9");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM10:
        DEBUGNAME("EV_USE_ITEM10");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM11:
        DEBUGNAME("EV_USE_ITEM11");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM12:
        DEBUGNAME("EV_USE_ITEM12");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM13:
        DEBUGNAME("EV_USE_ITEM13");
        CG_UseItem(cent);
        break;
    case EV_USE_ITEM14:
        DEBUGNAME("EV_USE_ITEM14");
        CG_UseItem(cent);
        break;

    //=================================================================

    //
    // other events
    //
    case EV_PLAYER_TELEPORT_IN:
        DEBUGNAME("EV_PLAYER_TELEPORT_IN");
        break;

    case EV_PLAYER_TELEPORT_OUT:
        DEBUGNAME("EV_PLAYER_TELEPORT_OUT");
        break;

    case EV_ITEM_POP:
        DEBUGNAME("EV_ITEM_POP");
        break;
    case EV_ITEM_RESPAWN:
        DEBUGNAME("EV_ITEM_RESPAWN");
        cent->miscTime = cg.time; // scale up from this
        break;

    case EV_GRENADE_BOUNCE:
        DEBUGNAME("EV_GRENADE_BOUNCE");
        break;


    //
    // missile impacts
    //
    case EV_WHISKEY_BURNS:
        ByteToDir(es->eventParm, dir);
        CG_CreateFire(position, dir);
        break;

    case EV_MISSILE_FIRE:
        ByteToDir(es->eventParm, dir);
        BG_EntityStateToDirs(es, dirs);
        CG_MissileHitWall(es->weapon, 0, position, dir, IMPACTSOUND_DEFAULT, -1,
                          -1, qtrue, dirs, -1);
        break;

    // not used anymore
    case EV_MISSILE_ALCOHOL:
        // DEBUGNAME("EV_MISSILE_ALCOHOL");
        // ByteToDir( es->eventParm, dir);
        // CG_MissileHitWall( es->weapon, 0, position, dir, IMPACTSOUND_DEFAULT,
        // -1, -1, qtrue, NULL);
        break;

    case EV_MISSILE_HIT:
        DEBUGNAME("EV_MISSILE_HIT");
        ByteToDir(es->eventParm, dir);
        CG_MissileHitPlayer(es->weapon, position, dir, es->otherEntityNum);
        break;

    case EV_MISSILE_MISS:
        DEBUGNAME("EV_MISSILE_MISS");
        ByteToDir(es->eventParm, dir);
        BG_EntityStateToDirs(es, dirs);
        CG_MissileHitWall(es->weapon, 0, position, dir, IMPACTSOUND_DEFAULT, -1,
                          -1, qfalse, dirs, -1);
        break;

    case EV_MISSILE_MISS_METAL:
        DEBUGNAME("EV_MISSILE_MISS_METAL");
        ByteToDir(es->eventParm, dir);
        CG_MissileHitWall(es->weapon, 0, position, dir, IMPACTSOUND_METAL, -1,
                          -1, qfalse, NULL, -1);
        break;


    // func events
    case EV_FUNCBREAKABLE:
        CG_LaunchFuncBreakable(cent);
        break;

    case EV_BULLET:
        break;

    case EV_BULLET_HIT_BOILER:
        DEBUGNAME("EV_BULLET_HIT_BOILER");
        ByteToDir(es->eventParm, dir);
        trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.boilerhit);
        trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.boilerhit);
        trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.boilerhit);
        CG_Bullet(es->pos.trBase, es->otherEntityNum, dir, qfalse,
                  ENTITYNUM_WORLD, es->time, SURF_METAL, -1, -1);
        // CG_Printf("Your Boiler Plate was hit (debugging)\n");
        if (es->eventParm == cg.snap->ps.clientNum) {
            //"shake" the view
            cg.landAngleChange = (rand() % 40) - 20;
            cg.landTime = cg.time;
            cg.landChange = 0;
        }
        break;

    case EV_KNIFEHIT:
        DEBUGNAME("EV_KNIFEHIT");
        trap_S_StartSound(es->pos.trBase, es->number, CHAN_WEAPON,
                          cgs.media.knifehit);

        // wall was hit
        if (es->torsoAnim != -1) {
            CG_LaunchImpactParticle(es->pos.trBase, dir, es->time2,
                                    es->torsoAnim, WP_KNIFE, qfalse);
            break;
        }

        if (es->weapon < MAX_CLIENTS && es->weapon >= 0) {
            CG_LaunchImpactParticle(es->pos.trBase, dir, -1, -1, -1, qtrue);
        }

        if (es->eventParm == cg.snap->ps.clientNum) {
            //"shake" the view
            cg.landAngleChange = (rand() % 30) - 15;
            cg.landTime = cg.time;
            cg.landChange = 0;
        }
        break;

    case EV_BOILER_HIT:
        DEBUGNAME("EV_BOILER_HIT"); // only sound
        trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.boilerhit);
        if (es->eventParm == cg.snap->ps.clientNum) {
            //"shake" the view
            cg.landAngleChange = (rand() % 40) - 20;
            cg.landTime = cg.time;
            cg.landChange = 0;
        }
        break;

    case EV_BULLET_HIT_WALL:
        DEBUGNAME("EV_BULLET_HIT_WALL");
        ByteToDir(es->eventParm, dir);
        CG_Bullet(es->pos.trBase, es->otherEntityNum, dir, qfalse,
                  ENTITYNUM_WORLD, es->time, es->time2, es->torsoAnim,
                  es->otherEntityNum2);
        break;

    case EV_BULLET_HIT_FLESH:
        DEBUGNAME("EV_BULLET_HIT_FLESH");
        ByteToDir(es->eventParm, dir);
        CG_Bullet(es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm,
                  es->time, -1, -1, -1);
        if (es->eventParm == cg.snap->ps.clientNum) {
            //"shake" the view
            cg.landAngleChange = (rand() % 30) - 15;
            cg.landTime = cg.time;
            cg.landChange = 0;
        }
        break;

    case EV_SHOTGUN:
        DEBUGNAME("EV_SHOTGUN");
        CG_ShotgunFire(es);
        break;

    case EV_SMOKE:
        DEBUGNAME("EV_SMOKE");
        if (es->frame != cg.snap->ps.clientNum || cg.renderingThirdPerson ||
            !cg_gunsmoke.integer)
            CG_MakeSmokePuff(es);
        break;

    case EV_GENERAL_SOUND:
        DEBUGNAME("EV_GENERAL_SOUND");
        if (cgs.gameSounds[es->eventParm]) {
            trap_S_StartSound(NULL, es->number, CHAN_VOICE,
                              cgs.gameSounds[es->eventParm]);
        } else {
            s = CG_ConfigString(CS_SOUNDS + es->eventParm);
            trap_S_StartSound(NULL, es->number, CHAN_VOICE,
                              CG_CustomSound(es->number, s));
        }
        break;

    case EV_ROUND_START:
        DEBUGNAME("EV_ROUND_START");
        CG_DeleteRoundEntities();
        break;

    case EV_ROUND_TIME:
        DEBUGNAME("EV_ROUND_TIME");

        // if(es->time2)
        cg.roundendtime = es->time2;
        // if(es->time)
        cg.roundstarttime = es->time;
        break;

    case EV_HIT_CHECK:
        /*if(ai_nodepointer >= MAX_AINODES)
                break;

        VectorCopy(es->pos.trBase, ai_nodes[ai_nodepointer]);
        VectorCopy(es->angles, ai_angles[ai_nodepointer]);
        ai_nodepointer++;*/
        break;

    case EV_DUEL_WON:

        trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_ANNOUNCER,
                          cgs.media.du_won_snd);
        break;

    case EV_DUEL_INTRO:
        DEBUGNAME("EV_DUEL_INTRO");

        // look if this info is for the current cgame
        /*if((int)es->angles[0] == cg.snap->ps.clientNum ||
                (int)es->angles[1] == cg.snap->ps.clientNum ||
                (int)es->origin[0] == cg.snap->ps.clientNum ||
                (int)es->origin[1] == cg.snap->ps.clientNum ){

                cg.currentmappart = es->eventParm;
        }*/

        cg.roundstarttime = es->time;
        cg.round = es->time2;
        break;

    case EV_GLOBAL_SOUND: // play from the player's head so it never diminishes
        DEBUGNAME("EV_GLOBAL_SOUND");
        if (cgs.gameSounds[es->eventParm]) {
            trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO,
                              cgs.gameSounds[es->eventParm]);
        } else {
            s = CG_ConfigString(CS_SOUNDS + es->eventParm);
            trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO,
                              CG_CustomSound(es->number, s));
        }
        break;

    case EV_GLOBAL_TEAM_SOUND: // play from the player's head so it never
                               // diminishes
    {
        break;
    }

    case EV_PAIN:
        // local player sounds are triggered in CG_CheckLocalSounds,
        // so ignore events on the player
        DEBUGNAME("EV_PAIN");
        if (cent->currentState.number != cg.snap->ps.clientNum) {
            CG_PainEvent(cent, es->eventParm);
        }
        break;

    case EV_DEATH_DEFAULT:
    case EV_DEATH_HEAD:
    case EV_DEATH_ARM_L:
    case EV_DEATH_ARM_R:
    case EV_DEATH_CHEST:
    case EV_DEATH_STOMACH:
    case EV_DEATH_LEG_L:
    case EV_DEATH_LEG_R:
    case EV_DEATH_FALL:
    case EV_DEATH_FALL_BACK:
        DEBUGNAME("EV_DEATHx");

        if (CG_WaterLevel(cent) >= 1) {
            trap_S_StartSound(NULL, es->number, CHAN_VOICE,
                              CG_CustomSound(es->number, "*drown.wav"));
        } else {
            trap_S_StartSound(NULL, es->number, CHAN_VOICE,
                              cgs.media.snd_death[event - EV_DEATH_DEFAULT]);
        }

        break;

    case EV_HIT_MESSAGE:
        DEBUGNAME("EV_HIT_MESSAGE");
        CG_Hit_Message(es);
        break;

    case EV_OBITUARY:
        DEBUGNAME("EV_OBITUARY");
        CG_Obituary(es);
        break;

    //
    // powerup events
    //
    case EV_POWERUP_QUAD:
        DEBUGNAME("EV_POWERUP_QUAD");
        break;
    case EV_POWERUP_BATTLESUIT:
        DEBUGNAME("EV_POWERUP_BATTLESUIT");
        break;
    case EV_POWERUP_REGEN:
        DEBUGNAME("EV_POWERUP_REGEN");
        break;

    case EV_GIB_PLAYER:
        DEBUGNAME("EV_GIB_PLAYER");
        // don't play gib sound when using the kamikaze because it interferes
        // with the kamikaze sound, downside is that the gib sound will also
        // not be played when someone is gibbed while just carrying the kamikaze
        CG_GibPlayer(cent->lerpOrigin);
        break;

    case EV_STOPLOOPINGSOUND:
        DEBUGNAME("EV_STOPLOOPINGSOUND");
        trap_S_StopLoopingSound(es->number);
        es->loopSound = 0;
        break;

    case EV_DEBUG_LINE:
        DEBUGNAME("EV_DEBUG_LINE");
        CG_Beam(cent);
        break;

    case EV_DEBUG_BULLET:
        DEBUGNAME("EV_DEBUG_BULLET");
        CG_BulletTracer(es->pos.trBase, es->origin2, es->eventParm,
                        es->otherEntityNum);
        break;

    case EV_HIT_FAR:
        DEBUGNAME("EV_HIT_FAR");
        CG_Hit_Far_Message(es->angles2[0], es->otherEntityNum);
        break;

    case EV_NOTHING:
        DEBUGNAME("EV_NOTHING");
        CG_Printf("who called that event 77?\n");
        break;

    default:
        DEBUGNAME("UNKNOWN");
        CG_Error("Unknown event: %i", event);
        break;
    }
}

/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents(centity_t *cent) {
    // check for event-only entities
    if (cent->currentState.eType > ET_EVENTS) {
        if (cent->previousEvent) {
            return; // already fired
        }

        cent->previousEvent = 1;

        cent->currentState.event = cent->currentState.eType - ET_EVENTS;
    } else {
        // check for events riding with another entity
        if (cent->currentState.event == cent->previousEvent) {
            return;
        }
        cent->previousEvent = cent->currentState.event;
        if ((cent->currentState.event & ~EV_EVENT_BITS) == 0) {
            return;
        }
    }

    // calculate the position at exactly the frame time
    BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime,
                          cent->lerpOrigin);
    CG_SetEntitySoundPosition(cent);

    CG_EntityEvent(cent, cent->lerpOrigin);
}
