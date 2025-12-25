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
#include "g_local.h"

/*

  Items are any object that a player can touch to gain some effect.

  Pickup will return the number of seconds until they should respawn.

  all items should pop when dropped in lava or slime

  Respawnable items don't actually go away when picked up, they are
  just made invisible and untouchable.  This allows them to ride
  movers and respawn apropriately.
*/

#define RESPAWN_ARMOR 25
#define RESPAWN_HEALTH 35
#define RESPAWN_AMMO 40
#define RESPAWN_HOLDABLE 60
#define RESPAWN_MEGAHEALTH 35 // 120
#define RESPAWN_POWERUP 45

//======================================================================

int Pickup_Powerup(gentity_t *ent, gentity_t *other) {
    if (ent->item->giTag == PW_GOLD && g_gametype.integer == GT_BR) {
        other->client->ps.stats[STAT_MONEY] += 10;
        other->client->ps.powerups[ent->item->giTag] = 1;
    } else if (ent->item->giTag == PW_GOLD) {
        other->client->ps.stats[STAT_MONEY] += ent->count;
    } else if (ent->item->giTag == PW_BELT) {
        // Added by Joe Kari: collecting an ammo belt give you full ammo just
        // like when you buy it so Breli will be happy :O)
        other->client->ps.ammo[WP_BULLETS_CLIP] =
            bg_weaponlist[WP_REM58].maxAmmo * 2;
        other->client->ps.ammo[WP_SHELLS_CLIP] =
            bg_weaponlist[WP_REMINGTON_GAUGE].maxAmmo * 2;
        other->client->ps.ammo[WP_CART_CLIP] =
            bg_weaponlist[WP_WINCHESTER66].maxAmmo * 2;
        other->client->ps.ammo[WP_GATLING_CLIP] =
            bg_weaponlist[WP_GATLING].maxAmmo * 2;
        other->client->ps.ammo[WP_SHARPS_CLIP] =
            bg_weaponlist[WP_SHARPS].maxAmmo * 2;
        other->client->ps.powerups[ent->item->giTag] = 1;
    } else {
        other->client->ps.powerups[ent->item->giTag] = 1;
    }

    if (other->client->ps.stats[STAT_MONEY] > g_maxMoney.integer)
        other->client->ps.stats[STAT_MONEY] = g_maxMoney.integer;

    if (ent->item->giTag == PW_GOLD && g_gametype.integer == GT_BR)
        return 0;

    return RESPAWN_POWERUP + rand() % 10;
}

//======================================================================


int Pickup_Holdable(gentity_t *ent, gentity_t *other) {

    other->client->ps.stats[STAT_HOLDABLE_ITEM] = ent->item - bg_itemlist;


    return RESPAWN_HOLDABLE;
}

//======================================================================

void Add_Ammo(gentity_t *ent, int clip, int count) {
    int belt = 1;
    int weapon;

    if (ent->client->ps.powerups[PW_BELT])
        belt = 2;

    switch (clip) {
    case WP_BULLETS_CLIP:
        weapon = WP_PEACEMAKER;
        break;
    case WP_SHELLS_CLIP:
        weapon = WP_REMINGTON_GAUGE;
        break;
    case WP_CART_CLIP:
        weapon = WP_WINCHESTER66;
        break;
    case WP_GATLING_CLIP:
        weapon = WP_GATLING;
        break;
    case WP_SHARPS_CLIP:
        weapon = WP_SHARPS;
        break;
    default:
        weapon = 0;
        break;
    }

    ent->client->ps.ammo[clip] += count;

    if (clip != WP_AKIMBO) {
        if (weapon) {
            if (ent->client->ps.ammo[clip] >
                bg_weaponlist[weapon].maxAmmo * belt) {
                ent->client->ps.ammo[clip] =
                    bg_weaponlist[weapon].maxAmmo * belt;
            }
        } else if (!bg_weaponlist[clip].clip) {
            if (ent->client->ps.ammo[clip] > bg_weaponlist[clip].maxAmmo) {
                ent->client->ps.ammo[clip] = bg_weaponlist[clip].maxAmmo;
            }
        } else if (ent->client->ps.ammo[clip] > bg_weaponlist[clip].clipAmmo) {
            ent->client->ps.ammo[clip] = bg_weaponlist[clip].clipAmmo;
        }
    } else {
        if (ent->client->ps.ammo[clip] > bg_weaponlist[WP_REM58].clipAmmo)
            ent->client->ps.ammo[clip] = bg_weaponlist[WP_REM58].clipAmmo;
    }
}

int Pickup_Ammo(gentity_t *ent, gentity_t *other) {
    int quantity;

    if (ent->count) {
        quantity = ent->count;
    } else {
        quantity = ent->item->quantity;
    }

    Add_Ammo(other, ent->item->giTag, quantity);

    return RESPAWN_AMMO;
}

//======================================================================

int Pickup_Weapon(gentity_t *ent, gentity_t *other) {
    int quantity;

    if (ent->count < 0) {
        quantity = 0; // None for you, sir!
    } else {
        if (ent->count) {
            quantity = ent->count - 1;
        } else {
            quantity = ent->item->quantity;
        }

        // dropped items and teamplay weapons always have full ammo
    }

    // if the weapon is the second the player owns(pistol)
    if (other->client->ps.stats[STAT_WEAPONS] & (1 << ent->item->giTag) &&
        bg_weaponlist[ent->item->giTag].wp_sort == WPS_PISTOL) {
        other->client->ps.stats[STAT_FLAGS] |= SF_SEC_PISTOL;
        Add_Ammo(other, WP_AKIMBO, quantity);
    } else {
        // add the weapon
        other->client->ps.stats[STAT_WEAPONS] |= (1 << ent->item->giTag);

        Add_Ammo(other, ent->item->giTag, quantity);
    }

    return g_weaponRespawn.integer;
}

//======================================================================


//======================================================================

int Pickup_Armor(gentity_t *ent, gentity_t *other) {
    if (ent->count) {
        other->client->ps.stats[STAT_ARMOR] += ent->count;
    } else {
        other->client->ps.stats[STAT_ARMOR] += ent->item->quantity;
    }
    if (other->client->ps.stats[STAT_ARMOR] >
        BOILER_PLATE /*other->client->ps.stats[STAT_MAX_HEALTH] * 2*/) {
        other->client->ps.stats[STAT_ARMOR] =
            BOILER_PLATE /*other->client->ps.stats[STAT_MAX_HEALTH] * 2*/;
    }

    return RESPAWN_ARMOR;
}

//======================================================================

/*
===============
RespawnItem
===============
*/
void RespawnItem(gentity_t *ent) {
    // randomly select from teamed entities
    if (ent->team) {
        gentity_t *master;
        int count;
        int choice;

        if (!ent->teammaster) {
            G_Error("RespawnItem: bad teammaster");
        }
        master = ent->teammaster;

        for (count = 0, ent = master; ent; ent = ent->teamchain, count++)
            ;

        choice = rand() % count;

        for (count = 0, ent = master; count < choice;
             ent = ent->teamchain, count++)
            ;
    }

    ent->r.contents = CONTENTS_TRIGGER;

    if (ent->item->giTag == WP_DYNAMITE && ent->item->giType == IT_WEAPON)
        ent->r.contents = CONTENTS_EXPLOSIVE;

    ent->s.eFlags &= ~EF_NODRAW;
    ent->r.svFlags &= ~SVF_NOCLIENT;
    trap_LinkEntity(ent);


    // play the normal respawn sound only to nearby clients
    G_AddEvent(ent, EV_ITEM_RESPAWN, 0);

    ent->nextthink = 0;
}

/*
===============
Touch_Item
===============
*/
void Touch_Item(gentity_t *ent, gentity_t *other, trace_t *trace) {
    int respawn;
    qboolean predict;

    if (!other->client)
        return;
    if (other->health < 1)
        return; // dead people can't pickup

    // the same pickup rules are used for client side and server side
    if (!BG_CanItemBeGrabbed(g_gametype.integer, &ent->s, &other->client->ps)) {
        return;
    }


    predict = other->client->pers.predictItemPickup;

    // call the item-specific pickup function
    switch (ent->item->giType) {
    case IT_WEAPON:
        respawn = Pickup_Weapon(ent, other);
        //		predict = qfalse;
        break;
    case IT_AMMO:
        respawn = Pickup_Ammo(ent, other);
        //		predict = qfalse;
        break;
    case IT_ARMOR:
        respawn = Pickup_Armor(ent, other);
        break;
    case IT_POWERUP:
        respawn = Pickup_Powerup(ent, other);
        predict = qfalse;
        break;
    default:
        G_LogPrintf("Item: %i %s\n", other->s.number, ent->item->classname);
        return;
    }

    if (g_gametype.integer >= GT_RTP)
        respawn = -1;

    if (!respawn) {
        G_LogPrintf("Item: %i %s\n", other->s.number, ent->item->classname);
        return;
    }

    // play the normal pickup sound
    if (predict) {
        if (!Q_stricmp(ent->item->classname, "pickup_money"))
            G_AddPredictableEvent(other, EV_MONEY_PICKUP, ent->count);
        else
            G_AddPredictableEvent(other, EV_ITEM_PICKUP, ent->s.modelindex);
    } else {
        if (!Q_stricmp(ent->item->classname, "pickup_money"))
            G_AddEvent(other, EV_MONEY_PICKUP, ent->count);
        else
            G_AddEvent(other, EV_ITEM_PICKUP, ent->s.modelindex);
    }

    // powerup pickups are global broadcasts

    // fire item targets
    G_UseTargets(ent, other);

    // wait of -1 will not respawn
    if (ent->wait == -1) {
        ent->r.svFlags |= SVF_NOCLIENT;
        ent->s.eFlags |= EF_NODRAW;
        ent->r.contents = 0;
        ent->unlinkAfterEvent = qtrue;
        G_LogPrintf("Item: %i %s\n", other->s.number, ent->item->classname);
        return;
    }

    // non zero wait overrides respawn time
    if (ent->wait) {
        respawn = ent->wait;
    }

    // random can be used to vary the respawn time
    if (ent->random) {
        respawn += crandom() * ent->random;
        if (respawn < 1) {
            respawn = 1;
        }
    }

    // dropped items will not respawn
    if (ent->flags & FL_DROPPED_ITEM) {
        ent->freeAfterEvent = qtrue;
    }

    // picked up items still stay around, they just don't
    // draw anything.  This allows respawnable items
    // to be placed on movers.
    ent->r.svFlags |= SVF_NOCLIENT;
    ent->s.eFlags |= EF_NODRAW;
    ent->r.contents = 0;

    // ZOID
    // A negative respawn times means to never respawn this item (but don't
    // delete it).  This is used by items that are respawned by third party
    // events such as ctf flags
    if (respawn <= 0) {
        ent->nextthink = 0;
        ent->think = 0;
    } else {
        ent->nextthink = level.time + respawn * 1000;
        ent->think = RespawnItem;
    }
    // Tequila comment: Check and report the buy case
    if (ent->flags & FL_BUY_ITEM)
        G_LogPrintf("Item: %i %s bought ($%i/$%i)\n", other->s.number,
                    ent->item->classname, ent->item->prize,
                    other->client->ps.stats[STAT_MONEY]);
    else if (!Q_stricmp(ent->item->classname, "pickup_money"))
        G_LogPrintf("Item: %i %s (%i) picked up ($%i)\n", other->s.number,
                    ent->item->classname, ent->count,
                    other->client->ps.stats[STAT_MONEY]);
    else
        G_LogPrintf("Item: %i %s (%i) picked up\n", other->s.number,
                    ent->item->classname, ent->count);
    trap_LinkEntity(ent);
}

//======================================================================

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t *LaunchItem(gitem_t *item, vec3_t origin, vec3_t velocity,
                      int droppedflags) {
    gentity_t *dropped;

    dropped = G_Spawn();

    dropped->s.eType = ET_ITEM;
    dropped->s.modelindex =
        item - bg_itemlist;     // store item number in modelindex
    dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

    dropped->classname = item->classname;
    dropped->item = item;
    G_SetItemBox(dropped);

    dropped->r.contents = CONTENTS_TRIGGER;
    if (dropped->item->giTag == WP_DYNAMITE &&
        dropped->item->giType == IT_WEAPON)
        dropped->r.contents = CONTENTS_EXPLOSIVE;

    dropped->touch = Touch_Item;

    G_SetOrigin(dropped, origin);
    dropped->s.pos.trType = TR_GRAVITY;
    dropped->s.pos.trTime = level.time;
    VectorCopy(velocity, dropped->s.pos.trDelta);

    dropped->s.eFlags |= EF_BOUNCE_HALF;
    if (!(item->giType == IT_POWERUP &&
          item->giTag == PW_GOLD)) { // auto-remove after 30 seconds
        dropped->think = G_FreeEntity;
        dropped->nextthink = level.time + 30000;
    }

    VectorCopy(dropped->r.currentAngles, dropped->s.apos.trBase);

    dropped->flags = droppedflags;        // FL_DROPPED_ITEM;
    dropped->s.eFlags |= EF_DROPPED_ITEM; // for cgame

    if (droppedflags & FL_THROWN_ITEM) {
        dropped->clipmask = MASK_SHOT;
        dropped->s.pos.trTime =
            level.time - 50; // move a bit on the very first frame
        VectorScale(velocity, 200, dropped->s.pos.trDelta); // 700
        SnapVector(dropped->s.pos.trDelta); // save net bandwidth
        dropped->physicsBounce = 0.2f;
    }

    trap_LinkEntity(dropped);

    return dropped;
}

/*
================
G_dropWeapon
================
*/
gentity_t *G_dropWeapon(gentity_t *ent, gitem_t *item, float angle,
                        int flags) { // XRAY FMJ
    vec3_t velocity;
    vec3_t angles, dir;

    // don't drop anything in duel
    if (g_gametype.integer == GT_DUEL)
        return NULL;

    // set aiming directions
    VectorCopy(ent->s.apos.trBase, angles);
    angles[YAW] += 0;
    angles[PITCH] = 0; // always forward

    AngleVectors(angles, velocity, NULL, NULL);
    VectorScale(velocity, 50, velocity);
    velocity[2] += 10;
    VectorNormalize(velocity);

    /*modified by Spoon Start*/
    VectorClear(dir);
    AngleVectors(angles, dir, NULL, NULL);
    VectorScale(dir, 50, dir);
    VectorAdd(ent->s.pos.trBase, dir, dir);

    // snap to integer coordinates for more efficient network bandwidth usage
    SnapVector(dir);

    return LaunchItem(item, dir, velocity, flags);
}

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t *Drop_Item(gentity_t *ent, gitem_t *item, float angle) {
    vec3_t velocity;
    vec3_t angles;
    int ammo;

    if (item->giType == IT_WEAPON)
        ammo = ent->client->ps.ammo[item->giTag];

    VectorCopy(ent->s.apos.trBase, angles);
    angles[YAW] += angle;
    angles[PITCH] = 0; // always forward

    AngleVectors(angles, velocity, NULL, NULL);
    VectorScale(velocity, 100, velocity);
    velocity[2] += 200 + crandom() * 50;

    // if player has no dynamite in his hands reset the mode to 0, otherwise
    // the dynamite(unlit) would explode
    if (ent->client->ps.weapon != WP_DYNAMITE &&
        ent->client->ps.weapon != WP_MOLOTOV)
        ent->client->ps.stats[STAT_WP_MODE] = 0;

    if (item->giTag == WP_DYNAMITE && item->giType == IT_WEAPON) {
        return fire_dynamite(ent, ent->s.pos.trBase, velocity, 50);
    } else if (item->giTag == WP_MOLOTOV && item->giType == IT_WEAPON &&
               ent->client->ps.stats[STAT_WP_MODE] < 0) {
        return fire_molotov(ent, ent->s.pos.trBase, velocity, 50);
    } else
        return LaunchItem(item, ent->s.pos.trBase, velocity, FL_DROPPED_ITEM);
}

/*
================
Use_Item

Respawn the item
================
*/
void Use_Item(gentity_t *ent, gentity_t *other, gentity_t *activator) {
    RespawnItem(ent);
}

//======================================================================

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningItem(gentity_t *ent) {
    trace_t tr;
    vec3_t dest;

    vec3_t mins, maxs;

    VectorSet(mins, -8, -8, -4);
    VectorSet(maxs, 8, 8, 21.63f);

    // mins and maxs is now set by G_SetItemBox
    G_SetItemBox(ent);

    ent->s.eType = ET_ITEM;
    ent->s.modelindex =
        ent->item - bg_itemlist; // store item number in modelindex
    ent->s.modelindex2 = 0;      // zero indicates this isn't a dropped item

    ent->r.contents = CONTENTS_TRIGGER;

    if (ent->item->giTag == WP_DYNAMITE && ent->item->giType == IT_WEAPON)
        ent->r.contents = CONTENTS_EXPLOSIVE;

    ent->touch = Touch_Item;
    // using an item causes it to respawn
    ent->use = Use_Item;

    if (ent->flags & FL_BUY_ITEM) {
        // only to be being bought not to sit around
        G_SetOrigin(ent, ent->s.origin);
        return;
    }

        // drop to floor
        VectorSet(dest, ent->s.origin[0], ent->s.origin[1],
                  ent->s.origin[2] - 4096);
        trap_Trace(&tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest,
                   ent->s.number, MASK_SOLID);

        if (tr.startsolid) {
        if (ent->item->giType == IT_POWERUP && ent->item->giTag == PW_GOLD) {
            trap_Trace(&tr, ent->s.origin, mins, maxs, dest, ent->s.number,
                       MASK_SOLID);
        }

        if (tr.startsolid) {
            G_Printf("FinishSpawningItem: %s startsolid at %s\n",
                     ent->classname, vtos(ent->s.origin));
            G_FreeEntity(ent);
            return;
        }
        }

        // allow to ride movers
        ent->s.groundEntityNum = tr.entityNum;

        G_SetOrigin(ent, tr.endpos);

    VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
    VectorCopy(ent->r.currentAngles, ent->s.apos.trBase);

    if (ent->item->giType == IT_POWERUP && ent->item->giTag == PW_GOLD) {
        if (g_gametype.integer != GT_BR) {
            if (ent->count <= 0)
                ent->count = 10;

            ent->s.time2 = ent->count;
            ent->s.apos.trBase[YAW] = ent->r.currentAngles[YAW] = rand() % 360;
        }
    }

    // team slaves and targeted items aren't present at start
    if ((ent->flags & FL_TEAMSLAVE) || ent->targetname) {
        ent->s.eFlags |= EF_NODRAW;
        ent->r.contents = 0;
        return;
    }

    // powerups don't spawn in for a while

    trap_LinkEntity(ent);
}

qboolean itemRegistered[MAX_ITEMS];

/*
==================
G_CheckTeamItems
==================
*/
void G_CheckTeamItems(void) {

    // Set up team stuff
    Team_InitGame();

}

/*
==============
ClearRegisteredItems
==============
*/
void ClearRegisteredItems(void) {
    int i;
    memset(itemRegistered, 0, sizeof(itemRegistered));

    // players always start with the base weapon
    for (i = WP_KNIFE; i < WP_NUM_WEAPONS; i++) {

        if (g_gametype.integer == GT_DUEL &&
            bg_weaponlist[i].wp_sort != WPS_PISTOL && i != WP_KNIFE)
            continue;

        RegisterItem(BG_FindItemForWeapon(i));
    }

    // additional items
    if (g_gametype.integer != GT_DUEL) {
        RegisterItem(BG_FindItemForClassname("item_scope"));
        RegisterItem(BG_FindItemForClassname("item_boiler_plate"));
        RegisterItem(BG_FindItemForClassname("item_belt"));
    }

    // money-bag for bank robbery
    if (g_gametype.integer == GT_BR)
        RegisterItem(BG_FindItemForClassname("item_money"));
    else if (g_gametype.integer != GT_RTP)
        RegisterItem(BG_FindItemForClassname("pickup_money"));

}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem(gitem_t *item) {
    if (!item) {
        G_Error("RegisterItem: NULL");
    }
    itemRegistered[item - bg_itemlist] = qtrue;
}

/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredItems(void) {
    char string[MAX_ITEMS + 1];
    int i;
    int count;

    count = 0;
    for (i = 0; i < bg_numItems; i++) {
        if (itemRegistered[i]) {
            count++;
            string[i] = '1';
        } else {
            string[i] = '0';
        }
    }
    string[bg_numItems] = 0;

    G_Printf("%i items registered\n", count);
    trap_SetConfigstring(CS_ITEMS, string);
}

/*
============
G_ItemDisabled
============
*/
int G_ItemDisabled(gitem_t *item) {

    char name[128];

    Com_sprintf(name, sizeof(name), "disable_%s", item->classname);
    return trap_Cvar_VariableIntegerValue(name);
}

/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnItem(gentity_t *ent, gitem_t *item) {
    G_SpawnFloat("random", "0", &ent->random);
    G_SpawnFloat("wait", "0", &ent->wait);

    RegisterItem(item);
    if (G_ItemDisabled(item))
        return;

    ent->item = item;
    // some movers spawn on the second frame, so delay item
    // spawns until the third frame so they can ride trains
    ent->nextthink = level.time + FRAMETIME * 2;
    ent->think = FinishSpawningItem;

    ent->physicsBounce = 0.30f; // items are bouncy
}

/*
================
G_BounceItem

================
*/
void G_BounceItem(gentity_t *ent, trace_t *trace) {
    vec3_t velocity;
    float dot;
    int hitTime;

    // reflect the velocity on the trace plane
    hitTime = level.previousTime +
              (level.time - level.previousTime) * trace->fraction;
    BG_EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity);
    dot = DotProduct(velocity, trace->plane.normal);
    VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);

    // cut the velocity to keep from bouncing forever
    VectorScale(ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta);

    // check for stop
    if (trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40) {
        trace->endpos[2] += 1.0; // make sure it is off ground
        SnapVector(trace->endpos);
        G_SetOrigin(ent, trace->endpos);
        ent->s.groundEntityNum = trace->entityNum;
        return;
    }

    VectorAdd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
    VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
    ent->s.pos.trTime = level.time;
}

/*
================
G_RunItem

================
*/
void G_RunItem(gentity_t *ent) {
    vec3_t origin;
    trace_t tr;
    int contents;
    int mask;

    // if its groundentity has been set to none, it may have been pushed off an
    // edge
    if (ent->s.groundEntityNum == ENTITYNUM_NONE) {
        if (ent->s.pos.trType != TR_GRAVITY) {
            ent->s.pos.trType = TR_GRAVITY;
            ent->s.pos.trTime = level.time;
        }
    }

    if (ent->s.pos.trType == TR_STATIONARY) {
        // check think function
        G_RunThink(ent);
        return;
    }

    // get current position
    BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);

    // trace a line from the previous position to the current position
    if (ent->clipmask) {
        mask = ent->clipmask;
    } else {
        mask = MASK_PLAYERSOLID & ~CONTENTS_BODY; // MASK_SOLID;
    }
    trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
               ent->r.ownerNum, mask);

    VectorCopy(tr.endpos, ent->r.currentOrigin);

    if (tr.startsolid) {
        tr.fraction = 0;
    }

    trap_LinkEntity(ent); // FIXME: avoid this for stationary?

    // check think function
    G_RunThink(ent);

    if (tr.fraction == 1) {
        return;
    }

    // check later if object has been removed in which knife stucks into
    VectorCopy(origin, ent->s.origin2);

    // if it is in a nodrop volume, remove it
    contents = trap_PointContents(ent->r.currentOrigin, -1);
    if (contents & CONTENTS_NODROP) {
        if (ent->item && ent->item->giType == IT_TEAM) {
            Team_FreeEntity(ent);
        } else {
            G_FreeEntity(ent);
        }
        return;
    }

    G_BounceItem(ent, &tr);
}
