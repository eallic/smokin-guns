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

/*****************************************************************************
 * name:		ai_team.c
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /MissionPack/code/game/ai_team.c $
 *
 *****************************************************************************/

#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_char.h"
#include "../botlib/be_ai_chat.h"
#include "../botlib/be_ai_gen.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/be_ai_weap.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
#include "ai_team.h"
#include "ai_vcmd.h"

#include "match.h"

// for the voice chats
#include "../ui/ui_menudef.h"

// ctf task preferences for a client

/*
==================
BotValidTeamLeader
==================
*/
int BotValidTeamLeader(bot_state_t *bs) {
    if (!strlen(bs->teamleader))
        return qfalse;
    if (ClientFromName(bs->teamleader) == -1)
        return qfalse;
    return qtrue;
}

/*
==================
BotNumTeamMates
==================
*/
int BotNumTeamMates(bot_state_t *bs) {
    int i, numplayers;
    char buf[MAX_INFO_STRING];
    static int maxclients;

    if (!maxclients)
        maxclients = trap_Cvar_VariableIntegerValue("sv_maxclients");

    numplayers = 0;
    for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
        trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
        // if no config string or no name
        if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n")))
            continue;
        // skip spectators
        if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR)
            continue;
        //
        if (BotSameTeam(bs, i)) {
            numplayers++;
        }
    }
    return numplayers;
}

/*
==================
BotClientTravelTimeToGoal
==================
*/
int BotClientTravelTimeToGoal(int client, bot_goal_t *goal) {
    playerState_t ps;
    int areanum;

    BotAI_GetClientState(client, &ps);
    areanum = BotPointAreaNum(ps.origin);
    if (!areanum)
        return 1;
    return trap_AAS_AreaTravelTimeToGoalArea(areanum, ps.origin, goal->areanum,
                                             TFL_DEFAULT);
}

/*
==================
BotSortTeamMatesByBaseTravelTime
==================
*/
int BotSortTeamMatesByBaseTravelTime(bot_state_t *bs, int *teammates,
                                     int maxteammates) {

    int i, j, k, numteammates, traveltime;
    char buf[MAX_INFO_STRING];
    static int maxclients;
    int traveltimes[MAX_CLIENTS];
    bot_goal_t *goal = NULL;

    if (!maxclients)
        maxclients = trap_Cvar_VariableIntegerValue("sv_maxclients");

    numteammates = 0;
    for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
        trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
        // if no config string or no name
        if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n")))
            continue;
        // skip spectators
        if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR)
            continue;
        //
        if (BotSameTeam(bs, i)) {
            //
            traveltime = BotClientTravelTimeToGoal(i, goal);
            //
            for (j = 0; j < numteammates; j++) {
                if (traveltime < traveltimes[j]) {
                    for (k = numteammates; k > j; k--) {
                        traveltimes[k] = traveltimes[k - 1];
                        teammates[k] = teammates[k - 1];
                    }
                    break;
                }
            }
            traveltimes[j] = traveltime;
            teammates[j] = i;
            numteammates++;
            if (numteammates >= maxteammates)
                break;
        }
    }
    return numteammates;
}

/*
==================
BotSetTeamMateTaskPreference
==================
*/

/*
==================
BotGetTeamMateTaskPreference
==================
*/

/*
==================
BotSortTeamMatesByTaskPreference
==================
*/

/*
==================
BotSayTeamOrders
==================
*/
void BotSayTeamOrderAlways(bot_state_t *bs, int toclient) {
    char teamchat[MAX_MESSAGE_SIZE];
    char buf[MAX_MESSAGE_SIZE];
    char name[MAX_NETNAME];

    // if the bot is talking to itself
    if (bs->client == toclient) {
        // don't show the message just put it in the console message queue
        trap_BotGetChatMessage(bs->cs, buf, sizeof(buf));
        ClientName(bs->client, name, sizeof(name));
        Com_sprintf(teamchat, sizeof(teamchat), EC "(%s" EC ")" EC ": %s", name,
                    buf);
        trap_BotQueueConsoleMessage(bs->cs, CMS_CHAT, teamchat);
    } else {
        trap_BotEnterChat(bs->cs, toclient, CHAT_TELL);
    }
}

/*
==================
BotSayTeamOrders
==================
*/
void BotSayTeamOrder(bot_state_t *bs, int toclient) {
    BotSayTeamOrderAlways(bs, toclient);
}

/*
==================
BotVoiceChat
==================
*/
void BotVoiceChat(bot_state_t *bs, int toclient, char *voicechat) {
}

/*
==================
BotVoiceChatOnly
==================
*/
void BotVoiceChatOnly(bot_state_t *bs, int toclient, char *voicechat) {
}

/*
==================
BotSayVoiceTeamOrder
==================
*/
void BotSayVoiceTeamOrder(bot_state_t *bs, int toclient, char *voicechat) {
}

/*
==================
BotCTFOrders
==================
*/

/*
==================
BotCTFOrders
==================
*/

/*
==================
BotCTFOrders
==================
*/

/*
==================
BotCTFOrders
==================
*/

/*
==================
BotCTFOrders
==================
*/

/*
==================
BotCreateGroup
==================
*/
void BotCreateGroup(bot_state_t *bs, int *teammates, int groupsize) {
    char name[MAX_NETNAME], leadername[MAX_NETNAME];
    int i;

    // the others in the group will follow the teammates[0]
    ClientName(teammates[0], leadername, sizeof(leadername));
    for (i = 1; i < groupsize; i++) {
        ClientName(teammates[i], name, sizeof(name));
        if (teammates[0] == bs->client) {
            BotAI_BotInitialChat(bs, "cmd_accompanyme", name, NULL);
        } else {
            BotAI_BotInitialChat(bs, "cmd_accompany", name, leadername, NULL);
        }
        BotSayTeamOrderAlways(bs, teammates[i]);
    }
}

/*
==================
BotTeamOrders

  FIXME: defend key areas?
==================
*/
void BotTeamOrders(bot_state_t *bs) {
    int teammates[MAX_CLIENTS];
    int numteammates, i;
    char buf[MAX_INFO_STRING];
    static int maxclients;

    if (!maxclients)
        maxclients = trap_Cvar_VariableIntegerValue("sv_maxclients");

    numteammates = 0;
    for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
        trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
        // if no config string or no name
        if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n")))
            continue;
        // skip spectators
        if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR)
            continue;
        //
        if (BotSameTeam(bs, i)) {
            teammates[numteammates] = i;
            numteammates++;
        }
    }
    //
    switch (numteammates) {
    case 1:
        break;
    case 2: {
        // nothing special
        break;
    }
    case 3: {
        // have one follow another and one free roaming
        BotCreateGroup(bs, teammates, 2);
        break;
    }
    case 4: {
        BotCreateGroup(bs, teammates, 2);     // a group of 2
        BotCreateGroup(bs, &teammates[2], 2); // a group of 2
        break;
    }
    case 5: {
        BotCreateGroup(bs, teammates, 2);     // a group of 2
        BotCreateGroup(bs, &teammates[2], 3); // a group of 3
        break;
    }
    default: {
        if (numteammates <= 10) {
            for (i = 0; i < numteammates / 2; i++) {
                BotCreateGroup(bs, &teammates[i * 2], 2); // groups of 2
            }
        }
        break;
    }
    }
}


/*
==================
FindHumanTeamLeader
==================
*/
int FindHumanTeamLeader(bot_state_t *bs) {
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (g_entities[i].inuse) {
            // if this player is not a bot
            if (!(g_entities[i].r.svFlags & SVF_BOT)) {
                // if this player is ok with being the leader
                if (!notleader[i]) {
                    // if this player is on the same team
                    if (BotSameTeam(bs, i)) {
                        ClientName(i, bs->teamleader, sizeof(bs->teamleader));
                        // if not yet ordered to do anything
                        if (!BotSetLastOrderedTask(bs)) {
                            // go on defense by default
                            BotVoiceChat_Defend(bs, i, SAY_TELL);
                        }
                        return qtrue;
                    }
                }
            }
        }
    }
    return qfalse;
}

/*
==================
BotTeamAI
==================
*/
