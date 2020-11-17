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
//  acebot_ai.c -      This file contains all of the 
//                     AI routines for the ACE II bot.
//
//
// NOTE: I went back and pulled out most of the brains from
//       a number of these functions. They can be expanded on 
//       to provide a "higher" level of AI. 
////////////////////////////////////////////////////////////////////////

#include "../g_local.h"
#include "../m_player.h"

#include "acebot.h"

//add hypov8
//found an enemy, forget out goal
void ACEAI_ResetLRG (edict_t *self)
{
	if (self->acebot.goal_node != INVALID)
	{
		self->acebot.goal_node = INVALID;
		self->acebot.state = BOTSTATE_WANDER;
		self->acebot.wander_timeout = level.time + 0.5;
		if (debug_mode&& !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
			debug_printf("%s found enemy, remove LRG.\n", self->client->pers.netname);
	}
}

///////////////////////////////////////////////////////////////////////
// Main Think function for bot
///////////////////////////////////////////////////////////////////////
void ACEAI_Think (edict_t *self)
{
	usercmd_t	ucmd;

	VectorCopy(self->s.angles, self->acebot.botDeathAngles);

	// Set up client movement
	VectorCopy(self->client->ps.viewangles,self->s.angles);
	VectorSet (self->client->ps.pmove.delta_angles, 0, 0, 0);
	memset (&ucmd, 0, sizeof (ucmd));
	self->enemy = NULL;
	self->movetarget = NULL;
	self->client->resp.check_idle = level.framenum; //hypov8 just incase	

	//if (level.modeset != MATCH && level.modeset != PUBLIC)
	if (level.modeset == ENDGAMEVOTE || level.modeset == ENDGAME)	{
		ClientThink(self, &ucmd);
		self->nextthink = level.time + 999;
		return;
	}

	// Force respawn 
	if (self->deadflag)
	{
		self->client->buttons = 0;
		ucmd.buttons = BUTTON_ATTACK;
	}
	
	if (self->acebot.state == BOTSTATE_WANDER && self->acebot.wander_timeout < level.time)
	  ACEAI_PickLongRangeGoal(self); // pick a new long range goal

	// Kill the bot if completely stuck somewhere
	self->acebot.botMoveDirVel = VectorDistance(self->acebot.botOldOrigin, self->s.origin);	// setup velocity for timeouts
#ifndef HYPODEBUG //stop bot timeout
	if (self->acebot.botMoveDirVel > 3) //if(VectorLength(self->velocity) > 37) //
#endif
		self->acebot.suicide_timeout = level.time + 12.0;

	if (self->acebot.suicide_timeout < level.time)
	{
		self->flags &= ~FL_GODMODE;
		self->health = 0;
		meansOfDeath = MOD_BOT_SUICIDE;	//hypov8 added. shown as player killed them selves now
		VectorSet(self->velocity, 0, 0, 0);	//hypo stop movement
		player_die (self, self, self, 1, vec3_origin,0,0);
	}
	
	// Find any short range goal
	ACEAI_PickShortRangeGoal(self);
	
	// Look for enemies
	if(ACEAI_FindEnemy(self))
	{	
		ACEAI_ResetLRG(self); //attack instead of going to goal
		ACEMV_Attack (self, &ucmd);
	}
	else
	{
		// Execute the move, or wander
		if (self->acebot.state == BOTSTATE_WANDER)
			ACEMV_Wander(self,&ucmd);
		else if (self->acebot.state == BOTSTATE_MOVE)
			ACEMV_Move(self,&ucmd);
	}
	
	//debug_printf("State: %d\n",self->acebot.state);

	// set approximate ping
	ucmd.msec = 100; // 75 + floor(random() * 25) + 1;

	// show random ping values in scoreboard
	self->client->ping = 0; // ucmd.msec;

	//hypo set old origin for movement calculations
	VectorCopy(self->s.origin, self->acebot.botOldOrigin);

	// set bot's view angle
	ucmd.angles[PITCH] = ANGLE2SHORT(self->s.angles[PITCH]);
	ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);
	ucmd.angles[ROLL] = ANGLE2SHORT(self->s.angles[ROLL]);
	
	// send command through id's code
	ClientThink (self, &ucmd);
	
	self->nextthink = level.time + FRAMETIME/2; //only run every 0.1 sec anyway (runframe())
}

///////////////////////////////////////////////////////////////////////
// Evaluate the best long range goal and send the bot on
// its way. This is a good time waster, so use it sparingly. 
// Do not call it for every think cycle.
///////////////////////////////////////////////////////////////////////
void ACEAI_PickLongRangeGoal(edict_t *self)
{

	int i;
	int node;
	float weight,best_weight=0.0;
	int current_node,goal_node;
	edict_t *goal_ent;
	int cost;

	if (self->acebot.bot_isPushed)
		return;
	
	// look for a target 
	current_node = ACEND_FindClosestReachableNode(self,BOTNODE_DENSITY,BOTNODE_ALL);

	self->acebot.current_node = current_node;
	
	if(current_node == -1)
	{
		self->acebot.state = BOTSTATE_WANDER;
		self->acebot.wander_timeout = level.time + 1.0;
		self->acebot.goal_node = -1;
		return;
	}

	///////////////////////////////////////////////////////
	// Items
	///////////////////////////////////////////////////////
	for(i=0;i<num_items;i++)
	{
		if (item_table[i].ent == NULL)
			continue;
		if (!item_table[i].ent->solid) // ignore items that are not there.
			continue;
		
		cost = ACEND_FindCost((short)current_node, (short)item_table[i].node);
		
		if(cost == INVALID || cost < 2) // ignore invalid and very short hops
			continue;
	
		weight = ACEIT_ItemNeed(self, item_table[i].item);
		weight *= random(); // Allow random variations
		weight /= (float)cost; // Check against cost of getting there
				
		if(weight > best_weight)
		{
			best_weight = weight;
			goal_node = item_table[i].node;
			goal_ent = item_table[i].ent;
		}
	}

	self->acebot.bot_targetPlayerNode = 0;//add hypov8
	///////////////////////////////////////////////////////
	// Players
	///////////////////////////////////////////////////////
	// This should be its own function and is for now just
	// finds a player to set as the goal.
	for(i=0;i<num_players;i++)
	{
		if (players[i] == self
			|| players[i]->solid == SOLID_NOT
			|| players[i]->movetype == MOVETYPE_NOCLIP
			|| players[i]->flags & FL_GODMODE
			|| players[i]->client->invincible_framenum > level.framenum)
			continue;

		node = ACEND_FindClosestNode(players[i],BOTNODE_DENSITY,BOTNODE_ALL);
		cost = ACEND_FindCost((short)current_node, (short)node);

		if(cost == INVALID || cost < 3) // ignore invalid and very short hops
			continue;
/*
		// Player carrying the flag?
		if(ctf->value && (players[i]->client->pers.inventory[ITEMLIST_FLAG2] || players[i]->client->pers.inventory[ITEMLIST_FLAG1]))
		  weight = 2.0;
		else*/
		  weight = 0.3; 
		
		weight *= random(); // Allow random variations
		weight /= cost; // Check against cost of getting there
		
		if(weight > best_weight)
		{		
			self->acebot.bot_targetPlayerNode = level.framenum + 100; //add hypov8 +10 sec timeout
			best_weight = weight;
			goal_node = node;
			goal_ent = players[i];
		}	
	}

	// If do not find a goal, go wandering....
	if(best_weight == 0.0 || goal_node == INVALID)
	{
		self->acebot.goal_node = INVALID;
		self->acebot.state = BOTSTATE_WANDER;
		self->acebot.wander_timeout = level.time + 1.0;
		if (debug_mode&& !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
			debug_printf("%s did not find a LR goal, wandering.\n",self->client->pers.netname);
		return; // no path? 
	}
	
	// OK, everything valid, let's start moving to our goal.
	self->acebot.state = BOTSTATE_MOVE;
	self->acebot.tries = 0; // Reset the count of how many times we tried this goal
	 
	if (goal_ent != NULL && debug_mode && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
		debug_printf("%s selected a \"%s\" at node %d for LR goal.\n",self->client->pers.netname, goal_ent->classname, goal_node);

	ACEND_SetGoal(self,goal_node);

}

///////////////////////////////////////////////////////////////////////
// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
///////////////////////////////////////////////////////////////////////
void ACEAI_PickShortRangeGoal(edict_t *self)
{
	edict_t *target;
	float weight,best_weight=0.0;
	edict_t *best;
	int index;
	
	// look for a target (should make more efficent later)
	target = findradius(NULL, self->s.origin, 200);
	
	while(target)
	{
		if(target->classname == NULL)
			return;
		
		// Missle avoidance code
		// Set our movetarget to be the rocket or grenade fired at us. 
		if(strcmp(target->classname,"rocket")==0 || strcmp(target->classname,"grenade")==0)
		{
			if(debug_mode) 
				debug_printf("ROCKET ALERT!\n");

			self->movetarget = target;
			return;
		}

		//add hypov8 skip invalid stuff
		if (!(strcmp(target->classname, "worldspawn") == 0 || strcmp(target->classname, "mdx_bbox") == 0
			||strcmp(target->classname, "bodyque") == 0 || strcmp(target->classname, "player") == 0
			|| strcmp(target->classname, "bot") == 0))
		{

			if (ACEIT_IsReachable(self, target->s.origin))
			{
				if (infront(self, target))
				{
					index = ACEIT_ClassnameToIndex(target->classname);
					weight = ACEIT_ItemNeed(self, index);

					if (weight > best_weight)
					{
						best_weight = weight;
						best = target;
					}
				}
			}
		}

		// next target
		target = findradius(target, self->s.origin, 200);
	}

	if(best_weight)
	{
		self->movetarget = best;
		
		if (debug_mode && self->goalentity != self->movetarget&& !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
			debug_printf("%s selected a %s for SR goal.\n",self->client->pers.netname, self->movetarget->classname);
		
		self->goalentity = best;

	}

}



/*
=============
infrontEnem

returns 1 if other is in front (in sight) of self
hypov8 look more to side for items was 0.2
0 must be 90 deg???
=============
*/
static qboolean ACEAI_InfrontEnemy(edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot, fov = 0.55;
	vec3_t	forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);
	//hypov8 increase fov over 3.0
	if (sv_botskill->value >= 3.0f)
	fov = (4 - sv_botskill->value) - 1; //0 to -1
	//1.0 is dead ahead.
	//-1.0 180 deg. 
	//0.5 45 deg (fov 90)

	if (dot > fov)
		return true;
	return false;
}


//add hpov8 bigger sight range needed(stop shooting through tiny gaps or soon as they get around a corner
qboolean ACEAI_VisiblePlayer(edict_t *self, edict_t *other)
{
	vec3_t min = {-8,-8,0};
	vec3_t max = {8,8,0};
	vec3_t	spot1;
	vec3_t	spot2;
	trace_t	trace;
	qboolean underAttack = false;

	//hypov8 shoot enemy faster if being attacked
	if (self->acebot.lastDamageTimer > (level.framenum - 40) && self->acebot.lastDamageTimer < (level.framenum - 10))
		if (self->acebot.lastDamagePlayerIndex == other->s.number)
			underAttack = true;

	VectorCopy(self->s.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy(other->s.origin, spot2);
	spot2[2] += other->viewheight;
	trace = gi.trace(spot1, min, max, spot2, self, MASK_BOT_SOLID_FENCE);

	if (trace.fraction == 1.0)
		//if (underAttack || ACEAI_InfrontEnemy(self, other))
		return true;

	return false;
}

///////////////////////////////////////////////////////////////////////
// Scan for enemy (simplifed for now to just pick any visible enemy)
// hypov8 pic closest enemy
///////////////////////////////////////////////////////////////////////
qboolean ACEAI_FindEnemy(edict_t *self)
{
	int i;
	int	j = -1;
	float range, range_tmp = 0;
	qboolean underAttack = false;

	//add hypov8 stop bot shooting on ladders
	if ((self->acebot.next_node != INVALID && nodes[self->acebot.next_node].type == BOTNODE_LADDER)
		|| (self->acebot.current_node != INVALID && nodes[self->acebot.current_node].type == BOTNODE_LADDER))
		return false;
	if (self->acebot.botNewTargetTime > level.time) //hypo add timer for bot to not own so much
		return false;
	if (self->health < 1)
		return false;

	//15 seconds, forget about old target
	if (self->acebot.old_targetTimer < level.framenum - 150)
		self->acebot.old_target = -1;

	//hypov8 shoot enemy faster if being attacked
	if (self->acebot.lastDamageTimer >= (level.framenum - 40)) //4 seconds. allow for skill
		underAttack = true;

	if (sv_botskill->value < 0.0f) 
		sv_botskill = gi.cvar_set("sv_botskill", "0.0");
	if (sv_botskill->value > 4.0f) 
		sv_botskill = gi.cvar_set("sv_botskill", "4.0");


	
	for (i = 0; i < num_players; i++)
	{
		if(players[i] == NULL || players[i] == self 
			|| players[i]->flags & FL_GODMODE //add hypov8. allow debug without being attacked
			|| players[i]->solid == SOLID_NOT)
		   continue;

		if (self->acebot.old_target == i && players[i]->health < 1)	{
			self->acebot.old_target = -1;
			continue;
		}

		//immortal
		if (players[i]->client->invincible_framenum > level.framenum)
			continue;
		//spec
		if (players[i]->client->pers.spectator == SPECTATING || players[i]->movetype != MOVETYPE_WALK)
			continue;

		if(!players[i]->deadflag 
			&& (ACEAI_VisiblePlayer(self, players[i])) 
			&&	gi.inPVS (self->s.origin, players[i]->s.origin))
		{
			// Base enemy selection on distance.		
			range = VectorDistance(self->s.origin, players[i]->s.origin);

			//make them a closer enemy
			if (underAttack && self->acebot.lastDamagePlayerIndex == players[i]->s.number)
				range *= .4;

			if (self->acebot.old_target == i){
				self->enemy = players[i];
				range *= .2;
			}
			else
				if (!underAttack && !ACEAI_InfrontEnemy(self, players[i]))
					continue;

			if (range_tmp) {	
				if (range < range_tmp)
					j = i; //j is new closer player
			}
			else {
				range_tmp = range;
				j = i;
			}
		}
	}

	// set a new target (closer)
	if (j != -1)	
	{
		self->acebot.new_target = j;
		self->acebot.old_targetTimer = level.framenum;

		if (self->acebot.new_target != self->acebot.old_target)
		{
			self->acebot.old_target = j;
			if (self->client->weaponstate == WEAPON_FIRING||underAttack)
				self->acebot.botNewTargetTime = level.time + ((4 - sv_botskill->value) / 4);//hypov8 target changed. shoot next closer player sooner
			else
				self->acebot.botNewTargetTime = level.time + (4 - sv_botskill->value) /* 2*/ ; //give player 2 seconds(default) leway between seeing n being shot

			return false; //self->client->weaponstate == WEAPON_FIRING;
		}

		self->enemy = players[j];
		return true;
	}

	return false;
}
