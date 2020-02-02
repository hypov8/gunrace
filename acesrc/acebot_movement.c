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
//  acebot_movement.c - This file contains all of the 
//                      movement routines for the ACE bot
//           
///////////////////////////////////////////////////////////////////////

#include "../g_local.h"
#include "acebot.h"

//hypov8 gr add
#include "../gunrace.h"

#define CHECKSKYDOWNDIST 3072

///////////////////////////////////////////////////////////////////////
// Checks if bot can move (really just checking the ground)
// Also, this is not a real accurate check, but does a
// pretty good job and looks for lava/slime. 
///////////////////////////////////////////////////////////////////////
qboolean ACEMV_CanMove(edict_t *self, int direction)
{
	vec3_t forward, right;
	vec3_t offset,start,end;
	vec3_t angles;
	trace_t tr;

	// Now check to see if move will move us off an edge
	VectorCopy(self->s.angles,angles);
	
	if(direction == MOVE_LEFT)
		angles[1] += 90;
	else if(direction == MOVE_RIGHT)
		angles[1] -= 90;
	else if(direction == MOVE_BACK)
		angles[1] -=180;

	// Set up the vectors
	AngleVectors (angles, forward, right, NULL);
	
	VectorSet(offset, 36, 0, 24);
	G_ProjectSource (self->s.origin, offset, forward, right, start);
		
	VectorSet(offset, 36, 0, -400);
	G_ProjectSource (self->s.origin, offset, forward, right, end);
	
	tr = gi.trace(start, NULL, NULL, end, self, MASK_BOT_SOLID_FENCE);
	
	if(tr.fraction > 0.3 && tr.fraction != 1 || tr.contents & (CONTENTS_LAVA|CONTENTS_SLIME))
	{
		if(debug_mode)
			debug_printf("%s: move blocked\n",self->client->pers.netname);
		return false;	
	}
	
	return true; // yup, can move
}


//hypov8 added to check flat ground/edge
static qboolean ACEMV_CanMove_Simple(edict_t *self, int direction)
{
	vec3_t forward, right;
	vec3_t offset, end, down;
	vec3_t angles;
	trace_t tr;
	vec3_t minx = { -4, -4, 0 }; //was 24
	vec3_t maxx = { 4, 4, 0 }; //was 24
	vec3_t minWall = { -16, -16, 0 }; // fix for steps
	vec3_t maxWall = { 16, 16, 48 };

	// Now check to see if move will move us off an edgemap team_fast_cash, dm_fis_b1
	VectorCopy(self->s.angles, angles); //MOVE_FORWARD

	if (direction == MOVE_LEFT)
		angles[1] += 90;
	else if (direction == MOVE_RIGHT)
		angles[1] -= 90;
	else if (direction == MOVE_BACK)
		angles[1] -= 180;

	// Set up the vectors
	AngleVectors(angles, forward, right, NULL);

	VectorSet(offset, 24, 0, 0); //hypo low value. incase steps
	G_ProjectSource(self->s.origin, offset, forward, right, end);

	tr = gi.trace(self->s.origin, minWall, maxWall, end, self,  MASK_BOT_SOLID_FENCE);

	if (tr.fraction != 1) //wall hit
	{
		return false;
	}
	else //check for falling off edge
	{
		VectorSet(offset, 48, 0, 0); //hypo increase?
		G_ProjectSource(self->s.origin, offset, forward, right, end);

		VectorCopy(end, down);
		down[2] -= CHECKSKYDOWNDIST;
		tr = gi.trace(end, minx, maxx, down, self, MASK_BOT_DROP_SKY);
		///VectorCopy(start, down);

		if (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
			return false;
		if ((tr.surface->flags & SURF_SKY) && !tr.startsolid)
			return false;

	}

	return true; // yup, can move
}


///////////////////////////////////////////////////////////////////////
// Handle special cases of crouch/jump
//
// If the move is resolved here, this function returns
// true.
///////////////////////////////////////////////////////////////////////
qboolean ACEMV_SpecialMove(edict_t *self, usercmd_t *ucmd)
{	//hypov8 todo: face towards ladder
	vec3_t dir,forward,right,start,end,offset;
	vec3_t top;
	trace_t tr; 
	
	// Get current direction
	VectorCopy(self->client->ps.viewangles,dir);
	dir[YAW] = self->s.angles[YAW];
	AngleVectors (dir, forward, right, NULL);

	VectorSet(offset, 18, 0, 0);
	G_ProjectSource (self->s.origin, offset, forward, right, start);
	offset[0] += 18;
	G_ProjectSource (self->s.origin, offset, forward, right, end);
	
	// trace it
	start[2] += 18; // so they are not jumping all the time
	end[2] += 18;
	tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE /*MASK_MONSTERSOLID*/);
		
	if (tr.allsolid || tr.fraction != 1.0f && 
		(strcmp(tr.ent->classname, "func_door") != 0 && strcmp(tr.ent->classname, "func_door_rotating") != 0))
	{
		// Check for crouching
		start[2] -= 14;
		end[2] -= 14;

		// Set up for crouching check
		VectorCopy(self->maxs,top);
		top[2] = 24; // crouching height //hypov8 todo: check this.. 24?
		tr = gi.trace(start, self->mins, top, end, self, MASK_BOT_SOLID_FENCE /*MASK_PLAYERSOLID*/);
		
		// Crouch
		if(!tr.allsolid && tr.fraction == 1.0f) 
		{
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = -BOT_JUMP_VEL;
			return true;
		}
		
		// Check for jump //hypov8 todo: kp box height
		start[2] += 32;
		end[2] += 32;
		tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE /* MASK_MONSTERSOLID*/);

		if (!tr.allsolid && tr.fraction == 1.0f)
		{	
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = BOT_JUMP_VEL;
			return true;
		}
	}
	
	return false; // We did not resolve a move here
}

//add hypov8
static int ACEMV_CheckLavaAndSky(edict_t *self) //add normal falling edges
{
	vec3_t dir, forward, right, offset, start, wall, down;
	trace_t trace; // for eyesight
	vec3_t minx = { 0, 0, 0 };
	vec3_t maxx = { 0, 0, 0 };
	vec3_t minx2 = { -12, -12, 0 }; //hypo 12, player not allways looking straight+steps
	vec3_t maxx2 = { 12, 12, 48 };

	//make sure we are not jumping. or allready falling :)
	if (!self->groundentity && self->velocity[2]< -120)
		return false; //return didnot resolve

	// Get current angle and set up "eyes"
	VectorCopy(self->s.angles, dir);
	dir[2] = 0.0f;
	AngleVectors(dir, forward, right, NULL);
	VectorSet(offset, 24, 0, 0); // focalpoint 
	G_ProjectSource(self->s.origin, offset, forward, right, wall);


	trace = gi.trace(self->s.origin, minx2, maxx2, wall, self, MASK_BOT_SOLID_FENCE);
	if (trace.fraction != 1)
		return false; // must b hitting wall, return didnot resolve


	VectorSet(offset, 58, 0, 0); // focalpoint 
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	VectorCopy(start, down);
	down[2] -= CHECKSKYDOWNDIST;

	trace = gi.trace(start, minx, maxx, down, self, MASK_BOT_DROP_SKY); //hypov8 todo water? & test alpha
	if (trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
	{
		if (ACEMV_CanMove_Simple(self, MOVE_LEFT)){
			self->s.angles[YAW] += 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
			self->s.angles[YAW] -= 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_BACK)){
			self->s.angles[YAW] -= 180;
			return true; //return resolved move
		}
		return 2; //cant move
	}
	else if ((trace.surface->flags & SURF_SKY) && !trace.startsolid)
	{
		if (ACEMV_CanMove_Simple(self, MOVE_LEFT)){
			self->s.angles[YAW] += 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
			self->s.angles[YAW] -= 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_BACK)){ //add hypo fail if all sky
			self->s.angles[YAW] -= 180;
			return true; //return resolved move
		}
		return 2; //cant move
	}

	return false; //cant move
}




///////////////////////////////////////////////////////////////////////
// Checks for obstructions in front of bot
//
// This is a function I created origianlly for ACE that
// tries to help steer the bot around obstructions.
//
// If the move is resolved here, this function returns true.
///////////////////////////////////////////////////////////////////////
qboolean ACEMV_CheckEyes(edict_t *self, usercmd_t *ucmd)
{
	vec3_t  forward, right;
	vec3_t  leftstart, rightstart,focalpoint;
	vec3_t  upstart,upend;
	vec3_t  dir,offset;
	vec3_t  headHeight; //hypov8 allow crouch

	trace_t traceRight,traceLeft,traceUp, traceFront; // for eyesight

	// Get current angle and set up "eyes"
	VectorCopy(self->s.angles,dir);
	AngleVectors (dir, forward, right, NULL);
	
	// Let them move to targets by walls
	if(!self->movetarget)
		VectorSet(offset,200,0,4); // focalpoint 
	else
		VectorSet(offset,36,0,4); // focalpoint 
	
	G_ProjectSource (self->s.origin, offset, forward, right, focalpoint);

	VectorCopy(self->maxs, headHeight);
	if (self->client->ps.pmove.pm_flags & PMF_DUCKED)	{
		headHeight[2] += -24;
		ucmd->upmove = 0;
	}

	// Check from self to focalpoint
	// Ladder code
	VectorSet(offset,36,0,0); // set as high as possible
	G_ProjectSource (self->s.origin, offset, forward, right, upend);
	traceFront = gi.trace(self->s.origin, self->mins, headHeight, upend, self, CONTENTS_LADDER | MASK_BOT_SOLID_FENCE);
		
	if (traceFront.contents & CONTENTS_LADDER) // using detail brush here cuz sometimes it does not pick up ladders...??
	{
		ucmd->upmove = BOT_JUMP_VEL;
		ucmd->forwardmove = BOT_FORWARD_VEL;
		return true;
	}
	
	// If this check fails we need to continue on with more detailed checks
	if(traceFront.fraction == 1)
	{	
		if (ACEMV_CheckLavaAndSky(self) == 2) //add hypov8
			return true;// standing on sky. dont move, die!!!
		ucmd->forwardmove = BOT_FORWARD_VEL;
		return true;
	}

	//add hypov8 we checked ok previously. try crouch??
	if (self->movetarget)
	{
		trace_t traceStand, traceCrouch;
		vec3_t v, GoalCpy;

		VectorCopy(self->mins, v);
		VectorCopy(self->movetarget->s.origin, GoalCpy);
		GoalCpy[2] += 9.5; //hypov8 move item off ground to match player height.
							//item_bbox=15 player_bbox=24 (9 differnce)

		traceStand = gi.trace(self->s.origin, self->mins, self->maxs, GoalCpy, self, MASK_BOT_SOLID_FENCE);
		if (traceStand.fraction != 1.0f)
		{
			VectorCopy(self->maxs, v);
			v[2] += -24; // bbox crouching height (DUCKING_MAX_Z = 24)
			traceCrouch = gi.trace(self->s.origin, self->mins, v, GoalCpy, self, MASK_BOT_SOLID_FENCE);

			if (traceCrouch.fraction == 1.0f)
			{
				ucmd->upmove = -BOT_JUMP_VEL;
				ucmd->forwardmove = BOT_FORWARD_VEL;
				return true;
			}
		}
	}

	VectorSet(offset, 0, 18, 4);
	G_ProjectSource (self->s.origin, offset, forward, right, leftstart);
	
	offset[1] -= 36; // want to make sure this is correct
	//VectorSet(offset, 0, -18, 4);
	G_ProjectSource (self->s.origin, offset, forward, right, rightstart);

	traceRight = gi.trace(rightstart, NULL, NULL, focalpoint, self, MASK_BOT_SOLID_FENCE);
	traceLeft = gi.trace(leftstart, NULL, NULL, focalpoint, self, MASK_BOT_SOLID_FENCE);

	// Wall checking code, this will degenerate progressivly so the least cost 
	// check will be done first.
		
	// If open space move ok
	if(traceRight.fraction != 1 || traceLeft.fraction != 1 
		&& strcmp(traceLeft.ent->classname,"func_door")!=0
		&& strcmp(traceLeft.ent->classname, "func_door_rotating") != 0)
	{
		// Special uppoint logic to check for slopes/stairs/jumping etc.
		VectorSet(offset, 0, 18, 24);
		G_ProjectSource (self->s.origin, offset, forward, right, upstart);

		VectorSet(offset,0,0,200); // scan for height above head
		G_ProjectSource (self->s.origin, offset, forward, right, upend);
		traceUp = gi.trace(upstart, NULL, NULL, upend, self, MASK_BOT_SOLID_FENCE );
			
		VectorSet(offset,200,0,200*traceUp.fraction-5); // set as high as possible
		G_ProjectSource (self->s.origin, offset, forward, right, upend);
		traceUp = gi.trace(upstart, NULL, NULL, upend, self, MASK_BOT_SOLID_FENCE );

		// If the upper trace is not open, we need to turn.
		if(traceUp.fraction != 1)
		{						
			if(traceRight.fraction > traceLeft.fraction)
				self->s.angles[YAW] += (1.0 - traceLeft.fraction) * 45.0;
			else
				self->s.angles[YAW] += -(1.0 - traceRight.fraction) * 45.0;
			
			ucmd->forwardmove = BOT_FORWARD_VEL;
			return true;
		}
	}
	if (ACEMV_CheckLavaAndSky(self) == 1)
		return true; //add hypov8			

	return false;
}

///////////////////////////////////////////////////////////////////////
// Make the change in angles a little more gradual, not so snappy
// Subtle, but noticeable.
// 
// Modified from the original id ChangeYaw code...
///////////////////////////////////////////////////////////////////////
void ACEMV_ChangeBotAngle (edict_t *ent)
{
	float	ideal_yaw;
	float   ideal_pitch;
	float	current_yaw;
	float   current_pitch;
	float	move;
	float	speed;
	vec3_t  ideal_angle;
			
	// Normalize the move angle first
	VectorNormalize(ent->acebot.move_vector);

	current_yaw = anglemod(ent->s.angles[YAW]);
	current_pitch = anglemod(ent->s.angles[PITCH]);
	
	vectoangles(ent->acebot.move_vector, ideal_angle);

	ideal_yaw = anglemod(ideal_angle[YAW]);
	ideal_pitch = anglemod(ideal_angle[PITCH]);

	// Yaw
	if (current_yaw != ideal_yaw)
	{	
		move = ideal_yaw - current_yaw;
		speed = ent->yaw_speed;
		if (ideal_yaw > current_yaw)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}
		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}
		ent->s.angles[YAW] = anglemod (current_yaw + move);	
	}

	// Pitch
	if (current_pitch != ideal_pitch)
	{	
		move = ideal_pitch - current_pitch;
		speed = ent->yaw_speed;
		if (ideal_pitch > current_pitch)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}
		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}
		ent->s.angles[PITCH] = anglemod (current_pitch + move);	
	}
}

///////////////////////////////////////////////////////////////////////
// Set bot to move to it's movetarget. (following node path)
///////////////////////////////////////////////////////////////////////
void ACEMV_MoveToGoal(edict_t *self, usercmd_t *ucmd)
{
	// If a rocket or grenade is around deal with it
	// Simple, but effective (could be rewritten to be more accurate)
	if(strcmp(self->movetarget->classname,"rocket")==0 ||
	   strcmp(self->movetarget->classname,"grenade")==0)
	{
		VectorSubtract(self->movetarget->s.origin, self->s.origin, self->acebot.move_vector);
		ACEMV_ChangeBotAngle(self);
		if(debug_mode)
			debug_printf("%s: Oh crap a rocket!\n",self->client->pers.netname);
		
		// strafe left/right
		if(rand()%1 && ACEMV_CanMove(self, MOVE_LEFT))
			ucmd->sidemove = -BOT_SIDE_VEL;
		else if(ACEMV_CanMove(self, MOVE_RIGHT))
			ucmd->sidemove = BOT_SIDE_VEL;
		return;

	}
	else
	{
		// Set bot's movement direction
		VectorSubtract(self->movetarget->s.origin, self->s.origin, self->acebot.move_vector);
		ACEMV_ChangeBotAngle(self);
		ucmd->forwardmove = BOT_FORWARD_VEL;
		return;
	}
}


//hypov8 face towards ladder surface
void ACEMV_FaceLadder(edict_t *self, usercmd_t *ucmd, qboolean onLadder)
{
	int i;
	trace_t tr;
	qboolean foundSurfaceDir = false;

	for (i = 0; i <= 3; i++)
	{
		vec3_t minx = { -2, -2, -24 };
		vec3_t maxx = { 2, 2, 48 };
		vec3_t angles;
		vec3_t target;
		VectorCopy(self->s.origin, target);
		if (i == 0)	target[0] += 18; //20 units infront
		else if (i == 1)	target[0] -= 18;
		else if (i == 2)	target[1] += 18;
		else if (i == 3)	target[1] -= 18;

		tr = gi.trace(self->s.origin, minx, maxx, target, self, MASK_ALL);
		if (tr.contents & CONTENTS_LADDER)
		{ 
			//face ladder
			VectorSubtract(target, self->s.origin, self->acebot.move_vector);
			vectoangles(self->acebot.move_vector, angles);
			VectorCopy(angles, self->s.angles);
			foundSurfaceDir = true;
			ACEMV_ChangeBotAngle(self);
			break;
		}
	}

	if (!onLadder || !foundSurfaceDir)
	{
		ucmd->upmove = BOT_JUMP_VEL *0.5;
		ucmd->forwardmove = BOT_FORWARD_VEL;
		self->acebot.move_vector[2]; //look straight ahead. not up?
		ACEMV_ChangeBotAngle(self);
	}
	else
	{
		ucmd->upmove = BOT_JUMP_VEL;
		ucmd->forwardmove = BOT_FORWARD_VEL *0.5;
		self->s.angles[0] = -10;
	}
	return;
}


//hypov8
//remove short range goals and set jump pad node as hit, incase in bad location
void ACEMV_JumpPadUpdate(edict_t *bot/*, float pushSpeed*/)
{
	//bot used jump pad.
	if (bot->acebot.is_bot)
	{
		//hypov8 todo: wander/goal. make jump pads a goal?
//		bot->trigPushTimer = level.framenum + 5; //0.5 seconds untill bot can move
//		bot->isMovingUpPushed = true;
		bot->goalentity = NULL; //wander?
		bot->goal_ent;
		bot->last_goal;
		//find closest node and link to next node
		bot->movetarget = NULL;

		bot->acebot.state = BOTSTATE_WANDER;
		bot->acebot.current_node = INVALID;
		bot->acebot.next_node = INVALID;
		bot->acebot.goal_node = INVALID;

		{ //hypov8 new ver
			bot->acebot.bot_isPushed = level.framenum + 10; //allow to be pusshed away, incase we are still touching ground
			VectorCopy(bot->s.origin, bot->acebot.botOldOrigin);
			bot->acebot.state = BOTSTATE_WANDER; //reset goal
			bot->acebot.bot_targetPlayerNode = 0;
			bot->acebot.wander_timeout = level.time + 0.2;
			bot->s.angles[PITCH] = 0;
		}
	}
}


///////////////////////////////////////////////////////////////////////
// Main movement code. (following node path)
///////////////////////////////////////////////////////////////////////
void ACEMV_Move(edict_t *self, usercmd_t *ucmd)
{
	vec3_t dist;
	int current_node_type=-1;
	int next_node_type=-1;
	int i;

	//add hypov8
	if (self->acebot.bot_isPushed != false && self->acebot.bot_isPushed < level.framenum && self->groundentity) //todo:
		self->acebot.bot_isPushed = false;

	//hypov8 stop bot movement if in the air
	if (self->acebot.bot_isPushed)
	{
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
		return;
	}

		
	// Get current and next node back from nav code.
	if(!ACEND_FollowPath(self))
	{
		self->acebot.state = BOTSTATE_WANDER;
		self->acebot.bot_targetPlayerNode = 0;
		self->acebot.wander_timeout = level.time + 1.0;
		return;
	}

	current_node_type = nodes[self->acebot.current_node].type;
	next_node_type = nodes[self->acebot.next_node].type;
		
	///////////////////////////
	// Move To Goal
	///////////////////////////
	if (self->movetarget)
		ACEMV_MoveToGoal(self,ucmd);

/*	////////////////////////////////////////////////////////
	// Grapple
	///////////////////////////////////////////////////////
	if(next_node_type == BOTNODE_GRAPPLE)
	{
		ACEMV_ChangeBotAngle(self);
		ACEIT_ChangeWeapon(self,FindItem("grapple"));	
		ucmd->buttons = BUTTON_ATTACK;
		return;
	}
	// Reset the grapple if hangin on a graple node
	if(current_node_type == BOTNODE_GRAPPLE)
	{
		CTFPlayerResetGrapple(self);
		return;
	}*/
	
	////////////////////////////////////////////////////////
	// Platforms
	///////////////////////////////////////////////////////
	if(current_node_type != BOTNODE_PLATFORM && next_node_type == BOTNODE_PLATFORM)
	{
		// check to see if lift is down?
		for(i=0;i<num_items;i++)
			if(item_table[i].node == self->acebot.next_node)
				if(item_table[i].ent->moveinfo.state != STATE_BOTTOM)
				    return; // Wait for elevator
	}
	if(current_node_type == BOTNODE_PLATFORM && next_node_type == BOTNODE_PLATFORM)
	{
		// Move to the center
		self->acebot.move_vector[2] = 0; // kill z movement	
		if(VectorLength(self->acebot.move_vector) > 10)
			ucmd->forwardmove = BOT_FORWARD_VEL *0.5; // walk to center
				
		ACEMV_ChangeBotAngle(self);
		
		return; // No move, riding elevator
	}

	//temp
	  if(current_node_type == BOTNODE_JUMP)
		  if (next_node_type != BOTNODE_ITEM && nodes[self->acebot.next_node].origin[2] > self->s.origin[2])
		  {
			  if (debug_mode)
				  debug_printf("%s: move jump\n");;
		  }


	////////////////////////////////////////////////////////
	// Jumpto Nodes
	///////////////////////////////////////////////////////
	if(next_node_type == BOTNODE_JUMP || 
	  (current_node_type == BOTNODE_JUMP && next_node_type != BOTNODE_ITEM && nodes[self->acebot.next_node].origin[2] > self->s.origin[2]))
	{	//hypov8 todo: face towards ladder
		// Set up a jump move
		if (!self->acebot.bot_isPushed) //todo:
		{
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = BOT_JUMP_VEL;

			ACEMV_ChangeBotAngle(self);

			VectorCopy(self->acebot.move_vector, dist);
			VectorScale(dist, 440, self->velocity);
		}
		else
		{
			ucmd->forwardmove = 0;
			ucmd->upmove = 0;
			self->s.angles[PITCH] = 0;
		}

		return;
	}
	
	////////////////////////////////////////////////////////
	// Ladder Nodes
	///////////////////////////////////////////////////////
	if(next_node_type == BOTNODE_LADDER && nodes[self->acebot.next_node].origin[2] > self->s.origin[2])
	{
		//getting onto ladder
		if (current_node_type != BOTNODE_LADDER)
			ACEMV_FaceLadder(self, ucmd, false);
		else
			ACEMV_FaceLadder(self, ucmd, true);
		return;
	}
	// If getting off the ladder
	if(current_node_type == BOTNODE_LADDER && next_node_type != BOTNODE_LADDER &&
	   nodes[self->acebot.next_node].origin[2] > self->s.origin[2])
	{
		ucmd->forwardmove = BOT_FORWARD_VEL;
		ucmd->upmove = BOT_JUMP_VEL*0.5;
		self->velocity[2] = 200;
		ACEMV_ChangeBotAngle(self);
		return;
	}

	////////////////////////////////////////////////////////
	// Water Nodes
	///////////////////////////////////////////////////////
	if(current_node_type == BOTNODE_WATER)
	{
		// We need to be pointed up/down
		ACEMV_ChangeBotAngle(self);

		// If the next node is not in the water, then move up to get out.
		if(next_node_type != BOTNODE_WATER && !(gi.pointcontents(nodes[self->acebot.next_node].origin) & MASK_WATER)) // Exit water
			ucmd->upmove = BOT_JUMP_VEL;
		
		ucmd->forwardmove = BOT_FORWARD_VEL *0.75;
		return;

	}
	
	// Falling off ledge? //hypov8 todo: face towards ladder
	if(!self->groundentity)
	{
		ACEMV_ChangeBotAngle(self);

		self->velocity[0] = self->acebot.move_vector[0] * 360;
		self->velocity[1] = self->acebot.move_vector[1] * 360;
	
		return;
	}
		
	// Check to see if stuck, and if so try to free us
	// Also handles crouching
	if (self->acebot.botMoveDirVel <= 3)
 	//if(VectorLength(self->velocity) < 37)
	{
		// Keep a random factor just in case....
		if(random() > 0.1 && ACEMV_SpecialMove(self, ucmd))
			return;
		
		self->s.angles[YAW] += random() * 180 - 90; 

		ucmd->forwardmove = BOT_FORWARD_VEL;
		
		return;
	}

	// Otherwise move as fast as we can
	ucmd->forwardmove = BOT_FORWARD_VEL;

	ACEMV_ChangeBotAngle(self);
	
}


///////////////////////////////////////////////////////////////////////
// Wandering code (based on old ACE movement code) 
///////////////////////////////////////////////////////////////////////
void ACEMV_Wander(edict_t *self, usercmd_t *ucmd)
{
	vec3_t  temp;
	
	// Do not move
	if(self->acebot.next_move_time > level.time)
		return;
	self->s.angles[PITCH] = 0; //hypov8 reset pitch

	//add hypov8
	if (self->acebot.bot_isPushed != false && self->acebot.bot_isPushed < level.framenum && self->groundentity) //todo:
		self->acebot.bot_isPushed = false;

	//hypov8 stop bot movement if in the air
	if (self->acebot.bot_isPushed)
	{
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
		return;
	}

	// Special check for elevators, stand still until the ride comes to a complete stop.
	if(self->groundentity != NULL && self->groundentity->use == Use_Plat)
		if(self->groundentity->moveinfo.state == STATE_UP ||
		   self->groundentity->moveinfo.state == STATE_DOWN) // only move when platform not
		{
			self->velocity[0] = 0;
			self->velocity[1] = 0;
			self->velocity[2] = 0;
			self->acebot.next_move_time = level.time + 0.5;
			return;
		}
	
	
	// Is there a target to move to
	if (self->movetarget)
		ACEMV_MoveToGoal(self,ucmd);
		
	////////////////////////////////
	// Swimming?
	////////////////////////////////
	VectorCopy(self->s.origin,temp);
	temp[2]+=24;

	if(gi.pointcontents (temp) & MASK_WATER)
	{
		// If drowning and no node, move up
		if(self->client->next_drown_time > 0)
		{
			ucmd->upmove = 1;	
			self->s.angles[PITCH] = -45; 
			ucmd->forwardmove = BOT_FORWARD_VEL *0.75;
		}
		else
		{
			if (self->goalentity  && self->goalentity->solid != SOLID_NOT)
			{
				vec3_t angles;
				// Set direction
				VectorSubtract(self->goalentity->s.origin, self->s.origin, self->acebot.move_vector);
				vectoangles(self->acebot.move_vector, angles);
				VectorCopy(angles, self->s.angles);
				ucmd->forwardmove = BOT_FORWARD_VEL *0.75;

				//add hypov8 jump if target above us
				if (self->s.origin[2] < self->goalentity->s.origin[2])
					ucmd->upmove = BOT_JUMP_VEL;

			}
			else
			{
				self->goalentity = NULL; //hypov8 was not solid
				ucmd->upmove = BOT_JUMP_VEL;
				self->s.angles[PITCH] = -10; //move up
				ucmd->forwardmove = BOT_FORWARD_VEL *0.75;
			}
		}

	}
	else
		self->client->next_drown_time = 0; // probably shound not be messing with this, but
	
	////////////////////////////////
	// Lava?
	////////////////////////////////
	temp[2]-=48;	
	if(gi.pointcontents(temp) & (CONTENTS_LAVA|CONTENTS_SLIME))
	{
		//	safe_bprintf(PRINT_MEDIUM,"lava jump\n");
		self->s.angles[YAW] += random() * 360 - 180; 
		ucmd->forwardmove = BOT_FORWARD_VEL;
		ucmd->upmove = BOT_JUMP_VEL;
		return;
	}

	if(ACEMV_CheckEyes(self,ucmd))
		return;
	
	// Check for special movement if we have a normal move (have to test)
	if (self->acebot.botMoveDirVel <= 3)
 	//if(VectorLength(self->velocity) < 37)
	{
		if(random() > 0.1 && ACEMV_SpecialMove(self,ucmd))
			return;

		self->s.angles[YAW] += random() * 180 - 90; 

		if(!M_CheckBottom(self) || !self->groundentity) // if there is ground continue otherwise wait for next move
			ucmd->forwardmove = BOT_FORWARD_VEL;
		
		return;
	}
	
	ucmd->forwardmove = BOT_FORWARD_VEL;

}


static void ACEMV_Attack_AimRandom(edict_t *self)
{
	float range, ran, rand_y, disAcc, skill, vel;
	vec3_t velTmp;

	if (!self->enemy)
		return;

	// Get distance.
	range = VectorDistance(self->acebot.bot_enemyOrigin, self->s.origin);
	//get enemy speed
	VectorCopy(self->enemy->velocity, velTmp);
	velTmp[2] = 0;
	vel = (float)VectorLength(velTmp);

	// closer the distance, the more accurate
	if		(range < 48)	disAcc = 2; //stop crowbar missing
	else if (range < 120)	disAcc = 28;
	else if (range < 200)	disAcc = 24;
	else if (range < 250)	disAcc = 22;
	else if (range < 300)	disAcc = 20;
	else if (range < 350)	disAcc = 19;
	else if (range < 475)	disAcc = 18;
	else					disAcc = 17;

	//rand scaled between -1 to 1
	ran = random();
	if (ran > 0.30 && ran < 0.70)
		ran = random(); //2nd try at less accurecy
	ran = (ran * 2) - 1;

	//inverse skill 1.0 to 0.0
	skill = (1 - (sv_botskill->value *.25));
	if (vel < 50)		skill *= .25;//skill *= (vel / 250); //more accurate when standing still	
	else if (vel < 100)	skill *= .5;
	else if (vel < 150)	skill *= .75;

	rand_y = skill * ran;

	if (self->onfiretime > 0){
		disAcc *= 0.75;
		rand_y *= 2;
	}

	self->acebot.bot_accuracy = rand_y * disAcc;
}
//end ATTACK RANDOM
//add hypov8
//random bullet dir
//stops bot view being jerky
void ACEMV_Attack_CalcRandDir(edict_t *self, vec3_t aimdir)
{
	vec3_t v, v2,viewH;
	if (!enable_bots)
		return;

	//random aim
	ACEMV_Attack_AimRandom(self);

	VectorCopy(self->s.origin, viewH);
	viewH[2] += self->viewheight;

	VectorSubtract(self->acebot.bot_enemyOrigin, viewH, aimdir);
	vectoangles(self->acebot.move_vector, v);

	v[YAW] += self->acebot.bot_accuracy;
	if (v[YAW] < 0)
		v[YAW] += 360;
	else if 
		(v[YAW] > 360)		v[YAW] -= 360;

	AngleVectors(v, v2, NULL, NULL);
	VectorCopy(v2, aimdir);
}



///////////////////////////////////////////////////////////////////////
// Attack movement routine
//
// NOTE: Very simple for now, just a basic move about avoidance.
//       Change this routine for more advanced attack movement.
///////////////////////////////////////////////////////////////////////
void ACEMV_Attack (edict_t *self, usercmd_t *ucmd)
{
	vec3_t  target;
	vec3_t  angles;
	//float randYaw;

//GUNRACE_START
	//make player run forward to enemy
	if (self->client->resp.curwepIndex == GR_WEPS - 1 /*!Q_stricmp(self->client->resp.curweap, GR_FINAL_WEP)*/)
	{
		ucmd->sidemove = 0;
		ucmd->forwardmove = BOT_FORWARD_VEL;
		//slow bot down some times. add variable
		if (sv_botskill->value <= 1.0f)
		{
			if (rand() % 1 == 1)
				ucmd->forwardmove = BOT_FORWARD_VEL*0.65;
		}
		else if (sv_botskill->value <= 2.0f)
		{
			if (rand() % 2 == 1)
				ucmd->forwardmove = BOT_FORWARD_VEL*0.65;
		}
		else if (sv_botskill->value <= 3.0f)
		{
			if (rand() % 3 == 1)
				ucmd->forwardmove = BOT_FORWARD_VEL*0.65;
		}
		else if (sv_botskill->value <= 3.6f)
		{
			if (rand() % 4 == 1)
				ucmd->forwardmove = BOT_FORWARD_VEL*0.65;
		}
	}
	else
//GUNRACE_END
#if 1 //ndef HYPODEBUG
	{
		qboolean strafe = false;
		qboolean strafeDir;
		static const int frames = 5;
		qboolean moveResolved = false;
		float c;
		float skill= sv_botskill->value* .25; //0 to 1

		// Randomly choose a movement direction
		c = random();

		if (c > .5 && skill < c)
			moveResolved = true;

		//hypo make player strafe in 1 dir longer
		if (self->acebot.bot_last_strafeTime >= level.framenum)
		{
			strafe = true;
			if (self->acebot.bot_last_strafeDir == MOVE_LEFT)
				strafeDir = MOVE_LEFT;
			else if (self->acebot.bot_last_strafeDir == MOVE_RIGHT)
				strafeDir = MOVE_RIGHT;
		}

		//dont move on jump pads
		if (!self->acebot.bot_isPushed && !moveResolved)
		{
			//Com_Printf("strafeDir=%i, strafe=%i, rand=%d\n", self->acebot.last_strafeDir, strafe, c);

			if (((c < 0.500f && !strafe) || (strafe && strafeDir == MOVE_LEFT))
				&& ACEMV_CanMove(self, MOVE_LEFT) && ACEMV_CanMove_Simple(self, MOVE_LEFT)){
				ucmd->sidemove = -BOT_SIDE_VEL;
				moveResolved = true;
				if (!strafe){
					self->acebot.bot_last_strafeTime = level.framenum + frames;
					self->acebot.bot_last_strafeDir = MOVE_LEFT;
				}
			}
			else if (((c >= 0.500f && !strafe) || (strafe && strafeDir == MOVE_RIGHT))
				&& ACEMV_CanMove(self, MOVE_RIGHT) && ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
				ucmd->sidemove = BOT_SIDE_VEL;
				moveResolved = true;
				if (!strafe){
					self->acebot.bot_last_strafeTime = level.framenum + frames;
					self->acebot.bot_last_strafeDir = MOVE_RIGHT;
				}
			}

			if (c < 0.3 && ACEMV_CanMove(self, MOVE_FORWARD) && ACEMV_CanMove_Simple(self, MOVE_FORWARD)){
				ucmd->forwardmove = BOT_FORWARD_VEL;
				moveResolved = true;
			}
			else if (c > 0.7 && ACEMV_CanMove(self, MOVE_BACK) && ACEMV_CanMove_Simple(self, MOVE_BACK)){ //was forward??
				ucmd->forwardmove = -BOT_FORWARD_VEL;
				moveResolved = true;
			}
		}

		//hyopv8 stop bots momentum running off edges, with no move random resolves
		if (!moveResolved && !self->acebot.bot_isPushed && self->groundentity){
			if (c < 0.500f)
			{
				if (ACEMV_CanMove_Simple(self, MOVE_LEFT)){
					ucmd->sidemove = -BOT_SIDE_VEL;
					moveResolved = true;
				}
				else if (ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
					ucmd->sidemove = BOT_SIDE_VEL;
					moveResolved = true;
				}
			}
			else
			{
				if (ACEMV_CanMove_Simple(self, MOVE_RIGHT))	{
					ucmd->sidemove = BOT_SIDE_VEL;
					moveResolved = true;
				}
				else if (ACEMV_CanMove_Simple(self, MOVE_LEFT))	{
					ucmd->sidemove = -BOT_SIDE_VEL;
					moveResolved = true;
				}
			}
		}

		if (!moveResolved)
		{
			if (ACEMV_CanMove_Simple(self, MOVE_FORWARD))
				ucmd->forwardmove = BOT_FORWARD_VEL;
			else if (ACEMV_CanMove_Simple(self, MOVE_BACK))
				ucmd->forwardmove = -BOT_FORWARD_VEL;
		}
	}
#else


		// Randomly choose a movement direction
		c = random();

		if (c < 0.2 && ACEMV_CanMove(self, MOVE_LEFT))
			ucmd->sidemove -= 400;
		else if (c < 0.4 && ACEMV_CanMove(self, MOVE_RIGHT))
			ucmd->sidemove += 400;

		if (c < 0.6 && ACEMV_CanMove(self, MOVE_FORWARD))
			ucmd->forwardmove += 400;
		else if (c < 0.8 && ACEMV_CanMove(self, MOVE_FORWARD))
			ucmd->forwardmove -= 400;
	}
#endif
	
	// Set the attack 
	ucmd->buttons = BUTTON_ATTACK;
	// Aim
	VectorCopy(self->enemy->s.origin,target);
	if ((self->enemy->client->ps.pmove.pm_flags & PMF_DUCKED))
		target[2] -= 28; //hypov8 move bots aim down todo: check fence???
	VectorCopy(target, self->acebot.bot_enemyOrigin);


		//VectorCopy(self->enemy->s.origin, target);
	//set aim dir, compensate for movement
	{
		float fwd;
		float side;
		vec3_t dist;
		vec3_t forward3, right3;
		vec3_t offset3, botMoveComp; //compensate for movement error
		vec3_t v;

		if (!self->acebot.bot_isPushed)
		{
			//hypov8 todo: PM_Friction. PM_Accelerate. fix slight aim errors
			//GUNRACE_START
			if (self->client->resp.curwepIndex == GR_WEPS - 1)
			{
				fwd =0.0f;
				side = 0.0f; //10 fps
			}
			else
			//GUNRACE_END
			{
				fwd = (float)ucmd->forwardmove / 11;
				side = (float)ucmd->sidemove / 11; //10 fps
			}
			VectorSet(offset3, fwd, side, 0);	
			VectorCopy(self->s.angles, v);

			AngleVectors(v, forward3, right3, NULL);
			G_ProjectSource(self->s.origin, offset3, forward3, right3, botMoveComp);
		}
		else
		{	// use momentum on jump pad
			VectorSubtract(self->s.origin, self->acebot.botOldOrigin, dist);
			VectorMA(self->acebot.botOldOrigin, 2.0f, dist, botMoveComp);
		}

		// Set Aim direction
		VectorSubtract(target, botMoveComp, self->acebot.move_vector);
		vectoangles(self->acebot.move_vector, angles);
		VectorCopy(angles, self->s.angles);

	}
	/*
	// Set direction
	VectorSubtract (target, self->s.origin, self->acebot.move_vector);
	vectoangles (self->acebot.move_vector, angles);
	VectorCopy(angles, self->s.angles);*/


	//hypov8 special move
	{
		vec3_t player_origin;

		//hypov8 move player up if it has a target in water
		VectorCopy(self->s.origin, player_origin);
		player_origin[2] += 8; //hypov8 was 24

		if (gi.pointcontents(player_origin) & MASK_WATER)
			ucmd->upmove = BOT_JUMP_VEL;

		//hypov8 stop bot movement if in the air
		if (!self->groundentity)
		{
			ucmd->forwardmove = 0;
			ucmd->sidemove = 0;
		}
	}
}
