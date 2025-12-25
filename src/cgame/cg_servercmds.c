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
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame

#include "cg_local.h"

/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores(void) {
    int i, powerups;

    cg.numScores = atoi(CG_Argv(1));
    if (cg.numScores > MAX_CLIENTS) {
        cg.numScores = MAX_CLIENTS;
    }

    cg.teamScores[0] = atoi(CG_Argv(2));
    cg.teamScores[1] = atoi(CG_Argv(3));

    memset(cg.scores, 0, sizeof(cg.scores));
    for (i = 0; i < cg.numScores; i++) {
        //
        cg.scores[i].client = atoi(CG_Argv(i * 8 + 4));
        cg.scores[i].score = atoi(CG_Argv(i * 8 + 5));
        cg.scores[i].ping = atoi(CG_Argv(i * 8 + 6));
        cg.scores[i].time = atoi(CG_Argv(i * 8 + 7));
        cg.scores[i].scoreFlags = atoi(CG_Argv(i * 8 + 8));
        powerups = atoi(CG_Argv(i * 8 + 9));
        cg.scores[i].realspec = atoi(CG_Argv(i * 8 + 10));
        cg.scores[i].won = atoi(CG_Argv(i * 8 + 11));
        cg.scores[i].accuracy = 0;
        cg.scores[i].impressiveCount = 0;
        cg.scores[i].excellentCount = 0;
        cg.scores[i].guantletCount = 0;
        cg.scores[i].defendCount = 0;
        cg.scores[i].assistCount = 0;
        cg.scores[i].perfect = 0;
        cg.scores[i].captures = 0;

        if (cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS) {
            cg.scores[i].client = 0;
        }
        cgs.clientinfo[cg.scores[i].client].score = cg.scores[i].score;
        cgs.clientinfo[cg.scores[i].client].powerups = powerups;

        cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;
    }
    CG_SetScoreSelection(NULL);
}

/*
=================
CG_ParseTeamInfo

=================
*/
static void CG_ParseTeamInfo(void) {
    int i;
    int client;

    numSortedTeamPlayers = atoi(CG_Argv(1));
    if (numSortedTeamPlayers < 0 || numSortedTeamPlayers > TEAM_MAXOVERLAY) {
        CG_Error("CG_ParseTeamInfo: numSortedTeamPlayers out of range (%d)",
                 numSortedTeamPlayers);
        return;
    }

    for (i = 0; i < numSortedTeamPlayers; i++) {
        client = atoi(CG_Argv(i * 6 + 2));
        if (client < 0 || client >= MAX_CLIENTS) {
            CG_Error("CG_ParseTeamInfo: bad client number: %d", client);
            return;
        }

        sortedTeamPlayers[i] = client;

        cgs.clientinfo[client].location = atoi(CG_Argv(i * 6 + 3));
        cgs.clientinfo[client].health = atoi(CG_Argv(i * 6 + 4));
        cgs.clientinfo[client].armor = atoi(CG_Argv(i * 6 + 5));
        cgs.clientinfo[client].curWeapon = atoi(CG_Argv(i * 6 + 6));
        cgs.clientinfo[client].powerups = atoi(CG_Argv(i * 6 + 7));
    }
}

/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo(void) {
    const char *info;
    char *mapname;
    char *s;

    info = CG_ConfigString(CS_SERVERINFO);
    cgs.gametype = atoi(Info_ValueForKey(info, "g_gametype"));
    trap_Cvar_Set("g_gametype", va("%i", cgs.gametype));
    cgs.dmflags = atoi(Info_ValueForKey(info, "dmflags"));
    cgs.teamflags = atoi(Info_ValueForKey(info, "teamflags"));
    s = Info_ValueForKey(info, "fraglimit");
    trap_Cvar_Set("cg_serverfraglimit", s);
    cgs.fraglimit = cg_serverfraglimit.integer;

    s = Info_ValueForKey(info, "duellimit");
    trap_Cvar_Set("cg_serverduellimit", s);
    cgs.duellimit = cg_serverduellimit.integer;

    cgs.scorelimit = atoi(Info_ValueForKey(info, "scorelimit"));
    cgs.specsareflies = atoi(Info_ValueForKey(info, "g_specsareflies"));
    cgs.timelimit = atoi(Info_ValueForKey(info, "timelimit"));
    cgs.maxclients = atoi(Info_ValueForKey(info, "sv_maxclients"));
    mapname = Info_ValueForKey(info, "mapname");
    Com_sprintf(cgs.mapname, sizeof(cgs.mapname), "maps/%s.bsp", mapname);
    Q_strncpyz(cgs.redTeam, Info_ValueForKey(info, "g_redTeam"),
               sizeof(cgs.redTeam));
    trap_Cvar_Set("g_redTeam", cgs.redTeam);
    Q_strncpyz(cgs.blueTeam, Info_ValueForKey(info, "g_blueTeam"),
               sizeof(cgs.blueTeam));
    trap_Cvar_Set("g_blueTeam", cgs.blueTeam);

    s = Info_ValueForKey(info, "g_newShotgunPattern");
    trap_Cvar_Set("cg_newShotgunPattern", s);
    cgs.newShotgunPattern = cg_newShotgunPattern.integer;

    s = Info_ValueForKey(info, "g_maxMoney");
    trap_Cvar_Set("cg_maxMoney", s);
    cgs.maxMoney = cg_maxMoney.integer;

    s = Info_ValueForKey(info, "g_roundNoMoveTime");
    trap_Cvar_Set("cg_roundNoMoveTime", s);
    cgs.roundNoMoveTime = (int)(cg_roundNoMoveTime.value * 1000);

    // unlagged - server options
    //  we'll need this for deciding whether or not to predict weapon effects
    cgs.delagHitscan = atoi(Info_ValueForKey(info, "g_delagHitscan"));
    trap_Cvar_Set("g_delagHitscan", va("%i", cgs.delagHitscan));
// unlagged - server options
}

/*
==================
CG_ParseWarmup
==================
*/
static void CG_ParseWarmup(void) {
    const char *info;
    int warmup;

    info = CG_ConfigString(CS_WARMUP);

    warmup = atoi(info);
    cg.warmupCount = -1;

    if (warmup == 0 && cg.warmup) {

    } else if (warmup > 0 && cg.warmup <= 0) {
    }

    cg.warmup = warmup;
}

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues(void) {

    cgs.scores1 = atoi(CG_ConfigString(CS_SCORES1));
    cgs.scores2 = atoi(CG_ConfigString(CS_SCORES2));
    cgs.levelStartTime = atoi(CG_ConfigString(CS_LEVEL_START_TIME));
    cg.warmup = atoi(CG_ConfigString(CS_WARMUP));
}

/*
=====================
CG_ShaderStateChanged
=====================
*/
void CG_ShaderStateChanged(void) {
    char originalShader[MAX_QPATH];
    char newShader[MAX_QPATH];
    char timeOffset[16];
    const char *o;
    char *n, *t;

    o = CG_ConfigString(CS_SHADERSTATE);
    while (o && *o) {
        n = strstr(o, "=");
        if (n && *n) {
            strncpy(originalShader, o, n - o);
            originalShader[n - o] = 0;
            n++;
            t = strstr(n, ":");
            if (t && *t) {
                strncpy(newShader, n, t - n);
                newShader[t - n] = 0;
            } else {
                break;
            }
            t++;
            o = strstr(t, "@");
            if (o) {
                strncpy(timeOffset, t, o - t);
                timeOffset[o - t] = 0;
                o++;
                trap_R_RemapShader(originalShader, newShader, timeOffset);
            }
        } else {
            break;
        }
    }
}

/*
================
CG_ConfigStringModified

================
*/
static void CG_ConfigStringModified(void) {
    const char *str;
    int num;

    num = atoi(CG_Argv(1));

    // get the gamestate from the client system, which will have the
    // new configstring already integrated
    trap_GetGameState(&cgs.gameState);

    // look up the individual string that was modified
    str = CG_ConfigString(num);

    // do something with it if necessary
    if (num == CS_MUSIC) {
        CG_SelectMusic();
    } else if (num == CS_SERVERINFO) {
        CG_ParseServerinfo();
    } else if (num == CS_WARMUP) {
        CG_ParseWarmup();
    } else if (num == CS_SCORES1) {
        cgs.scores1 = atoi(str);
    } else if (num == CS_SCORES2) {
        cgs.scores2 = atoi(str);
    } else if (num == CS_LEVEL_START_TIME) {
        cgs.levelStartTime = atoi(str);
    } else if (num == CS_VOTE_TIME) {
        cgs.voteTime = atoi(str);
        cgs.voteModified = qtrue;
    } else if (num == CS_VOTE_YES) {
        cgs.voteYes = atoi(str);
        cgs.voteModified = qtrue;
    } else if (num == CS_VOTE_NO) {
        cgs.voteNo = atoi(str);
        cgs.voteModified = qtrue;
    } else if (num == CS_VOTE_STRING) {
        Q_strncpyz(cgs.voteString, str, sizeof(cgs.voteString));
    } else if (num >= CS_TEAMVOTE_TIME && num <= CS_TEAMVOTE_TIME + 1) {
        cgs.teamVoteTime[num - CS_TEAMVOTE_TIME] = atoi(str);
        cgs.teamVoteModified[num - CS_TEAMVOTE_TIME] = qtrue;
    } else if (num >= CS_TEAMVOTE_YES && num <= CS_TEAMVOTE_YES + 1) {
        cgs.teamVoteYes[num - CS_TEAMVOTE_YES] = atoi(str);
        cgs.teamVoteModified[num - CS_TEAMVOTE_YES] = qtrue;
    } else if (num >= CS_TEAMVOTE_NO && num <= CS_TEAMVOTE_NO + 1) {
        cgs.teamVoteNo[num - CS_TEAMVOTE_NO] = atoi(str);
        cgs.teamVoteModified[num - CS_TEAMVOTE_NO] = qtrue;
    } else if (num >= CS_TEAMVOTE_STRING && num <= CS_TEAMVOTE_STRING + 1) {
        Q_strncpyz(cgs.teamVoteString[num - CS_TEAMVOTE_STRING], str,
                   sizeof(cgs.teamVoteString));
    } else if (num == CS_INTERMISSION) {
        cg.intermissionStarted = atoi(str);
    } else if (num >= CS_MODELS && num < CS_MODELS + MAX_MODELS) {
        cgs.gameModels[num - CS_MODELS] = trap_R_RegisterModel(str);
    } else if (num >= CS_SOUNDS && num < CS_SOUNDS + MAX_SOUNDS) {
        if (str[0] != '*') { // player specific sounds don't register here
            cgs.gameSounds[num - CS_SOUNDS] = trap_S_RegisterSound(str, qfalse);
        }
    } else if (num >= CS_PLAYERS && num < CS_PLAYERS + MAX_CLIENTS) {
        CG_NewClientInfo(num - CS_PLAYERS);
        CG_BuildSpectatorString();
    } else if (num == CS_FLAGSTATUS) {
    } else if (num == CS_SHADERSTATE) {
        CG_ShaderStateChanged();
    }
}

/*
=======================
CG_AddToTeamChat

=======================
*/
/*
=======================
CG_AddToChat
Based on vanilla CG_AddToTeamChat
=======================
*/
static void CG_AddToChat(const char *str) {
    int len;
    char *p, *ls;
    int lastcolor;
    int chatHeight, chatWidth;

    if (cg_chatHeight.integer < CHAT_HEIGHT) {
        chatHeight = cg_chatHeight.integer;
    } else {
        chatHeight = CHAT_HEIGHT;
    }

    if (chatHeight <= 0 || cg_chatTime.integer <= 0) {
        // chat disabled, dump into normal chat
        cgs.ChatPos = cgs.LastChatPos = 0;
        return;
    }

    if (cg_chatWidth.integer < CHAT_WIDTH) {
        chatWidth = cg_chatWidth.integer;
    } else {
        chatWidth = CHAT_WIDTH;
    }

    len = 0;

    p = cgs.ChatMsgs[cgs.ChatPos % chatHeight];
    *p = 0;

    lastcolor = '7';

    ls = NULL;
    while (*str) {
        if (len > chatWidth - 1) {
            if (ls) {
                str -= (p - ls);
                str++;
                p -= (p - ls);
            }
            *p = 0;

            cgs.ChatMsgTimes[cgs.ChatPos % chatHeight] = cg.time;

            cgs.ChatPos++;
            p = cgs.ChatMsgs[cgs.ChatPos % chatHeight];
            *p = 0;
            *p++ = Q_COLOR_ESCAPE;
            *p++ = lastcolor;
            len = 0;
            ls = NULL;
        }

        if (Q_IsColorString(str)) {
            *p++ = *str++;
            lastcolor = *str;
            *p++ = *str++;
            continue;
        }
        if (*str == ' ') {
            ls = p;
        }
        *p++ = *str++;
        len++;
    }
    *p = 0;

    cgs.ChatMsgTimes[cgs.ChatPos % chatHeight] = cg.time;
    cgs.ChatPos++;

    if (cgs.ChatPos - cgs.LastChatPos > chatHeight)
        cgs.LastChatPos = cgs.ChatPos - chatHeight;
}

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart(void) {
    if (cg_showmiss.integer) {
        CG_Printf("CG_MapRestart\n");
    }

    CG_InitLocalEntities();
    CG_InitMarkPolys();

    // make sure the "3 frags left" warnings play again
    cg.fraglimitWarnings = 0;

    cg.timelimitWarnings = 0;
    cg.rewardTime = 0;
    cg.rewardStack = 0;
    cg.intermissionStarted = qfalse;
    cg.levelShot = qfalse;

    cgs.voteTime = 0;

    cg.mapRestart = qtrue;

    CG_SelectMusic();

    trap_S_ClearLoopingSounds(qtrue);

    trap_Cvar_Set("cl_menu", "0");

    // we really should clear more parts of cg here and stop sounds

    // play the "fight" sound if this is a restart without warmup
    if (cg_singlePlayerActive.integer) {
        trap_Cvar_Set("ui_matchStartTime", va("%i", cg.time));
        if (cg_recordSPDemo.integer && cg_recordSPDemoName.string &&
            *cg_recordSPDemoName.string) {
            trap_SendConsoleCommand(
                va("set g_synchronousclients 1 ; record %s \n",
                   cg_recordSPDemoName.string));
        }
    }
}

// disable voice chat

/*
=================
CG_RemoveChatEscapeChar
=================
*/
static void CG_RemoveChatEscapeChar(char *text) {
    int i, l;

    l = 0;
    for (i = 0; text[i]; i++) {
        if (text[i] == '\x19')
            continue;
        text[l++] = text[i];
    }
    text[l] = '\0';
}

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
static void CG_ServerCommand(void) {
    const char *cmd;
    char text[MAX_SAY_TEXT];

    cmd = CG_Argv(0);

    if (!cmd[0]) {
        // server claimed the command
        return;
    }

    if (!strcmp(cmd, "cp")) {
        CG_CenterPrint(CG_Argv(1), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
        return;
    }

    if (!strcmp(cmd, "cmappart")) {
        cg.currentmappart = atoi(CG_Argv(1));
        return;
    }

    if (!strcmp(cmd, "cs")) {
        CG_ConfigStringModified();
        return;
    }

    if (!strcmp(cmd, "print")) {
        CG_Printf("%s", CG_Argv(1));
        return;
    }

    if (!strcmp(cmd, "chat")) {
        if (cg_chatMode.integer < 2) {
            CG_PlayTalkSound();
            Q_strncpyz(text, CG_Argv(1), MAX_SAY_TEXT);
            CG_RemoveChatEscapeChar(text);
            if (!cg_chatMode.integer) {
                CG_Printf("%s\n", text);
            } else {
                CG_AddToChat(text);
                CG_Printf("[skipnotify]%s\n", text);
            }
        }
        return;
    }

    if (!strcmp(cmd, "tchat")) {
        CG_PlayTalkSound();
        Q_strncpyz(text, CG_Argv(1), MAX_SAY_TEXT);
        CG_RemoveChatEscapeChar(text);
        if (!cg_chatMode.integer) {
            CG_Printf("%s\n", text);
        } else {
            CG_AddToChat(text);
            CG_Printf("[skipnotify]%s\n", text);
        }
        return;
    }


    if (!strcmp(cmd, "scores")) {
        CG_ParseScores();
        return;
    }

    if (!strcmp(cmd, "tinfo")) {
        CG_ParseTeamInfo();
        return;
    }

    if (!strcmp(cmd, "map_restart")) {
        CG_MapRestart();
        return;
    }

    if (Q_stricmp(cmd, "remapShader") == 0) {
        if (trap_Argc() == 4) {
            char shader1[MAX_QPATH];
            char shader2[MAX_QPATH];
            char shader3[MAX_QPATH];

            Q_strncpyz(shader1, CG_Argv(1), sizeof(shader1));
            Q_strncpyz(shader2, CG_Argv(2), sizeof(shader2));
            Q_strncpyz(shader3, CG_Argv(3), sizeof(shader3));

            trap_R_RemapShader(shader1, shader2, shader3);
        }

        return;
    }

    // loaddeferred can be both a servercmd and a consolecmd
    if (!strcmp(
            cmd,
            "loaddefered")) { // FIXME: spelled wrong, but not changing for demo
        CG_LoadDeferredPlayers();
        return;
    }

    // clientLevelShot is sent before taking a special screenshot for
    // the menu system during development
    if (!strcmp(cmd, "clientLevelShot")) {
        cg.levelShot = qtrue;
        return;
    }

    CG_Printf("Unknown client game command: %s\n", cmd);
}

/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteNewServerCommands(int latestSequence) {
    while (cgs.serverCommandSequence < latestSequence) {
        if (trap_GetServerCommand(++cgs.serverCommandSequence)) {
            CG_ServerCommand();
        }
    }
}
