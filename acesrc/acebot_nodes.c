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
//  acebot_nodes.c -   This file contains all of the 
//                     pathing routines for the ACE bot.
// 
///////////////////////////////////////////////////////////////////////

#include "../g_local.h"
#include "acebot.h"

// flags
qboolean newmap=true;

// Total number of nodes that are items
//int numitemnodes;  //hypov8 unused, now "num_items" ??

// Total number of nodes
int numnodes; 

#define NODE0 ((short)0)

// For debugging paths
int show_path_from = -1;
int show_path_to = -1;

// array for node data
botnode_t nodes[MAX_BOTNODES]; 
short path_table[MAX_BOTNODES][MAX_BOTNODES];

///////////////////////////////////////////////////////////////////////
// NODE INFORMATION FUNCTIONS
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Determin cost of moving from one node to another
///////////////////////////////////////////////////////////////////////
int ACEND_FindCost(short from, short to)
{
	short curnode;
	int cost=1; // Shortest possible is 1

	// If we can not get there then return invalid
	if (path_table[from][to] == INVALID)
		return INVALID;

	// Otherwise check the path and return the cost
	curnode = path_table[from][to];

	// Find a path (linear time, very fast)
	while(curnode != to)
	{
		curnode = path_table[curnode][to];
		if(curnode == INVALID) // something has corrupted the path abort
			return INVALID;
		cost++;

		if (cost >= (int)numnodes) // add hypov8. something has corrupted the path abort 999?
			return INVALID;
	}
	
	return cost;
}

// hypov8 add 
float VectorDistanceFlat(vec3_t vec1, vec3_t vec2)
{
	vec3_t	vec, vec1_, vec2_;

	VectorCopy(vec1, vec1_);
	VectorCopy(vec2, vec2_);
	vec1_[2] = vec2_[2] = 0; //hypo remove height from distance

	VectorSubtract(vec1, vec2, vec);
	return VectorLength(vec);
}

//add hypov8 makes node level with player
void ACEND_MoveToPLayerHeight(int nodeIndex, vec3_t out)
{
	vec3_t origin;
	int type =	nodes[nodeIndex].type;
	VectorCopy(nodes[nodeIndex].origin , origin);

	switch (type)
	{
		case BOTNODE_MOVE:			origin[2] -= 8; break; //8
		case BOTNODE_LADDER:		origin[2] -= 8; break; //8
		case BOTNODE_PLATFORM:		origin[2] -= 8; break; //32
		case BOTNODE_TELEPORTER:	origin[2] -= 8; break; //32
		case BOTNODE_ITEM:			origin[2] -= 8; break; //16
		default:
		case BOTNODE_WATER:		/*origin[2] -= 8;*/ break; //0
		case BOTNODE_GRAPPLE: 	/*origin[2] -= 8;*/ break; //0 //BEGIN HITMEN
		case BOTNODE_JUMP:			origin[2] -= 8; break; //8
	}

	VectorCopy(origin, out);
}

///////////////////////////////////////////////////////////////////////
// Find the closest node to the player within a certain range
///////////////////////////////////////////////////////////////////////
int ACEND_FindClosestReachableNode(edict_t *self, int range, int type)
{
	int i;
	float closest = 99999;
	float dist;
	int node=-1;
	vec3_t v;
	trace_t tr;
	float rng;
	vec3_t maxs,mins;

	VectorCopy(self->mins,mins);
	VectorCopy(self->maxs,maxs);
	mins[0] = mins[1] = -15; //item bbox size. player hits wall
	maxs[0] = maxs[1] = 15;
	
	// For Ladders, do not worry so much about reachability
	if(type == BOTNODE_LADDER)
	{
		VectorCopy(vec3_origin,maxs);
		VectorCopy(vec3_origin,mins);
	}
	else
		mins[2] += 18; // Stepsize

	rng = (float)(range * range); // square range for distance comparison (eliminate sqrt)	
	
	for(i=0;i<numnodes;i++)
	{		
		if(type == BOTNODE_ALL || type == nodes[i].type) // check node type
		{
			VectorSubtract(nodes[i].origin, self->s.origin,v); // subtract first
			dist = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
		
			if(dist < closest && dist < rng) 
			{
				// make sure it is visible
				tr = gi.trace(self->s.origin, mins, maxs, nodes[i].origin, self, MASK_BOT_SOLID_FENCE);
				if(tr.fraction == 1.0)
				{
					node = i;
					closest = dist;
				}
				else //hypov8 crouch? simple check. flat ground.
				{
					vec3_t maxs_c, mins_c;
					vec3_t movedDown;
					VectorCopy(self->maxs, maxs_c);
					VectorCopy(self->mins, mins_c);
					maxs_c[2] = 24; //crouch //-24

					ACEND_MoveToPLayerHeight(i, movedDown);

					// visible crouching?
					tr = gi.trace(self->s.origin, mins_c, maxs_c, movedDown, self,  MASK_BOT_SOLID_FENCE);
					if (tr.fraction == 1.0)
					{
						node = i;
						closest = dist;
					}
					else if (self->groundentity && self->groundentity->use != Use_Plat 
						&& nodes[i].origin[2] + 18 > self->s.origin[0]) //hypov8 jump?
					{
						vec3_t  headHeight;
						VectorCopy(self->s.origin, headHeight);
						headHeight[2] += 60; //jump		

						// make sure it is visible
						tr = gi.trace(headHeight, self->mins, self->maxs, movedDown, self, MASK_BOT_SOLID_FENCE);
						if (tr.fraction == 1.0)
						{
							node = i;
							closest = dist;
						}
					}
				}
			}
		}
	}
	
	return node;
}


///////////////////////////////////////////////////////////////////////
// Find the closest node to the player within a certain range
// hypov8 also used in localnode.
// does not check trace
///////////////////////////////////////////////////////////////////////
short ACEND_FindClosestNode(edict_t *self, int range, short type)
{
	short i;
	float closest = 99999;
	float dist;
	short node = INVALID;
	vec3_t v;
	float rng;

	rng = (float)(range * range); // square range for distance comparison (eliminate sqrt)	

	for (i = NODE0; i<numnodes; i++)
	{
		if (type == BOTNODE_ALL || type == nodes[i].type) // check node type
		{
			VectorSubtract(nodes[i].origin, self->s.origin, v); // subtract first

			dist = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

			if (dist < closest && dist < rng)
			{
				node = i;
				closest = dist;
			}
		}
	}

	return node;
}

///////////////////////////////////////////////////////////////////////
// BOT NAVIGATION ROUTINES
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Set up the goal
///////////////////////////////////////////////////////////////////////
void ACEND_SetGoal(edict_t *self, int goal_node)
{
	int node;

	self->acebot.goal_node = goal_node;
	node = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY*3, BOTNODE_ALL);
	
	if(node == -1)
		return;
	
	if (debug_mode && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
		debug_printf("%s new start node selected %d\n", self->client->pers.netname, node);
	
	
	self->acebot.current_node = node;
	self->acebot.next_node = self->acebot.current_node; // make sure we get to the nearest node first
	self->acebot.node_timeout = 0;

}

///////////////////////////////////////////////////////////////////////
// Move closer to goal by pointing the bot to the next node
// that is closer to the goal
///////////////////////////////////////////////////////////////////////
qboolean ACEND_FollowPath(edict_t *self)
{
	vec3_t v;
	qboolean isPlatTop = false;
	
#if 1 //def HYPODEBUG //defined in project DEBUG
	//////////////////////////////////////////
	// Show the path (uncomment for debugging)
	if (debug_mode && !debug_mode_origin_ents) //hypov8 disable path lines, to many overflows
	{
		show_path_from = self->acebot.current_node;
		show_path_to = self->acebot.goal_node;
		ACEND_DrawPath();
	}
	//////////////////////////////////////////
#endif

	if (!ACEIT_CheckIfItemExists(self))
		return false;
	//add hypov8 chect if item is still valid

	// Try again?
	if (self->acebot.node_timeout++ > 30)
	{
		if (self->acebot.tries++ > 3)
			return false;
		else
			ACEND_SetGoal(self, self->acebot.goal_node);
	}

	//hypov8 check plate. fix for plate with hand rails
	if (nodes[self->acebot.current_node].type == BOTNODE_PLATFORM)	{
		int i;
		for (i = 0; i < num_items; i++)		{
			if (item_table[i].node == self->acebot.current_node)			{
				if (item_table[i].ent && item_table[i].ent->moveinfo.state == STATE_TOP)
					isPlatTop = true;
				break; //found.
			}
		}
	}

	// Are we there yet? //hypov8 check plate
	VectorSubtract(self->s.origin, nodes[self->acebot.next_node].origin, v);
	if (VectorLength(v) < 32 || isPlatTop)
	{
		// reset timeout
		self->acebot.node_timeout = 0;

		if (self->acebot.next_node == self->acebot.goal_node)
		{
			if (debug_mode && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on 
				debug_printf("%s reached goal!\n",self->client->pers.netname);	
			
			ACEAI_PickLongRangeGoal(self); // Pick a new goal
		}
		else
		{
			self->acebot.current_node = self->acebot.next_node;
			self->acebot.next_node = path_table[self->acebot.current_node][self->acebot.goal_node];
		}
	}
	
	if (self->acebot.current_node == -1 || self->acebot.next_node == -1)
		return false;

	// Set bot's movement vector
	{
		vec3_t lookTarget;
		//hypove bring bots aim down to match original node height
		VectorCopy(nodes[self->acebot.next_node].origin, lookTarget);
		if (nodes[self->acebot.next_node].type == BOTNODE_MOVE) lookTarget[2] -= 8;
		if (nodes[self->acebot.next_node].type == BOTNODE_ITEM) lookTarget[2] -= 16;
		if (nodes[self->acebot.next_node].type == BOTNODE_TELEPORTER) lookTarget[2] -= 32;
		if (nodes[self->acebot.next_node].type == BOTNODE_PLATFORM) lookTarget[2] -= 32;
		if (nodes[self->acebot.next_node].type == BOTNODE_JUMP) lookTarget[2] += 32;

		VectorSubtract(lookTarget, self->s.origin, self->acebot.move_vector);
	}
	return true;
}


///////////////////////////////////////////////////////////////////////
// MAPPING CODE
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
// Check for adding ladder nodes
///////////////////////////////////////////////////////////////////////

static qboolean ACEND_CheckForLadder(edict_t *self)
{
	short closest_node;
	trace_t tr;
	vec3_t ladderMin = { -16, -16, 16 }; //hypov8 dont add nodes above the top of ladder
	vec3_t ladderMax = { 16, 16, 48 };

	if (self->client->ps.pmove.pm_type != PM_NORMAL || self->movetype == MOVETYPE_NOCLIP)
		return false;

	if (!self->groundentity && self->velocity[2] > 20)
	{
		vec3_t dir, forward, right, focalpoint;
		VectorCopy(self->s.angles, dir);
		dir [0]= dir[2] = 0;
		AngleVectors(dir, forward, right, NULL);
		VectorMA(self->s.origin, 1, forward, focalpoint);
		tr = gi.trace(self->s.origin, ladderMin, ladderMax, focalpoint, self, MASK_ALL); //add hypov8 ladder fix //BOTNODE_LADDER

		// If there is a ladder and we are moving up, see if we should add a ladder node
		if (tr.contents & CONTENTS_LADDER)
		{
			closest_node = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY_HALVE, BOTNODE_LADDER);
			if (closest_node == INVALID)
				closest_node = ACEND_AddNode(self, BOTNODE_LADDER);

			if (self->acebot.pm_last_node != INVALID && nodes[self->acebot.pm_last_node].origin[2] < nodes[closest_node].origin[2]) //make sure its above us
				ACEND_UpdateNodeEdge(self->acebot.pm_last_node, closest_node, true, true, true, false);

			self->acebot.pm_last_node = closest_node; // set visited to last
			return true;
		}
	}

	//add node to top of ladder
	if (self->acebot.pm_last_node != INVALID && nodes[self->acebot.pm_last_node].type == BOTNODE_LADDER
		&& nodes[self->acebot.pm_last_node].origin[2] < self->s.origin[2]
		&& self->groundentity)
	{
		float distToTarget = VectorDistanceFlat(self->s.origin, nodes[self->acebot.pm_last_node].origin);

		//make sure last node is close to us
		if (distToTarget < 42)
		{	
			closest_node = ACEND_FindClosestNode(self, BOTNODE_DENSITY_THIRD, BOTNODE_LADDER); //add hypov8
			//closest_node = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY_HALVE, BOTNODE_ALL);
			if (closest_node == INVALID)
				closest_node = ACEND_AddNode(self, BOTNODE_LADDER);

			ACEND_UpdateNodeEdge(self->acebot.pm_last_node, closest_node, true, true, false, false);
			self->acebot.pm_last_node = closest_node; // set visited to last
			return true;
		}
	}
	return false;
}

static qboolean ACEND_PathMapIsFirstPlayer(edict_t *self)
{
	edict_t *dood;
	int i;
	for_each_player_not_bot(dood, i)
	{
		if (dood->inuse 
			&& !dood->acebot.is_bot
			&& self->solid
			&& dood->client 
			&& dood->client->pers.spectator == PLAYING
			&& self->movetype == MOVETYPE_WALK)
		{
			if (self == dood)
				return true;

			break; //only use player#1
		}
	}
	return false;

}

///////////////////////////////////////////////////////////////////////
// This routine is called to hook in the pathing code and sets
// the current node if valid.
///////////////////////////////////////////////////////////////////////
void ACEND_PathMap(edict_t *self)
{ 
	int closest_node;
	static float last_update = 0; // start off low
	vec3_t v;

	if (!enable_bots) //disabled in comp.ini
		return;
	if (!sv_botpath->value)
		return;
	if (self->acebot.is_bot)
		return;
	if (!ACEND_PathMapIsFirstPlayer(self)) //add hypov8 only rout on 1 player
		return;

	//add hypov8 check jumping every frame.
	//invalidate last node if to far?
	if (self->client->ps.pmove.pm_flags & PMF_JUMP_HELD)
	{
		//are we starting to jump.
		if (!self->groundentity	&& self->acebot.bot_pm_playerJumpTime < level.framenum
			&&	self->velocity[2] > 100 && self->acebot.pm_last_node != INVALID
			&& (nodes[self->acebot.pm_last_node].type == BOTNODE_MOVE
			|| nodes[self->acebot.pm_last_node].type == BOTNODE_ITEM
			|| nodes[self->acebot.pm_last_node].type == BOTNODE_JUMP))
		{
			float lastNodeHeight = nodes[self->acebot.pm_last_node].origin[2] - 8;
			float distToTarget = VectorDistanceFlat(self->s.origin, nodes[self->acebot.pm_last_node].origin);

			//make sure last node is lower and close enough to us
			if (self->s.origin[2] > lastNodeHeight && distToTarget < 64)
				self->acebot.bot_pm_playerJumpTime = level.framenum + 41; //4 secs to do a normal jump
			else
				self->acebot.pm_last_node = INVALID;
		}
	}
	//catch quick jumps. touching ground
	if (self->acebot.bot_pm_playerJumpTime >= level.framenum && level.time < last_update && self->groundentity)
	{
		last_update = 0; //hypov8 run checks
	}


	//add hypov8 reset path update time on "changemap" cmd
	if (level.time <= 5.0f || !(level.modeset == MATCH || level.modeset == PUBLIC))
	{
		last_update = 0;
		return;
	}

	if ( self->deadflag
		|| self->client->pers.spectator
		|| self->solid == SOLID_NOT
		//|| !sv_botpath->value
		|| self->movetype != MOVETYPE_WALK) //noclip
		return;	

	if(level.time < last_update)
		return;

	last_update = level.time + 0.10; // slow down updates a bit //hypov8 was 0.15

	// Special node drawing code for debugging
    if(show_path_to != -1)
		ACEND_DrawPath();
	
	////////////////////////////////////////////////////////
	// Special check for ladder nodes
	///////////////////////////////////////////////////////
	if(ACEND_CheckForLadder(self)) // check for ladder nodes
		return;

	// Not on ground, and not in the water, so bail
    if(!self->groundentity && !self->waterlevel)
		return;

	////////////////////////////////////////////////////////
	// Lava/Slime
	////////////////////////////////////////////////////////
	VectorCopy(self->s.origin,v);
	v[2] -= 18;
	if(gi.pointcontents(v) & (CONTENTS_LAVA|CONTENTS_SLIME))
		return; // no nodes in slime
	
    ////////////////////////////////////////////////////////
	// Jumping
	///////////////////////////////////////////////////////
	if (self->acebot.bot_pm_playerJumpTime >= level.framenum)
	{	
		if (self->acebot.pm_last_node != INVALID
			&& (nodes[self->acebot.pm_last_node].type == BOTNODE_MOVE
			|| nodes[self->acebot.pm_last_node].type == BOTNODE_ITEM
			|| nodes[self->acebot.pm_last_node].type == BOTNODE_JUMP))
		{
			float distToNode = VectorDistanceFlat(self->s.origin, nodes[self->acebot.pm_last_node].origin);

			//hypov8 allow jump->jump up crates. stop node linking down jumpnode if too close
			if (distToNode > 64
				|| (distToNode > 30 && nodes[self->acebot.pm_last_node].type != BOTNODE_JUMP)
				|| (nodes[self->acebot.pm_last_node].type == BOTNODE_JUMP && nodes[self->acebot.pm_last_node].origin[2] < self->s.origin[2]))
			{ 
				closest_node = ACEND_FindClosestReachableNode(self, 64, BOTNODE_JUMP); //check jump to item
				if (closest_node != INVALID && self->acebot.pm_last_node != INVALID)
				{
					ACEND_UpdateNodeEdge(self->acebot.pm_last_node, closest_node, false, true, true, false);
					self->acebot.pm_last_node = closest_node; // set visited to last
				}
				else //jump failed, set last node.
					self->acebot.pm_last_node = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY, BOTNODE_ALL);
			}
			else //jump failed, set last node.
				self->acebot.pm_last_node = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY, BOTNODE_ALL);
		}
		self->acebot.bot_pm_playerJumpTime = 0;
		return;
	}

	//check jump pads
	if (self->acebot.bot_pm_jumpPadMove&& self->acebot.bot_pm_jumpPadMove < level.framenum)
		ACEND_PathToTrigPushDest(self);
/*
	////////////////////////////////////////////////////////////
	// Grapple
	// Do not add nodes during grapple, added elsewhere manually
	////////////////////////////////////////////////////////////
	if(ctf->value && self->client->ctf_grapplestate == CTF_GRAPPLE_STATE_PULL)
		return;*/
	 
	// Iterate through all nodes to make sure far enough apart
	closest_node = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY, BOTNODE_ALL);

	////////////////////////////////////////////////////////
	// Special Check for Platforms
	////////////////////////////////////////////////////////
	if(self->groundentity && self->groundentity->use == Use_Plat)
	{
		float dist = (self->groundentity->moveinfo.start_origin[2] - self->groundentity->moveinfo.end_origin[2])/2;

		if(closest_node == INVALID)
			return; // Do not want to do anything here.

		//hypov8 
		if (self->groundentity->moveinfo.state == STATE_BOTTOM 
			|| self->groundentity->moveinfo.remaining_distance > dist+8)
		{
			closest_node = ACEND_FindClosestNode(self, BOTNODE_DENSITY, BOTNODE_PLATFORM);
			ACEND_UpdateNodeEdge(self->acebot.pm_last_node, closest_node, true, true, true, false);
		}
		else //must be past halve way
		{
			closest_node = ACEND_FindClosestNode(self, BOTNODE_DENSITY, BOTNODE_PLATFORM);
			if (nodes[self->acebot.pm_last_node].type == BOTNODE_PLATFORM) //only update if previous node was a plat
				ACEND_UpdateNodeEdge(self->acebot.pm_last_node, closest_node, true, true, false, false);
		} 
		self->acebot.pm_last_node = closest_node;

		return;
	}
	 
	 ////////////////////////////////////////////////////////
	 // Add Nodes as needed
	 ////////////////////////////////////////////////////////
	 if(closest_node == INVALID)
	 {
		 // Add nodes in the water as needed
		 if (self->waterlevel)
			 closest_node = ACEND_AddNode(self, BOTNODE_WATER);
		 else
		 {	//double check water
			 if (gi.pointcontents(self->s.origin) & MASK_WATER) //add hypov8 is 24 deep??
				 closest_node = ACEND_AddNode(self, BOTNODE_WATER);
			 else
				 closest_node = ACEND_AddNode(self, BOTNODE_MOVE);
		 }
		 ACEND_UpdateNodeEdge(self->acebot.pm_last_node, closest_node, true, true, true, false);
	 }
	 else if (closest_node != self->acebot.pm_last_node && self->acebot.pm_last_node != INVALID)
	 {
		 float distToItem = VectorDistanceFlat(self->s.origin, nodes[closest_node].origin);

		 if (distToItem < BOTNODE_DENSITY)
			 ACEND_UpdateNodeEdge(self->acebot.pm_last_node, closest_node, true, true, true, false);
		 else if (debug_mode)
			 debug_printf("Node not linked, to far. %f\n", distToItem);
	 }
	 self->acebot.pm_last_node = closest_node; // set visited to last
	
}




///////////////////////////////////////////////////////////////////////
// Init node array (set all to INVALID)
///////////////////////////////////////////////////////////////////////
void ACEND_InitNodes(void)
{
	if (!enable_bots) //disabled in comp.ini
		return;

	numnodes = 0; //hypov8 index starts at 0
	memset(nodes,0,sizeof(botnode_t) * MAX_BOTNODES);
	memset(path_table,INVALID,sizeof(short int)*MAX_BOTNODES*MAX_BOTNODES);	
}

///////////////////////////////////////////////////////////////////////
// Show the node for debugging (utility function)
///////////////////////////////////////////////////////////////////////
void ACEND_ShowNode(int node, int isTmpNode)
{
	edict_t *ent;

#ifndef HYPODEBUG //defined in project DEBUG
	if (dedicated->value)
		return;
#endif

	if (!debug_mode)
		return;

	if (node == INVALID)
		return;

	//stop showing other nodes if in "localnode" mode
	if (debug_mode_origin_ents && isTmpNode != 2)
		return;

	ent = G_Spawn();

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;

	ent->s.effects = (EF_COLOR_SHELL||EF_ROTATE);
	ent->s.renderfx2 = RF2_NOSHADOW;
	ent->s.renderfx = RF_FULLBRIGHT;

	if (nodes[node].type == BOTNODE_MOVE)				ent->s.skinnum = 0;
	else if (nodes[node].type == BOTNODE_LADDER)		ent->s.skinnum = 1;
	else if (nodes[node].type == BOTNODE_PLATFORM)		ent->s.skinnum = 2;
	else if (nodes[node].type == BOTNODE_TELEPORTER)	ent->s.skinnum = 3;
	else if (nodes[node].type == BOTNODE_ITEM)			ent->s.skinnum = 4;
	else if (nodes[node].type == BOTNODE_WATER)			ent->s.skinnum = 5;
	else if (nodes[node].type == BOTNODE_GRAPPLE)		ent->s.skinnum = 6;
	else if (nodes[node].type == BOTNODE_JUMP)			ent->s.skinnum = 7;
	else 												ent->s.skinnum = 8;


	ent->model = "models/bot/tris.md2";
	ent->s.modelindex = gi.modelindex(ent->model);

	ent->owner = ent;
	if (isTmpNode == 1)
		ent->nextthink = level.time + 20.0;//findenode
	else if (isTmpNode == 2)
		ent->nextthink = level.time + 0.1;//localnode
	else
		ent->nextthink = level.time + 50; //map loaded or node added (localnode off)


	ent->think = G_FreeEdict;                
	ent->dmg = 0;

	VectorCopy(nodes[node].origin,ent->s.origin);
	gi.linkentity (ent);

}

///////////////////////////////////////////////////////////////////////
// Draws the current path (utility function)
///////////////////////////////////////////////////////////////////////
void ACEND_DrawPath()
{
	int current_node, goal_node, next_node;
	int i = 0;

	current_node = show_path_from;
	goal_node = show_path_to;

	next_node = path_table[current_node][goal_node];

	// Now set up and display the path
	while(current_node != goal_node && current_node != -1)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BFG_LASER);
		gi.WritePosition (nodes[current_node].origin);
		gi.WritePosition (nodes[next_node].origin);
		gi.multicast (nodes[current_node].origin, MULTICAST_PVS);
		current_node = next_node;
		next_node = path_table[current_node][goal_node];
		i++;
		if (i > 15) //add hypov8 draw short paths
			break;
	}
}

///////////////////////////////////////////////////////////////////////
// Turns on showing of the path, set goal to -1 to 
// shut off. (utility function)
///////////////////////////////////////////////////////////////////////
void ACEND_ShowPath(edict_t *self, int goal_node)
{
	show_path_from = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY, BOTNODE_ALL);
	show_path_to = goal_node;
}

///////////////////////////////////////////////////////////////////////
// Add a node of type ?
///////////////////////////////////////////////////////////////////////
int ACEND_AddNode(edict_t *self, int type)
{
	// Block if we exceed maximum
	if (numnodes + 1 > MAX_BOTNODES)
		return INVALID;

	if (debug_mode) // add hypov8
		gi.sound(self, CHAN_AUTO, gi.soundindex("world/switches/pushbutton.wav"), 1, ATTN_NORM, 0);
	
	// Set location
	VectorCopy(self->s.origin,nodes[numnodes].origin);

	// Set type
	nodes[numnodes].type = type;

	/////////////////////////////////////////////////////
	// ITEMS
	// Move the z location up just a bit.
	if(type == BOTNODE_ITEM)
		nodes[numnodes].origin[2] += 16;

	// add hypov8 move node to be level with items
	if (type == BOTNODE_MOVE || type == BOTNODE_JUMP) //ladder?
		nodes[numnodes].origin[2] += 8;

	// Teleporters
	if (type == BOTNODE_TELEPORTER)
	{
		edict_t		*dest;
		dest = G_Find(NULL, FOFS(targetname), self->target);
		if (dest){
			VectorCopy(dest->s.origin, nodes[numnodes].origin);
			nodes[numnodes].origin[2] += 16; //+20
			nodes[numnodes].type = BOTNODE_MOVE;
			numnodes++;
			ACEND_UpdateNodeEdge(numnodes, numnodes - 1, true, false, false, false);
		}
		else
			safe_ConsolePrintf(PRINT_MEDIUM, "ACE: Teleporter without destination\n");

		VectorCopy(self->s.origin, nodes[numnodes].origin);
		nodes[numnodes].type = type;
		nodes[numnodes].origin[2] += 16; //+20

		if (debug_mode)
		{
			debug_printf("Node added %d type: Teleporter Dest\n", numnodes - 1);
			ACEND_ShowNode(numnodes - 1, 0);
		}
	}

	//add hypov8 trigger_push
	if (type == BOTNODE_TRIGPUSH)
	{
		vec3_t v1, v2;
		VectorCopy(self->maxs, v1);
		VectorCopy(self->mins, v2);
		nodes[numnodes].origin[0] = (v1[0] - v2[0]) / 2 + v2[0];
		nodes[numnodes].origin[1] = (v1[1] - v2[1]) / 2 + v2[1];
		nodes[numnodes].origin[2] = (v1[2] - v2[2]) / 2 + v2[2];
		nodes[numnodes].type = BOTNODE_JUMP;
	}


	if(type == BOTNODE_LADDER)
	{
		nodes[numnodes].type = BOTNODE_LADDER;
				
		if(debug_mode)
		{
			debug_printf("Node added %d type: Ladder\n",numnodes);
			ACEND_ShowNode(numnodes, 0);
		}

		// add hypov8
		nodes[numnodes].origin[2] += 8;
		
		numnodes++;
		return numnodes-1; // return the node added

	}

	// For platforms drop two nodes one at top, one at bottom
	if(type == BOTNODE_PLATFORM)
	{
		vec3_t maxx, minx;
		VectorCopy(self->maxs, maxx);
		VectorCopy(self->mins, minx);

		// To get the center for top node
		nodes[numnodes].origin[0] = (maxx[0] - minx[0]) / 2 + minx[0];
		nodes[numnodes].origin[1] = (maxx[1] - minx[1]) / 2 + minx[1];
		nodes[numnodes].origin[2] = self->maxs[2] +32; //hypov8 add +32
			
		if(debug_mode)	
			ACEND_ShowNode(numnodes,0);
		numnodes++;

		//bottom node
		nodes[numnodes].origin[0] = nodes[numnodes-1].origin[0];
		nodes[numnodes].origin[1] = nodes[numnodes-1].origin[1];
		nodes[numnodes].origin[2] = self->maxs[2] + self->pos2[2] + 32;  //hypov8 was +64// use actual height
		nodes[numnodes].type = BOTNODE_PLATFORM;

		// Add a link
		ACEND_UpdateNodeEdge(numnodes, numnodes - 1, true, true, false, false);
		
		if(debug_mode)
		{
			debug_printf("Node added %d type: Platform\n",numnodes);
			ACEND_ShowNode(numnodes, 0);
		}

		numnodes++;

		return numnodes -1;
	}
		
	if(debug_mode)
	{
		if(nodes[numnodes].type == BOTNODE_MOVE)
			debug_printf("Node added %d type: Move\n",numnodes);
		else if(nodes[numnodes].type == BOTNODE_TELEPORTER)
			debug_printf("Node added %d type: Teleporter\n",numnodes);
		else if(nodes[numnodes].type == BOTNODE_ITEM)
			debug_printf("Node added %d type: Item\n",numnodes);
		else if(nodes[numnodes].type == BOTNODE_WATER)
			debug_printf("Node added %d type: Water\n",numnodes);
		else if(nodes[numnodes].type == BOTNODE_JUMP)
			debug_printf("Node added %d type: Jump\n",numnodes);
/*		else if(nodes[numnodes].type == BOTNODE_GRAPPLE)
			debug_printf("Node added %d type: Grapple\n",numnodes);*/

		ACEND_ShowNode(numnodes, 0);
	}
		
	numnodes++;
	
	return numnodes-1; // return the node added
}

///////////////////////////////////////////////////////////////////////
// Add/Update node connections (paths)
///////////////////////////////////////////////////////////////////////
void ACEND_UpdateNodeEdge(int from, int to, qboolean stopJumpNodes, qboolean stopTeleNodes, qboolean checkSight, qboolean isTrigPush)
{
	int i;
	
	if (from == -1 || to == -1 || from == to || from > 999 || to > 999){
		if (debug_mode)	debug_printf(" ****ERROR**** Update: from:%d to:%d\n", from, to);
		return; } // safety
	if (stopJumpNodes && nodes[to].type == BOTNODE_JUMP){  //add hypov8 dont rout to jump nodes.
		if (debug_mode)	debug_printf(" *REJECTED* To jump node. Link %d -> %d\n", from, to);
		return;	}
	if (stopTeleNodes && nodes[from].type == BOTNODE_TELEPORTER)  //add hypov8 dont rout from teleporter nodes.
		return;
	if (nodes[from].type == BOTNODE_LADDER && nodes[from].origin[2] > nodes[to].origin[2]+32) //add hypov8 dont allow down on ladders
		return;

#if 1
	///////////////////
	// can node be seen
	///////////////////
	if (checkSight && !isTrigPush)
	{
		vec3_t v, angled, from2, to2;
		float distToTarget;
		trace_t tr;

		VectorCopy(nodes[from].origin, from2);
		VectorCopy(nodes[to].origin, to2);

		if (!(nodes[from].type == BOTNODE_LADDER || nodes[to].type == BOTNODE_LADDER) 
			&& nodes[from].type != BOTNODE_WATER)
		{
			if ((nodes[to].origin[2] - nodes[from].origin[2]) > 60)		
			{
				float pitch;
				VectorSubtract(from2, to2, v);
				distToTarget = VectorLength(v);	

				//allow stairs??
				VectorNormalize(v);
				vectoangles(v, angled);
				pitch = anglemod(angled[PITCH]);

				if (distToTarget > BOTNODE_DENSITY_STAIR || pitch > 50){
					if (debug_mode)	debug_printf(" *REJECTED* To High. Link %d -> %d\n", from, to);
					return;
				}
			}
		}

		//check any node for visual connection
		tr = gi.trace(from2, vec3_origin, vec3_origin, to2, NULL, MASK_BOT_SOLID_FENCE | CONTENTS_LADDER);
		if (tr.fraction != 1.0) //1.0 = nothing in the way
		{
			from2[2] += 20; //nudge up
			to2[2] += 20;
			tr = gi.trace(from2, vec3_origin, vec3_origin, to2, NULL, MASK_BOT_SOLID_FENCE | CONTENTS_LADDER);
			if (!(tr.fraction == 1.0)) //1.0 = nothing in the way
			{
				if (debug_mode)
					debug_printf(" *REJECTED* Not in sight. Link %d -> %d\n", from, to);

				return; //add hypov8. node link not added because its to far from edge and way above our head
			}
		}

	}
#endif


	// Add the link
	path_table[from][to] = to;

	// Now for the self-referencing part, linear time for each link added
	for(i=0;i<numnodes;i++)
		if(path_table[i][from] != INVALID)
			if(i == to)
				path_table[i][to] = INVALID; // make sure we terminate
			else
				path_table[i][to] = path_table[i][from];
		
	if(debug_mode)
		debug_printf("Link %d -> %d\n", from, to);
}

///////////////////////////////////////////////////////////////////////
// Remove a node edge
///////////////////////////////////////////////////////////////////////
void ACEND_RemoveNodeEdge(edict_t *self, int from, int to)
{
	int i;

	if(debug_mode) 
		debug_printf("%s: Removing Edge %d -> %d\n", self->client->pers.netname, from, to);
		
	path_table[from][to] = INVALID; // set to invalid			

	// Make sure this gets updated in our path array
	for(i=0;i<numnodes;i++)
		if(path_table[from][i] == to)
			path_table[from][i] = INVALID;
}

///////////////////////////////////////////////////////////////////////
// This function will resolve all paths that are incomplete
// usually called before saving to disk
///////////////////////////////////////////////////////////////////////
void ACEND_ResolveAllPaths()
{
	int i, from, to;
	int num=0;
	
	safe_ConsolePrintf(PRINT_HIGH, "ACE: Resolving all paths...");

	for (from = 0; from < numnodes; from++)
	{
		for (to = 0; to < numnodes; to++)
		{
			// update unresolved paths
			// Not equal to itself, not equal to -1 and equal to the last link
			if (from != to && path_table[from][to] == to)
			{
				num++;

				// Now for the self-referencing part linear time for each link added
				for (i = 0; i < numnodes; i++)
				{
					if (path_table[i][from] != -1)
						if (i == to)
							path_table[i][to] = -1; // make sure we terminate
						else
							path_table[i][to] = path_table[i][from];
				}
			}
		}
	}
	safe_ConsolePrintf(PRINT_MEDIUM, "done (%d updated)\n", num);
}

///////////////////////////////////////////////////////////////////////
// Save to disk file
//
// Since my compression routines are one thing I did not want to
// release, I took out the compressed format option. Most levels will
// save out to a node file around 50-200k, so compression is not really
// a big deal.
///////////////////////////////////////////////////////////////////////
void ACEND_SaveNodes()
{
	FILE *pOut;
	char filename[60];
	int i,j;
	int version = 1;
	cvar_t	*game_dir;
	char buf[32];

	if (!enable_bots) //disabled in comp.ini
		return;

	if (!level.is_spawn_bot)
		return;

	game_dir = gi.cvar("game", "", 0);
	sprintf(buf, "%s/nav/", game_dir->string); //DIR_SLASH

	
	// Resolve paths
	ACEND_ResolveAllPaths();

	safe_ConsolePrintf(PRINT_MEDIUM, "ACE: Saving node table...");

	strcpy(filename, buf);
	strcat(filename,level.mapname);
	strcat(filename,".nod");

	if ((pOut = fopen(filename, "wb")) == NULL)
	{
		safe_ConsolePrintf(PRINT_MEDIUM, "ERROR.\n");
		return; // bail
	}
	
	fwrite(&version,sizeof(int),1,pOut); // write version
	fwrite(&numnodes,sizeof(int),1,pOut); // write count
	fwrite(&num_items,sizeof(int),1,pOut); // write facts count
	
	fwrite(nodes,sizeof(botnode_t),numnodes,pOut); // write nodes
	
	for(i=0;i<numnodes;i++)
		for(j=0;j<numnodes;j++)
			fwrite(&path_table[i][j],sizeof(short int),1,pOut); // write count
		
	fwrite(item_table,sizeof(item_table_t),num_items,pOut); 		// write out the fact table

	fclose(pOut);
	
	safe_ConsolePrintf(PRINT_MEDIUM, "done. (%d nodes)\n", numnodes);
}

///////////////////////////////////////////////////////////////////////
// Read from disk file
///////////////////////////////////////////////////////////////////////
void ACEND_LoadNodes(void)
{
	FILE *pIn;
	int i,j;
	size_t bytes;
	char filename[60];
	int version;
	cvar_t	*game_dir;
	char buf[32];

	if (!enable_bots) //disabled in comp.ini
		return;
	if (!level.is_spawn_bot)
		return;

	game_dir = gi.cvar("game", "", 0);
	sprintf(buf, "%s/nav/", game_dir->string); //DIR_SLASH

	strcpy(filename, buf);
	strcat(filename,level.mapname);
	strcat(filename,".nod");

	if((pIn = fopen(filename, "rb" )) == NULL)
    {
		// Create item table
		safe_ConsolePrintf(PRINT_MEDIUM, "ACE: No node file found, creating new one...\n");
		ACEIT_BuildItemNodeTable(false);
		return; 
	}

	// determin version
	bytes = fread(&version,sizeof(int),1,pIn); // read version
	
	if(version == 1) 
	{
		safe_ConsolePrintf(PRINT_MEDIUM, "ACE: Loading node table...");

		bytes = fread(&numnodes,sizeof(int),1,pIn); // read count
		bytes = fread(&num_items,sizeof(int),1,pIn); // read facts count
		
		for(i=0;i<numnodes;i++)
			bytes = fread(&nodes[i], sizeof(botnode_t), 1, pIn);

		for(i=0;i<numnodes;i++)
			for(j=0;j<numnodes;j++)
				bytes = fread(&path_table[i][j],sizeof(short int),1,pIn); // write count

		for(i=0;i<num_items;i++)
			bytes = fread(&item_table[i], sizeof(item_table_t), 1, pIn);

		fclose(pIn);
	}
	else
	{
		// Create item table
		safe_ConsolePrintf(PRINT_MEDIUM, "ACE: No node file found, creating new one...\n");
		ACEIT_BuildItemNodeTable(false);
		return; // bail
	}
	
	safe_ConsolePrintf(PRINT_MEDIUM, "done.\n");
	
	ACEIT_BuildItemNodeTable(true);

}


// hypov8
// force link to bad teleportern.. inside brushes
void ACEND_PathToTeleporter(edict_t *player)
{
	short closest_node;
	if (!enable_bots) //disabled in comp.ini
		return;
	if (player->acebot.is_bot) //add hypov8 only route on 1st player
		return;

	if (sv_botpath->value && ACEND_PathMapIsFirstPlayer(player))
	{
		closest_node = ACEND_FindClosestNode(player, BOTNODE_DENSITY, BOTNODE_TELEPORTER);
		ACEND_UpdateNodeEdge(player->acebot.pm_last_node, closest_node, true, true, false, false);
		player->acebot.pm_last_node = closest_node;
	}
}

// hypov8
// teleportern destination
void ACEND_PathToTeleporterDest(edict_t *player)
{
	short closest_node;
	if (!enable_bots) //disabled in comp.ini
		return;
	if (player->acebot.is_bot) //add hypov8 only rout on 1st player
		return;

	if (sv_botpath->value && ACEND_PathMapIsFirstPlayer(player))
	{
		closest_node = ACEND_FindClosestNode(player, BOTNODE_DENSITY, BOTNODE_ALL);
		ACEND_UpdateNodeEdge(player->acebot.pm_last_node, closest_node, true, false, false, false);
		player->acebot.pm_last_node = closest_node;
	}
}

//add hypov8
void ACEND_PathToTrigPush(edict_t *player)
{
	if (!enable_bots) //disabled in comp.ini
		return;
	if (player->acebot.is_bot) //add hypov8 only rout on 1st player
		return;

	if (sv_botpath->value && ACEND_PathMapIsFirstPlayer(player))
	{
		short closest_node = ACEND_FindClosestNode(player, BOTNODE_DENSITY, BOTNODE_JUMP);
		ACEND_UpdateNodeEdge(player->acebot.pm_last_node, closest_node, false, true, true, true);
		player->acebot.pm_last_node = closest_node;
		player->acebot.bot_pm_jumpPadMove = level.framenum + 3; //wait for ground entity to be null
		player->acebot.bot_pm_playerJumpTime = false;
	}
}

//add hypov8 set trig_push destination to a jump node.
void ACEND_PathToTrigPushDest(edict_t *player)
{
	if (!enable_bots) //disabled in comp.ini
		return;
	if (player->acebot.is_bot) //add hypov8 only rout on 1st player
		return;

	if (sv_botpath->value && ACEND_PathMapIsFirstPlayer(player))
	{
		short closest_node;
		closest_node = ACEND_FindClosestNode(player, BOTNODE_DENSITY, BOTNODE_JUMP);
		ACEND_UpdateNodeEdge(player->acebot.pm_last_node, closest_node, false, true, false, true);
		player->acebot.pm_last_node = closest_node;
		player->acebot.bot_pm_jumpPadMove = false;
	}
}



//////////////////////////////////////////////////////////////////////
//ACEND_DebugNodesLocal
// Draws local path (utility function) 
//hypo
//will draw a path from closest nodes to there routable paths
///////////////////////////////////////////////////////////////////////
void ACEND_DebugNodesLocal(void)
{
#define NODECOUNT 20
	float distToTarget;
	edict_t *firstPlayer;
	short j, k = NODE0, i, m = NODE0, iPlyr;
	short current_node, next_node, i2, n;
	static short count[NODECOUNT];

	if (debug_mode && debug_mode_origin_ents) //"sv botdebug" "localnode"
	{
		memset(count, INVALID, sizeof(count));
		//only used first player
		for_each_player_not_bot(firstPlayer, iPlyr)
		{
			//hypov8 show all close nodes
			for (j = 0; j < numnodes; j++)	
			{
				distToTarget = VectorDistance(firstPlayer->s.origin, nodes[j].origin);
				if (distToTarget <= 192)
				{
					ACEND_ShowNode(j, 2); //hypov8 show closest node
					k++;
					if (k >= NODECOUNT) //only do 20 nodes
					{
						safe_cprintf(firstPlayer, PRINT_MEDIUM, "*ERROR* more than 20 nodes in your area\n");
						break;
					}
				}
			}

			//hypov8 get closest node
			current_node = ACEND_FindClosestNode(firstPlayer, 64, BOTNODE_ALL);
			if (current_node != INVALID)
			{
				m = NODE0;
				n = NODE0;
				//loop through nodes and display any paths connecting from closest node
				for (i = NODE0; i < 1000; i++)
				{
					if (path_table[current_node][i] != -1)
					{
						next_node = path_table[current_node][i];
						if (next_node <= -2)
							break;
						for (i2 = NODE0; i2 < NODECOUNT; i2++)
						{
							if (count[i2] == next_node)
								break;
						}
						if (i2 == NODECOUNT)
						{
							count[n] = next_node;
							gi.WriteByte(svc_temp_entity);
							gi.WriteByte(TE_BFG_LASER);
							gi.WritePosition(nodes[current_node].origin);
							gi.WritePosition(nodes[next_node].origin);
							gi.multicast(nodes[current_node].origin, MULTICAST_PVS);
							//current_node = next_node;
							//next_node = path_table[current_node][goal_node];
							m++;
							n++;
							if (m > NODECOUNT) //add hypov8 draw short paths
								break;
						}
					}
				}
			}

			break; //only use first valid player "for_each_player_not_bot"
		}

	}
}
