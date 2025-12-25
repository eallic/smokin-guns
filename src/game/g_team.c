/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2003 Iron Claw Interactive
Copyright (C) 2005-2010 Smokin' Guns

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

#include "g_local.h"

typedef struct teamgame_s {
    float last_flag_capture;
    int last_capture_team;
    flagStatus_t redStatus;  // CTF
    flagStatus_t blueStatus; // CTF
    flagStatus_t flagStatus; // One Flag CTF
    int redTakenTime;
    int blueTakenTime;
    int redObeliskAttackedTime;
    int blueObeliskAttackedTime;
} teamgame_t;

teamgame_t teamgame;

gentity_t *neutralObelisk;

void Team_SetFlagStatus(int team, flagStatus_t status);

void Team_InitGame(void) {
    memset(&teamgame, 0, sizeof teamgame);

    switch (g_gametype.integer) {
    default:
        break;
    }
}

int OtherTeam(int team) {
    if (team == TEAM_RED)
        return TEAM_BLUE;
    else if (team == TEAM_BLUE)
        return TEAM_RED;
    return team;
}

const char *TeamName(int team) {
    if (team == TEAM_RED)
        return g_redteam.string;
    else if (team == TEAM_BLUE)
        return g_blueteam.string;
    else if (team >= TEAM_SPECTATOR)
        return "SPECTATOR";
    return "FREE";
}

const char *TeamColorString(int team) {
    if (team == TEAM_RED)
        return S_COLOR_RED;
    else if (team == TEAM_BLUE)
        return S_COLOR_BLUE;
    else if (team == TEAM_SPECTATOR)
        return S_COLOR_YELLOW;
    return S_COLOR_WHITE;
}

// NULL for everyone
static __attribute__((format(printf, 2, 3))) void QDECL
PrintMsg(gentity_t *ent, const char *fmt, ...) {
    char msg[1024];
    va_list argptr;
    char *p;

    va_start(argptr, fmt);
    if (Q_vsnprintf(msg, sizeof(msg), fmt, argptr) >= sizeof(msg)) {
        G_Error("PrintMsg overrun");
    }
    va_end(argptr);

    // double quotes are bad
    while ((p = strchr(msg, '"')) != NULL)
        *p = '\'';

    trap_SendServerCommand(((ent == NULL) ? -1 : ent - g_entities),
                           va("print \"%s\"", msg));
}

/*
==============
AddTeamScore

 used for gametype > GT_TEAM
 for gametype GT_TEAM the level.teamScores is updated in AddScore in g_combat.c
==============
*/
void AddTeamScore(vec3_t origin, int team, int score) {
    gentity_t *te;

    te = G_TempEntity(origin, EV_GLOBAL_TEAM_SOUND);
    te->r.svFlags |= SVF_BROADCAST;

    if (team == TEAM_RED) {
        if (level.teamScores[TEAM_RED] + score == level.teamScores[TEAM_BLUE]) {
            // teams are tied sound
            te->s.eventParm = GTS_TEAMS_ARE_TIED;
        } else if (level.teamScores[TEAM_RED] <= level.teamScores[TEAM_BLUE] &&
                   level.teamScores[TEAM_RED] + score >
                       level.teamScores[TEAM_BLUE]) {
            // red took the lead sound
            te->s.eventParm = GTS_REDTEAM_TOOK_LEAD;
        } else {
            // red scored sound
            te->s.eventParm = GTS_REDTEAM_SCORED;
        }
    } else {
        if (level.teamScores[TEAM_BLUE] + score == level.teamScores[TEAM_RED]) {
            // teams are tied sound
            te->s.eventParm = GTS_TEAMS_ARE_TIED;
        } else if (level.teamScores[TEAM_BLUE] <= level.teamScores[TEAM_RED] &&
                   level.teamScores[TEAM_BLUE] + score >
                       level.teamScores[TEAM_RED]) {
            // blue took the lead sound
            te->s.eventParm = GTS_BLUETEAM_TOOK_LEAD;
        } else {
            // blue scored sound
            te->s.eventParm = GTS_BLUETEAM_SCORED;
        }
    }
    level.teamScores[team] += score;
}

/*
==============
OnSameTeam
==============
*/
qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2) {
    if (!ent1->client || !ent2->client) {
        return qfalse;
    }

    if (g_gametype.integer < GT_TEAM) {
        return qfalse;
    }

    if (ent1->client->sess.sessionTeam == ent2->client->sess.sessionTeam) {
        return qtrue;
    }

    return qfalse;
}



/*
================
Team_ForceGesture
================
*/
void Team_ForceGesture(int team) {
    int i;
    gentity_t *ent;

    for (i = 0; i < MAX_CLIENTS; i++) {
        ent = &g_entities[i];
        if (!ent->inuse)
            continue;
        if (!ent->client)
            continue;
        if (ent->client->sess.sessionTeam != team)
            continue;
        //
        ent->flags |= FL_FORCE_GESTURE;
    }
}

/*
================
Team_FragBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumulative.  You get one, they are in importance
order.
================
*/

/*
================
Team_CheckHurtCarrier

Check to see if attacker hurt the flag carrier.  Needed when handing out bonuses
for assistance to flag carrier defense.
================
*/







void Team_FreeEntity(gentity_t *ent) {
}

/*
==============
Team_DroppedFlagThink

Automatically set in Launch_Item if the item is one of the flags

Flags are unique in that if they are dropped, the base flag must be respawned
when they time out
==============
*/

/*
==============
Team_DroppedFlagThink
==============
*/



/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
gentity_t *Team_GetLocation(gentity_t *ent) {
    gentity_t *eloc, *best;
    float bestlen, len;
    vec3_t origin;

    best = NULL;
    bestlen = 3 * 8192.0 * 8192.0;

    VectorCopy(ent->r.currentOrigin, origin);

    for (eloc = level.locationHead; eloc; eloc = eloc->nextTrain) {
        len = (origin[0] - eloc->r.currentOrigin[0]) *
                  (origin[0] - eloc->r.currentOrigin[0]) +
              (origin[1] - eloc->r.currentOrigin[1]) *
                  (origin[1] - eloc->r.currentOrigin[1]) +
              (origin[2] - eloc->r.currentOrigin[2]) *
                  (origin[2] - eloc->r.currentOrigin[2]);

        if (len > bestlen) {
            continue;
        }

        if (!trap_InPVS(origin, eloc->r.currentOrigin)) {
            continue;
        }

        bestlen = len;
        best = eloc;
    }

    return best;
}

/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
qboolean Team_GetLocationMsg(gentity_t *ent, char *loc, int loclen) {
    gentity_t *best;

    best = Team_GetLocation(ent);

    if (!best)
        return qfalse;

    if (best->count) {
        if (best->count < 0)
            best->count = 0;
        if (best->count > 7)
            best->count = 7;
        Com_sprintf(loc, loclen, "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE,
                    best->count + '0', best->message);
    } else
        Com_sprintf(loc, loclen, "%s", best->message);

    return qtrue;
}

/*---------------------------------------------------------------------------*/

/*
================
SelectRandomTeamSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define MAX_TEAM_SPAWN_POINTS 32
gentity_t *SelectRandomTeamSpawnPoint(int teamstate, team_t team) {
    gentity_t *spot;
    int count;
    int selection;
    gentity_t *goodspots[MAX_TEAM_SPAWN_POINTS];
    gentity_t **spots = goodspots;
    // Tequila: In the case, we would telefrag, we would really select
    // a random spawnpoint for delayed respawn, so we need to know the badspots
    // list
    gentity_t *badspots[MAX_TEAM_SPAWN_POINTS];
    int badcount = 0;
    char *classname;

    if (g_gametype.integer == GT_BR) {

        if (g_robteam == team)
            classname = "team_CTF_blueplayer";
        else
            classname = "team_CTF_redplayer";

    } else if (teamstate == TEAM_BEGIN) {
        if (team == TEAM_RED)
            if (g_gametype.integer == GT_RTP) {
                switch (sg_redspawn) {
                case 0:
                    classname = "team_CTF_redplayer";
                    break;
                case 1:
                    classname = "team_CTF_blueplayer";
                    break;
                case 2:
                    classname = "team_CTF_redspawn";
                    break;
                case 3:
                default:
                    classname = "team_CTF_bluespawn";
                    break;
                }
            } else
                classname = "team_CTF_redplayer";
        else if (team == TEAM_BLUE)
            if (g_gametype.integer == GT_RTP) {

                switch (sg_bluespawn) {
                case 0:
                    classname = "team_CTF_redplayer";
                    break;
                case 1:
                    classname = "team_CTF_blueplayer";
                    break;
                case 2:
                    classname = "team_CTF_redspawn";
                    break;
                case 3:
                default:
                    classname = "team_CTF_bluespawn";
                    break;
                }
            } else
                classname = "team_CTF_blueplayer";
        else
            return NULL;
    } else {
        if (team == TEAM_RED)
            if (g_gametype.integer >= GT_RTP) {
                switch (sg_redspawn) {
                case 0:
                    classname = "team_CTF_redplayer";
                    break;
                case 1:
                    classname = "team_CTF_blueplayer";
                    break;
                case 2:
                    classname = "team_CTF_redspawn";
                    break;
                case 3:
                default:
                    classname = "team_CTF_bluespawn";
                    break;
                }
            } else
                classname = "team_CTF_redspawn";
        else if (team == TEAM_BLUE)
            if (g_gametype.integer >= GT_RTP) {
                switch (sg_bluespawn) {
                case 0:
                    classname = "team_CTF_redplayer";
                    break;
                case 1:
                    classname = "team_CTF_blueplayer";
                    break;
                case 2:
                    classname = "team_CTF_redspawn";
                    break;
                case 3:
                default:
                    classname = "team_CTF_bluespawn";
                    break;
                }
            } else
                classname = "team_CTF_bluespawn";
        else
            return NULL;
    }
    count = 0;

    spot = NULL;

    while ((spot = G_Find(spot, FOFS(classname), classname)) != NULL) {
        if (SpotWouldTelefrag(spot)) {
            badspots[badcount] = spot;
            if (++badcount == MAX_TEAM_SPAWN_POINTS)
                break;
            continue;
        }
        spots[count] = spot;
        if (++count == MAX_TEAM_SPAWN_POINTS)
            break;
    }

    if (!count) { // no spots that won't telefrag
        // Tequila: G_Find will always return the last spawnpoint, it's better
        // now to return a random bad spawnpoint
        spots = badspots;
        count = badcount;
    }

    selection = rand() % count;
    return spots[selection];
}

/*
===========
SelectCTFSpawnPoint

============
*/
gentity_t *SelectCTFSpawnPoint(team_t team, int teamstate, vec3_t origin,
                               vec3_t angles, qboolean isbot, int mappart) {
    gentity_t *spot;

    spot = SelectRandomTeamSpawnPoint(teamstate, team);

    if (!spot) {
        return SelectSpawnPoint(vec3_origin, origin, angles, isbot, mappart,
                                NULL);
    }

    VectorCopy(spot->s.origin, origin);
    VectorCopy(spot->s.angles, angles);

    return spot;
}

/*---------------------------------------------------------------------------*/

static int QDECL SortClients(const void *a, const void *b) {
    return *(int *)a - *(int *)b;
}

/*
==================
TeamplayLocationsMessage

Format:
        clientNum location health armor weapon powerups

==================
*/
void TeamplayInfoMessage(gentity_t *ent) {
    char entry[1024];
    char string[8192];
    int stringlength;
    int i, j;
    gentity_t *player;
    int cnt;
    int h, a;
    int clients[TEAM_MAXOVERLAY];
    int team;

    if (!ent->client->pers.teamInfo)
        return;

    // send team info to spectator for team of followed client
    if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
        if (ent->client->sess.spectatorState != SPECTATOR_FOLLOW ||
            ent->client->sess.spectatorClient < 0) {
            return;
        }
        team = g_entities[ent->client->sess.spectatorClient]
                   .client->sess.sessionTeam;
    } else {
        team = ent->client->sess.sessionTeam;
    }

    if (team != TEAM_RED && team != TEAM_BLUE) {
        return;
    }

    // figure out what client should be on the display
    // we are limited to 8, but we want to use the top eight players
    // but in client order (so they don't keep changing position on the overlay)
    for (i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY;
         i++) {
        player = g_entities + level.sortedClients[i];
        if (player->inuse && player->client->sess.sessionTeam == team) {
            clients[cnt++] = level.sortedClients[i];
        }
    }

    // We have the top eight players, sort them by clientNum
    qsort(clients, cnt, sizeof(clients[0]), SortClients);

    // send the latest information on all clients
    string[0] = 0;
    stringlength = 0;

    for (i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY;
         i++) {
        player = g_entities + i;
        if (player->inuse && player->client->sess.sessionTeam == team) {

            h = player->client->ps.stats[STAT_HEALTH];
            a = player->client->ps.stats[STAT_ARMOR];
            if (h < 0)
                h = 0;
            if (a < 0)
                a = 0;

            Com_sprintf(entry, sizeof(entry), " %i %i %i %i %i %i",
                        //				level.sortedClients[i],
                        //player->client->pers.teamState.location, h, a,
                        i, player->client->pers.teamState.location, h, a,
                        player->client->ps.weapon, player->s.powerups);
            j = strlen(entry);
            if (stringlength + j >= sizeof(string))
                break;
            strcpy(string + stringlength, entry);
            stringlength += j;
            cnt++;
        }
    }

    trap_SendServerCommand(ent - g_entities, va("tinfo %i %s", cnt, string));
}

void CheckTeamStatus(void) {
    int i;
    gentity_t *loc, *ent;

    if (level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME) {

        level.lastTeamLocationTime = level.time;

        for (i = 0; i < g_maxclients.integer; i++) {
            ent = g_entities + i;

            if (ent->client->pers.connected != CON_CONNECTED) {
                continue;
            }

            if (ent->inuse && (ent->client->sess.sessionTeam == TEAM_RED ||
                               ent->client->sess.sessionTeam == TEAM_BLUE)) {
                loc = Team_GetLocation(ent);
                if (loc)
                    ent->client->pers.teamState.location = loc->health;
                else
                    ent->client->pers.teamState.location = 0;
            }
        }

        for (i = 0; i < g_maxclients.integer; i++) {
            ent = g_entities + i;

            if (ent->client->pers.connected != CON_CONNECTED) {
                continue;
            }

            if (ent->inuse) {
                TeamplayInfoMessage(ent);
            }
        }
    }
}

/*-----------------------------------------------------------------*/

/*QUAKED team_CTF_redplayer (1 0 0) (-16 -16 -16) (16 16 32)
Only in CTF games.  Red players spawn here at game start.
*/
void SP_team_CTF_redplayer(gentity_t *ent) {}

/*QUAKED team_CTF_blueplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in CTF games.  Blue players spawn here at game start.
*/
void SP_team_CTF_blueplayer(gentity_t *ent) {}

/*QUAKED team_CTF_redspawn (1 0 0) (-16 -16 -24) (16 16 32)
potential spawning position for red team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_redspawn(gentity_t *ent) {}

/*QUAKED team_CTF_bluespawn (0 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for blue team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_bluespawn(gentity_t *ent) {}

