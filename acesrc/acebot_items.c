///////////////////////////////////////////////////////////////////////
//
//  ACE - Quake II Bot Base Code
//
//  Version 1.0
//
//  This file is Copyright(c), Steve Yeager 1998, All Rights Reserved
//
//
//	All other files are Copyright(c) Id Software, Inc.
//
//	Please see liscense.txt in the source directory for the copyright
//	information regarding those files belonging to Id Software, Inc.
//	
//	Should you decide to release a modified version of ACE, you MUST
//	include the following text (minus the BEGIN and END lines) in the 
//	documentation for your modification.
//
//	--- BEGIN ---
//
//	The ACE Bot is a product of Steve Yeager, and is available from
//	the ACE Bot homepage, at http://www.axionfx.com/ace.
//
//	This program is a modification of the ACE Bot, and is therefore
//	in NO WAY supported by Steve Yeager.

//	This program MUST NOT be sold in ANY form. If you have paid for 
//	this product, you should contact Steve Yeager immediately, via
//	the ACE Bot homepage.
//
//	--- END ---
//
//	I, Steve Yeager, hold no responsibility for any harm caused by the
//	use of this source code, especially to small children and animals.
//  It is provided as-is with no implied warranty or support.
//
//  I also wish to thank and acknowledge the great work of others
//  that has helped me to develop this code.
//
//  John Cricket    - For ideas and swapping code.
//  Ryan Feltrin    - For ideas and swapping code.
//  SABIN           - For showing how to do true client based movement.
//  BotEpidemic     - For keeping us up to date.
//  Telefragged.com - For giving ACE a home.
//  Microsoft       - For giving us such a wonderful crash free OS.
//  id              - Need I say more.
//  
//  And to all the other testers, pathers, and players and people
//  who I can't remember who the heck they were, but helped out.
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
//  acebot_items.c - This file contains all of the 
//                   item handling routines for the 
//                   ACE bot, including fact table support
//           
///////////////////////////////////////////////////////////////////////

#include "../g_local.h"
#include "acebot.h"

int	num_players = 0;
int num_items = 0;
item_table_t item_table[MAX_EDICTS];
edict_t *players[MAX_CLIENTS];		// pointers to all players in the game

//hypov8
//has item been picked up?
qboolean ACEIT_CheckIfItemExists(edict_t *self)
{
	int i;
	if (self->acebot.goal_node == INVALID)
		return true;
	if (nodes[self->acebot.goal_node].type != BOTNODE_ITEM)
		return true;
	if (self->acebot.bot_targetPlayerNode > level.framenum)
		return true; //hypov8 player.

	//add hypov8 chect if item is still valid
	for (i = 0; i < num_items; i++)	{
		if (item_table[i].node == self->acebot.goal_node){
			if (item_table[i].ent && !item_table[i].ent->solid)
				return false;
			return true; //found. still exist
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////
// Add the player to our list
///////////////////////////////////////////////////////////////////////
void ACEIT_PlayerAdded(edict_t *ent)
{
	if (!enable_bots) //disabled in comp.ini
		return;

	if (ent->client->pers.spectator != SPECTATING 
		&& (level.modeset == PUBLIC || level.modeset == PUBLICSPAWN
		|| level.modeset == MATCH || level.modeset == MATCHSPAWN))
		players[num_players++] = ent;
}


///////////////////////////////////////////////////////////////////////
// Remove player from list
///////////////////////////////////////////////////////////////////////
void ACEIT_PlayerRemoved(edict_t *ent)
{
	int i;
	int pos = -1;

	if (!enable_bots) //disabled in comp.ini
		return;

	// watch for 0 players
	if(num_players == 0)
		return;

	// special cas for only one player
	if(num_players == 1)
	{	
		num_players = 0;
		return;
	}

	// Find the player
	for (i = 0; i < num_players; i++)
	{
		if (ent == players[i]){
			pos = i;
			break; //found
		}
	}

	if (pos == -1) //invalid player
		return;

	// decrement
	for (i = pos; i < num_players - 1; i++)
	{
		players[i] = players[i + 1];
	}

	num_players--;
}

///////////////////////////////////////////////////////////////////////
// Can we get there?
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_IsReachable(edict_t *self, vec3_t goal)
{
	trace_t trace;
	vec3_t v, GoalCpy;

	VectorCopy(self->mins,v);
	VectorCopy(goal, GoalCpy);
	v[2] += 18; // Stepsize
	GoalCpy[2] += 9.5; //hypov8 move item off ground to match player height.
					//item_bbox=15 player_bbox=24 (9 differnce)

	trace = gi.trace(self->s.origin, v, self->maxs, GoalCpy, self, MASK_BOT_SOLID_FENCE);
	if (trace.fraction != 1.0f)
	{	//add hypov8 crouch?? kp 72=stand 48=crouch.
		VectorCopy(self->maxs,v);
		v[2] += -24; // bbox crouching height (DUCKING_MAX_Z = 24)
		trace = gi.trace(self->s.origin, self->mins, v, GoalCpy, self, MASK_BOT_SOLID_FENCE);
	}
	
	// Yes we can see it
	if (trace.fraction == 1.0)
		return true;
	else
		return false;

}

///////////////////////////////////////////////////////////////////////
// Visiblilty check 
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_IsVisible(edict_t *self, vec3_t goal)
{
	trace_t trace;
	
	trace = gi.trace(self->s.origin, vec3_origin, vec3_origin, goal, self, MASK_BOT_SOLID_FENCE);
	
	// Yes we can see it
	if (trace.fraction == 1.0)
		return true;
	else
		return false;

}

///////////////////////////////////////////////////////////////////////
//  Weapon changing support
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_ChangeWeapon (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;
		
	// see if we're already using it
	if (item == ent->client->pers.weapon)
		return true; 

	// Has not picked up weapon yet
	if(!ent->client->pers.inventory[ITEM_INDEX(item)])
		return false;

	// Do we have ammo for it?
	if (item->ammo)
	{
		ammo_item = FindItem(item->ammo);
		ammo_index = ITEM_INDEX(ammo_item);
		if (!ent->client->pers.inventory[ammo_index] && !g_select_empty->value)
			return false;
	}

	// Change to this weapon
	ent->client->newweapon = item;
	
	return true;
}


extern gitem_armor_t jacketarmor_info;
extern gitem_armor_t combatarmor_info;
extern gitem_armor_t bodyarmor_info;

///////////////////////////////////////////////////////////////////////
// Check if we can use the armor
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_CanUseArmor (gitem_t *item, edict_t *other)
{
	int				old_armor_index;
	gitem_armor_t	*oldinfo;
	gitem_armor_t	*newinfo;
	int				newcount;
	float			salvage;
	int				salvagecount;

	// get info on new armor
	newinfo = (gitem_armor_t *)item->info;

	old_armor_index = 0; // ArmorIndex(other); //hypov8 disabled

	// handle armor shards specially
	if (item->tag == ARMOR_SHARD)
		return true;
	
	// get info on old armor
	if (old_armor_index == ITEM_INDEX(FindItem("Jacket Armor")))
		oldinfo = &jacketarmor_info;
	else if (old_armor_index == ITEM_INDEX(FindItem("Combat Armor")))
		oldinfo = &combatarmor_info;
	else // (old_armor_index == body_armor_index)
		oldinfo = &bodyarmor_info;

	if (newinfo->normal_protection <= oldinfo->normal_protection)
	{
		// calc new armor values
		salvage = newinfo->normal_protection / oldinfo->normal_protection;
		salvagecount = salvage * newinfo->base_count;
		newcount = other->client->pers.inventory[old_armor_index] + salvagecount;

		if (newcount > oldinfo->max_count)
			newcount = oldinfo->max_count;

		// if we're already maxed out then we don't need the new armor
		if (other->client->pers.inventory[old_armor_index] >= newcount)
			return false;

	}

	return true;
}

//add hypov8
qboolean ACEIT_Needs_Armor(edict_t *bot, char * itemName)
{
	if (strcmp(itemName, "item_armor_helmet") == 0)
	{
		gitem_t *itemh = FindItem("Helmet Armor");
		gitem_t *itemhh = FindItem("Helmet Armor Heavy");

		if ((bot->client->pers.inventory[ITEM_INDEX(itemh)] > 80) ||
			(bot->client->pers.inventory[ITEM_INDEX(itemhh)] > 50))
			return false;

	}
	else if (strcmp(itemName, "item_armor_jacket") == 0)
	{
		gitem_t *itemj = FindItem("Jacket Armor");
		gitem_t *itemjh = FindItem("Jacket Armor Heavy");

		if ((bot->client->pers.inventory[ITEM_INDEX(itemj)] > 80) ||
			(bot->client->pers.inventory[ITEM_INDEX(itemjh)] > 50))
			return false;
	}
	else if (strcmp(itemName, "item_armor_legs") == 0)
	{
		gitem_t *iteml = FindItem("Legs Armor");
		gitem_t *itemlh = FindItem("Legs Armor Heavy");

		if ((bot->client->pers.inventory[ITEM_INDEX(iteml)] > 80) ||
			(bot->client->pers.inventory[ITEM_INDEX(itemlh)] >50))
			return false;

	}
	else if (strcmp(itemName, "item_armor_helmet_heavy") == 0)
	{
		gitem_t *itemhh = FindItem("Helmet Armor Heavy");

		if (bot->client->pers.inventory[ITEM_INDEX(itemhh)] > 50)
			return false;

	}
	else if (strcmp(itemName, "item_armor_jacket_heavy") == 0)
	{
		gitem_t *itemjh = FindItem("Jacket Armor Heavy");

		if (bot->client->pers.inventory[ITEM_INDEX(itemjh)] > 50)
			return false;
	}
	else if (strcmp(itemName, "item_armor_legs_heavy") == 0)
	{
		gitem_t *itemlh = FindItem("Legs Armor Heavy");

		if (bot->client->pers.inventory[ITEM_INDEX(itemlh)] > 50)
			return false;
	}
	return true;
}


///////////////////////////////////////////////////////////////////////
// Determins the NEED for an item
//
// This function can be modified to support new items to pick up
// Any other logic that needs to be added for custom decision making
// can be added here. For now it is very simple.
///////////////////////////////////////////////////////////////////////
float ACEIT_ItemNeed(edict_t *self, int item)
{
	// Make sure item is at least close to being valid
	if(item < 0 || item > 100)
		return 0.0;


	switch(item)
	{
		// Weapons

		// Ammo

		// Health
		case ITEMLIST_HEALTH_SMALL:	
		case ITEMLIST_HEALTH_LARGE:	
			if(self->health < 100)
				return 1.0 - (float)self->health/100.0f; // worse off, higher priority
			break;

		// Mods
		case ITEMLIST_PISTOLMODS2:		
		case ITEMLIST_PISTOLMODS3:		
		case ITEMLIST_PISTOLMODS4:	
		case ITEMLIST_HMGMODS:
			if (!self->client->pers.pistol_mods || ! self->client->pers.hmg_shots)
				return 0.3;
			break;

		// Armor
		case ITEMLIST_ARMORHELMET:
			if (ACEIT_Needs_Armor(self, "item_armor_helmet"))
				return  0.4; break;
		case ITEMLIST_ARMORJACKET:
			if (ACEIT_Needs_Armor(self, "item_armor_jacket"))
				return 0.4;	break;
		case ITEMLIST_ARMORLEGS:
			if (ACEIT_Needs_Armor(self, "item_armor_legs"))
				return 0.4;	break;

		case ITEMLIST_ARMORHELMETHEAVY:
			if (ACEIT_Needs_Armor(self, "item_armor_helmet_heavy"))
				return 0.8;	break;
		case ITEMLIST_ARMORJACKETHEAVY:
			if (ACEIT_Needs_Armor(self, "item_armor_jacket_heavy"))
				return 0.8;	break;
		case ITEMLIST_ARMORLEGSHEAVY:
			if (ACEIT_Needs_Armor(self, "item_armor_legs_heavy"))
				return 0.8;	break;

		//misc
		case ITEMLIST_TELEPORTER:
			return 0.1;
		/*case ITEMLIST_PACK: //hypov8 todo needed???
			if (self->client->pers.max_bullets < 300) //must not have pack yet. does not check low ammo..
				return 0.5; 
			else 
				return 0.0;*/


		default:
			return 0.0;	


			
	}
	return 0.0;		
}

///////////////////////////////////////////////////////////////////////
// Convert a classname to its index value
//
// I prefer to use integers/defines for simplicity sake. This routine
// can lead to some slowdowns I guess, but makes the rest of the code
// easier to deal with.
///////////////////////////////////////////////////////////////////////
int ACEIT_ClassnameToIndex(char *classname)
{
	if(strcmp(classname,"item_armor_helmet")==0) 
		return ITEMLIST_ARMORHELMET;
	
	if(strcmp(classname,"item_armor_jacket")==0)
		return ITEMLIST_ARMORJACKET;

	if(strcmp(classname,"item_armor_legs")==0)
		return ITEMLIST_ARMORLEGS;
	
	if(strcmp(classname,"item_armor_helmet_heavy")==0) 
		return ITEMLIST_ARMORHELMETHEAVY;
	
	if(strcmp(classname,"item_armor_jacket_heavy")==0)
		return ITEMLIST_ARMORJACKETHEAVY;

	if(strcmp(classname,"item_armor_legs_heavy")==0)
		return ITEMLIST_ARMORLEGSHEAVY;

	if(strcmp(classname,"item_health_sm")==0)
		return ITEMLIST_HEALTH_SMALL;

	if(strcmp(classname,"item_health_lg")==0)
		return ITEMLIST_HEALTH_LARGE;
	
	if(strcmp(classname,"item_pack")==0)
		return ITEMLIST_PACK;

	if(strcmp(classname,"item_adrenaline")==0)
		return ITEMLIST_ADRENALINE;

	if(strcmp(classname,"item_pistol_mods")==0)
		return ITEMLIST_PISTOLMODS;

	if(strcmp(classname,"pistol_mod_rof")==0)
		return ITEMLIST_PISTOLMODS2;

	if(strcmp(classname,"pistol_mod_reload")==0)
		return ITEMLIST_PISTOLMODS3;

	if(strcmp(classname,"pistol_mod_damage")==0)
		return ITEMLIST_PISTOLMODS4;

	if(strcmp(classname,"hmg_mod_cooling")==0)
		return ITEMLIST_HMGMODS;

	if (strcmp(classname, "misc_teleporter") == 0)
		return ITEMLIST_TELEPORTER;
		
	return INVALID;
}


///////////////////////////////////////////////////////////////////////
// Only called once per level, when saved will not be called again
//
// Downside of the routine is that items can not move about. If the level
// has been saved before and reloaded, it could cause a problem if there
// are items that spawn at random locations.
//
//#define DEBUG // uncomment to write out items to a file.
///////////////////////////////////////////////////////////////////////
void ACEIT_BuildItemNodeTable (qboolean rebuild)
{
	edict_t *items;
	int i,item_index;
	vec3_t v,v1,v2;

#ifdef DEBUG
	FILE *pOut; // for testing
	if((pOut = fopen("items.txt","wt"))==NULL)
		return;
#endif
	
	num_items = 0;

	// Add game items
	for(items = g_edicts; items < &g_edicts[globals.num_edicts]; items++)
	{
		// filter out crap
		if(items->solid == SOLID_NOT)
			continue;
		
		if(!items->classname)
			continue;
		
		/////////////////////////////////////////////////////////////////
		// Items
		/////////////////////////////////////////////////////////////////
		item_index = ACEIT_ClassnameToIndex(items->classname);
		
		////////////////////////////////////////////////////////////////
		// SPECIAL NAV NODE DROPPING CODE
		////////////////////////////////////////////////////////////////
		// Special node dropping for platforms
		if(strcmp(items->classname,"func_plat")==0)
		{
			if(!rebuild)
				/*ACEND_AddNode(items,BOTNODE_PLATFORM);*/
			{
				item_table[num_items].node = ACEND_AddNode(items, BOTNODE_PLATFORM);
				item_table[num_items].ent = items;
				item_table[num_items].item = 99;
				num_items++;
				continue; //add hypov8
			}
			item_index = 99; // to allow to pass the item index test
		}
		
		// Special node dropping for teleporters
		if(strcmp(items->classname,"misc_teleporter")==0)
		{
			if(!rebuild)
				/*ACEND_AddNode(items,BOTNODE_TELEPORTER);*/
			{
				item_table[num_items].node = ACEND_AddNode(items, BOTNODE_TELEPORTER );
				item_table[num_items].item = 99;
				num_items++;
				continue; //add hypov8
			}
			item_index = 99;
		}

		// Special node dropping for trigger_push
		if (strcmp(items->classname, "trigger_push") == 0)
		{
			if (!rebuild)
				ACEND_AddNode(items, BOTNODE_TRIGPUSH);
			continue; //just drop a node. not linked to an entity
		}
		
		#ifdef DEBUG
		if(item_index == INVALID)
			fprintf(pOut,"Rejected item: %s node: %d pos: %f %f %f\n",items->classname,item_table[num_items].node,items->s.origin[0],items->s.origin[1],items->s.origin[2]);
		else
			fprintf(pOut,"item: %s node: %d pos: %f %f %f\n",items->classname,item_table[num_items].node,items->s.origin[0],items->s.origin[1],items->s.origin[2]);
		#endif		
	
		if(item_index == INVALID)
			continue;

		// add a pointer to the item entity
		item_table[num_items].ent = items;
		item_table[num_items].item = item_index;
	
		// If new, add nodes for items
		if(!rebuild)
		{
			// Add a new node at the item's location.
			item_table[num_items].node = ACEND_AddNode(items,BOTNODE_ITEM);
			num_items++;
		}
		else // Now if rebuilding, just relink ent structures 
		{
			// Find stored location
			for(i=0;i<numnodes;i++)
			{
				if(nodes[i].type == BOTNODE_ITEM ||
				   nodes[i].type == BOTNODE_PLATFORM ||
				   nodes[i].type == BOTNODE_TELEPORTER) // valid types
				{
					VectorCopy(items->s.origin,v);
					
					if(nodes[i].type == BOTNODE_ITEM)
						v[2] += 16;
					
					if(nodes[i].type == BOTNODE_TELEPORTER)
						v[2] += 32;
					
					if(nodes[i].type == BOTNODE_PLATFORM)
					{
						VectorCopy(items->maxs,v1);
						VectorCopy(items->mins,v2);
		
						// To get the center
						v[0] = (v1[0] - v2[0]) / 2 + v2[0];
						v[1] = (v1[1] - v2[1]) / 2 + v2[1];
						//v[2] = items->mins[2] + 32; //was 64
						v[2] = items->maxs[2] + items->pos2[2] + 32;
					}

					if(v[0] == nodes[i].origin[0] &&
 					   v[1] == nodes[i].origin[1] &&
					   v[2] == nodes[i].origin[2])
					{
						// found a match now link to facts
						item_table[num_items].node = i;
#ifdef DEBUG
						fprintf(pOut,"Relink item: %s node: %d pos: %f %f %f\n",items->classname,item_table[num_items].node,items->s.origin[0],items->s.origin[1],items->s.origin[2]);
#endif							
						num_items++;
						break; //add hypov8. stop it serching for new items. will get stuck if item is at same origin
					}
				}
			}
		}
		

	}

#ifdef DEBUG
	fclose(pOut);
#endif

}
