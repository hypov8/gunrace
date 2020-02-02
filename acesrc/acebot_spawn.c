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
//  acebot_spawn.c - This file contains all of the 
//                   spawing support routines for the ACE bot.
//
///////////////////////////////////////////////////////////////////////

#include "../g_local.h"
#include "../m_player.h"
#include "acebot.h"

//hypov8 add
///////////////////////////
// load bots only when a client exists and game is running
///////////////////////////
void ACECM_LevelBegin(void)
{
	if (!enable_bots) 
		return;

	if (!(level.modeset == PUBLIC || level.modeset == PUBLICSPAWN
		|| level.modeset == MATCHSPAWN || level.modeset == MATCH))
		return;

	if (!level.is_spawn)
		return;
		
	if (!level.is_spawn_bot)
	{
		int		count = 0;
		int		i;
		edict_t	*doot;

		for_each_player_not_bot(doot, i)
			count++;

		if (count)
		{
			level.is_spawn_bot = true;
			ACEND_InitNodes();
			ACEND_LoadNodes();
			ACESP_LoadBots();
		}
	}
}


static void ACESP_playerskin(int playernum, char *s)
{
	// only update player's skin config if it has changed (saves a bit of bandwidth)
	if (strcmp(level.playerskins[playernum], s))
	{
		strncpy(level.playerskins[playernum], s, sizeof(level.playerskins[playernum]) - 1);
		gi.configstring(CS_PLAYERSKINS + playernum, s);
	}
}

/*
===========
ACESP_ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ACESP_ClientDisconnect(edict_t *ent)
{
	int		playernum;
	int		i;

	if (!ent->client || !ent->client->pers.connected)
		return;

	if (ent->inuse)
	{
		if (ent->solid != SOLID_NOT) DropCash(ent);
		if (ent->client->resp.vote == CALLED_VOTE)
			level.voteset = NO_VOTES;

		playernum = ent - g_edicts - 1;

		if (ent->client->resp.time && (ent->client->pers.team || ent->client->resp.score > 0))
		{
			if (level.player_num == 64)
			{
				memmove(playerlist, playerlist + 1, 63 * sizeof(playerlist[0]));
				level.player_num--;
			}
			playerlist[level.player_num].frags = ent->client->resp.score;
			playerlist[level.player_num].deposits = ent->client->resp.deposited;
			playerlist[level.player_num].stole = ent->client->resp.stole;
			playerlist[level.player_num].acchit = ent->client->resp.acchit;
			playerlist[level.player_num].accshot = ent->client->resp.accshot;
			for (i = 0; i<8; i++)
				playerlist[level.player_num].fav[i] = ent->client->resp.fav[i];
			playerlist[level.player_num].team = ent->client->pers.team;
			playerlist[level.player_num].time = ent->client->resp.time;
			strcpy(playerlist[level.player_num].player, level.playerskins[playernum]);
			if (teamplay->value)
			{
				char *p = strrchr(playerlist[level.player_num].player, '/');
				if (!p) goto skiplist; // shouldn't happen but just in case
				memset(p + 5, ' ', 7); // ignore body+legs
			}
			level.player_num++;
		}

	skiplist:
		// inform any chasers
		for (i = 1; i <= maxclients->value; i++)
		{
			if (!g_edicts[i].inuse)
				continue;
			if (!g_edicts[i].client)
				continue;
			if (g_edicts[i].client->chase_target == ent)
				ChaseStop(&g_edicts[i]);
		}

	ACEIT_PlayerRemoved(ent);

		if (ent->solid != SOLID_NOT)
		{
			// send effect
			gi.WriteByte(svc_muzzleflash);
			gi.WriteShort(ent - g_edicts);
			gi.WriteByte(MZ_LOGOUT);
			gi.multicast(ent->s.origin, MULTICAST_PVS);
		}

		gi.unlinkentity(ent);
		ent->s.modelindex = 0;
		ent->s.num_parts = 0;
		ent->solid = SOLID_NOT;
		ent->inuse = false;

		ACESP_playerskin(playernum, "");
	}

	ent->acebot.is_bot = false; //bug fix

	ent->classname = "disconnected";
	ent->client->pers.connected = 0;
}



///////////////////////////////////////////////////////////////////////
// Called when the level changes, store maps and bots (disconnected)
///////////////////////////////////////////////////////////////////////
void ACECM_LevelEnd(void)
{
	edict_t *ent;
	int	i;

	if (!enable_bots) //disabled in comp.ini
		return;

	for (i = 0; i < maxclients->value; i++)
	{
		ent = g_edicts + i + 1;
		if (ent->inuse && ent->client && ent->client->pers.connected) //hypov8 add spec?
		{
			if (ent->acebot.is_bot)
				ACESP_ClientDisconnect(ent);
			else
				ACEIT_PlayerRemoved(ent);
		}	
	}
}
//end

///////////////////////////////////////////////////////////////////////
// Had to add this function in this version for some reason.
// any globals are wiped out between level changes....so
// save the bots out to a file. 
//
// NOTE: There is still a bug when the bots are saved for
//       a dm game and then reloaded into a CTF game.
///////////////////////////////////////////////////////////////////////
void ACESP_SaveBots()
{
	edict_t *bot;
	FILE *pOut;
	int i, count = 0;
	cvar_t	*game_dir;
	char fileName[32];

	if (!enable_bots) //disabled in comp.ini
		return;

	if (!level.is_spawn_bot)
		return;

	if (level.modeset != MATCH && level.modeset != PUBLIC)
		return;

	game_dir = gi.cvar("game", "", 0);
	sprintf(fileName, "%s/_bots.tmp", game_dir->string); //DIR_SLASH

	if ((pOut = fopen(fileName, "wb")) == NULL){
		safe_ConsolePrintf(PRINT_MEDIUM, "ACE: Saveing bots...ERROR");
		return; // bail
	}
	// Get number of bots
	for (i = (int)maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i /*+ 1*/; //hypov8 bug invalid client number

		if (bot->inuse && bot->acebot.is_bot)
			count++;
	}
	
	fwrite(&count,sizeof (int),1,pOut); // Write number of bots

	safe_ConsolePrintf(PRINT_MEDIUM, "ACE: Saveing %i bots...", count);

	for (i = (int)maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i /*+ 1*/; //hypov8 bug invalid client number

		if (bot->inuse && bot->acebot.is_bot)
			fwrite(bot->client->pers.userinfo,sizeof (char) * MAX_INFO_STRING,1,pOut); 
	}
		
    fclose(pOut);
	safe_ConsolePrintf(PRINT_MEDIUM, "done.\n");
}

///////////////////////////////////////////////////////////////////////
// Had to add this function in this version for some reason.
// any globals are wiped out between level changes....so
// load the bots from a file.
//
// Side effect/benifit are that the bots persist between games.
///////////////////////////////////////////////////////////////////////
void ACESP_LoadBots()
{
    FILE *pIn;
	char userinfo[MAX_INFO_STRING];
	int i, count;
	size_t bytes;
	cvar_t	*game_dir;
	char fileName[32];

	if (!enable_bots) //disabled in comp.ini
		return;

	game_dir = gi.cvar("game", "", 0);
	sprintf(fileName, "%s/_bots.tmp", game_dir->string); //DIR_SLASH


	if ((pIn = fopen(fileName, "rb")) == NULL){
		safe_ConsolePrintf(PRINT_MEDIUM, "ACE: Loading bots...ERROR.\n");
		return; // bail
	}
	bytes = fread(&count,sizeof (int),1,pIn); 
	safe_ConsolePrintf(PRINT_MEDIUM, "ACE: Loading %i bots...", count);

	for(i=0;i<count;i++)
	{
		bytes = fread(userinfo,sizeof(char) * MAX_INFO_STRING,1,pIn); 
		ACESP_SpawnBot (NULL, NULL, NULL, userinfo);
	}
		
    fclose(pIn);
	safe_ConsolePrintf(PRINT_MEDIUM, "done.\n");
}

///////////////////////////////////////////////////////////////////////
// Called by PutClient in Server to actually release the bot into the game
// Keep from killin' each other when all spawned at once
///////////////////////////////////////////////////////////////////////
void ACESP_HoldSpawn(edict_t *self)
{
	if (!KillBox (self))
	{	// could't spawn in?
	}

	gi.linkentity (self);

	self->think = ACEAI_Think;
	self->nextthink = level.time + FRAMETIME;

	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (self-g_edicts);
	gi.WriteByte (MZ_LOGIN);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	safe_bprintf (PRINT_MEDIUM, "%s entered the game\n", self->client->pers.netname);

}



/*
===========
ACESP_ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
static void ACESP_ClientConnect(edict_t *ent, char *userinfo)
{
	char	*value;
	//edict_t	*doot;
	//int j;

	ent->client = NULL;
	//ent->inuse = false;
	ent->flags = 0;

	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);

	// clear the respawning variables
	InitClientResp(ent->client);
	{
		memset(&ent->client->pers, 0, sizeof(ent->client->pers));
		InitClientPersistant(ent->client);
		ent->client->pers.connected = -1; // distinguish between initial and map change connections
	}

	value = Info_ValueForKey(userinfo, "ip");
	strncpy(ent->client->pers.ip, value, sizeof(ent->client->pers.ip) - 1);

	ent->client->resp.enterframe = 0;
	ent->client->move_frame = ent->client->resp.name_change_frame = -80;  //just to be sure
	ClientUserinfoChanged(ent, userinfo);

	strncpy(ent->client->pers.country, "Botville", sizeof(ent->client->pers.country) - 1); //GeoIP2 

#if 0
	//dont send joined info if they are bots connecting
	gi.dprintf("ACE: bot (%s) connected\n", ent->client->pers.netname);

	for_each_player_not_bot(doot, j)
		safe_cprintf(doot, PRINT_CHAT, "%s connected from %s\n", ent->client->pers.netname, ent->client->pers.country);
#endif
	
	ent->client->pers.lastpacket = curtime;
	level.lastactive = level.framenum;
	ent->client->showscores = NO_SCOREBOARD;
}





///////////////////////////////////////////////////////////////////////
// Modified version of id's code
///////////////////////////////////////////////////////////////////////
void ACESP_PutClientInServer (edict_t *bot, int team)
{


#if 1
	PutClientInServer(bot, true, team);
#else
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 48};
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	client_persistant_t	saved;
	client_respawn_t	resp;
//	char *s;
	
	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	SelectSpawnPoint (bot, spawn_origin, spawn_angles);
	
	index = bot-g_edicts-1;
	client = bot->client;

	// deathmatch wipes most client data every spawn
	{
		char userinfo[MAX_INFO_STRING];

		resp = bot->client->resp;
		memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
		InitClientPersistant (client);
		bot->client->move_frame = bot->client->resp.name_change_frame = -80;  //just to be sure
		ClientUserinfoChanged (bot, userinfo);
	}

	bot->name_index = -1;

	// clear everything but the persistant data
	saved = client->pers;
	memset (client, 0, sizeof(*client));
	client->pers = saved;
	client->resp = resp;
	
	// copy some data from the client to the entity
	FetchClientEntData (bot);
	
//GUNRACE_START
	gr_ResetPlayerBeginDM(client);
//GUNRACE_END

	// clear entity values
	bot->groundentity = NULL;
	bot->client = &game.clients[index];
	bot->takedamage = DAMAGE_AIM;
	bot->movetype = MOVETYPE_WALK;
	bot->solid = SOLID_BBOX;
	bot->svflags &= ~(SVF_DEADMONSTER | SVF_NOCLIENT);

	//give 3 seconds of imortality on each spawn (anti-camp) 
	if (anti_spawncamp->value)
		client->invincible_framenum = level.framenum + 10;  //1 second (1.5 for players)

	// RAFAEL
	bot->viewheight = 40;
	bot->inuse = true;

	bot->classname = "bot";
	//bot->classname = "player";
	bot->mass = 200;
	bot->deadflag = DEAD_NO;
	bot->air_finished = level.time + 12;
	bot->clipmask = MASK_PLAYERSOLID;
//	bot->model = "players/male/tris.md2";
	bot->pain = player_pain;
	bot->die = player_die;
	bot->waterlevel = 0;
	bot->watertype = 0;
	bot->flags &= ~FL_NO_KNOCKBACK;
	//////////////////////////////////bot->svflags &= ~(SVF_DEADMONSTER|SVF_NOCLIENT);
	bot->acebot.is_jumping = false;
	bot->acebot.bot_isPushed = false;
	bot->acebot.lastDamageTimer = 0;
	bot->acebot.lastDamagePlayerIndex = -1;
	bot->client->pers.noantilag = false; // add lag to bot :)
/*	if(ctf->value)
	{
		client->resp.ctf_team = team;
		client->resp.ctf_state = CTF_BOTSTATE_START;
		s = Info_ValueForKey (client->pers.userinfo, "skin");
		CTFAssignSkin(bot, s);
	}*/

	bot->s.renderfx2 = 0;
	bot->onfiretime = 0;

	bot->cast_info.aiflags |= AI_GOAL_RUN;	// make AI run towards us if in pursuit
	//KP_END


	VectorCopy (mins, bot->mins);
	VectorCopy (maxs, bot->maxs);
	VectorClear (bot->velocity);

	//KP_ADD
	bot->cast_info.standing_max_z = bot->maxs[2];

	bot->cast_info.scale = MODEL_SCALE;
	bot->s.scale = bot->cast_info.scale - 1.0;
	//KP_END

	//acebot
	bot->acebot.old_target = -1; //hypo add
	bot->acebot.old_targetTimer = 0;	//hypov8 add
	client->pers.team = team;
	bot->client->pers.noantilag = true;
	//end

	if (bot->solid)
	{
		trace_t tr;
		tr = gi.trace(spawn_origin, bot->mins, bot->maxs, spawn_origin, NULL, CONTENTS_MONSTER);
		if (tr.startsolid)
		{
			// spawn point is occupied, try next to it
			vec3_t origin1;
			int c;
			VectorCopy(spawn_origin, origin1);
			for (c = 0;;)
			{
				for (i = 0; i<4; i++)
				{
					vec3_t start, end;
					float angle = (spawn_angles[YAW] + i * 90 - 45) / 360 * M_PI * 2;
					start[0] = spawn_origin[0] + cos(angle) * 50;
					start[1] = spawn_origin[1] + sin(angle) * 50;
					start[2] = spawn_origin[2];
					VectorCopy(start, end);
					end[2] -= 25;
					tr = gi.trace(start, bot->mins, bot->maxs, end, NULL, MASK_PLAYERSOLID);
					if (!tr.startsolid && tr.fraction < 1)
					{
						VectorCopy(start, spawn_origin);
						break;
					}
				}
				if (i < 4) break;
				if (++c == 2) break;
				// try another spawn point
				for (i = 0; i<3; i++)
				{
					SelectSpawnPoint(bot, spawn_origin, spawn_angles);
					if (!VectorCompare(spawn_origin, origin1))
						break;
				}
				if (i == 3) break;
			}
		}
	}
//end add


	client->ps.pmove.origin[0] = spawn_origin[0]*8;
	client->ps.pmove.origin[1] = spawn_origin[1]*8;
	client->ps.pmove.origin[2] = spawn_origin[2]*8;

//ZOID
//	client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
//ZOID

	if (((int)dmflags->value & DF_FIXED_FOV))
	{
		client->ps.fov = 90;
	}
	else
	{
		client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));
		if (client->ps.fov < 1)
			client->ps.fov = 90;
		else if (client->ps.fov > 160)
			client->ps.fov = 160;
	}

	// RAFAEL
	// weapon mdx
	{
		int i;
	
		memset(&(client->ps.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);

		client->ps.num_parts++;
	// JOSEPH 22-JAN-99
		if (client->pers.weapon)
			client->ps.model_parts[PART_HEAD].modelindex = gi.modelindex(client->pers.weapon->view_model);
		
		for (i=0; i<MAX_MODELPART_OBJECTS; i++)
			client->ps.model_parts[PART_HEAD].skinnum[i] = 0; // will we have more than one skin???
	}

	if (client->pers.weapon)
		client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);
	// END JOSEPH


	// clear entity state values
	bot->s.effects = 0;
	bot->s.skinnum = bot - g_edicts - 1;
	bot->s.modelindex = 255;		// will use the skin specified model
// KINGPIN_X	bot->s.modelindex2 = 255;		// custom gun model
	bot->s.frame = 0;
	VectorCopy (spawn_origin, bot->s.origin);
	bot->s.origin[2] += 1;	// make sure off ground
	VectorCopy (bot->s.origin, bot->s.old_origin);

	//add hypov8. calculate bots movement
	VectorCopy(spawn_origin, bot->acebot.botOldOrigin);


	// bikestuff
	bot->biketime = 0;
	bot->bikestate = 0;


// Ridah, Hovercars
	if (g_vehicle_test->value)
	{
		if (g_vehicle_test->value == 3)
			bot->s.modelindex = gi.modelindex ("models/props/moto/moto.mdx");
		else
			bot->s.modelindex = gi.modelindex ("models/vehicles/cars/viper/tris_test.md2");

//		ent->s.modelindex2 = 0;
		bot->s.skinnum = 0;
		bot->s.frame = 0;

		if ((int)g_vehicle_test->value == 1)
			bot->flags |= FL_HOVERCAR_GROUND;
		else if ((int)g_vehicle_test->value == 2)
			bot->flags |= FL_HOVERCAR;
		else if ((int)g_vehicle_test->value == 3)
			bot->flags |= FL_BIKE;
		else if ((int)g_vehicle_test->value == 4)
			bot->flags |= FL_CAR;
	}
// done.
	else if (dm_locational_damage->value)	// deathmatch, note models must exist on server for client's to use them, but if the server has a model a client doesn't that client will see the default male model
	{
		char	*s;
		char	modeldir[MAX_QPATH];//, *skins;
		int		len;
		int		did_slash;
		char	modelname[MAX_QPATH];
//		int		skin;

		// NOTE: this is just here for collision detection, modelindex's aren't actually set

		bot->s.num_parts = 0;		// so the client's setup the model for viewing

		s = Info_ValueForKey (client->pers.userinfo, "skin");

//		skins = strstr( s, "/" ) + 1;

		// converts some characters to NULL's
		len = strlen( s );
		did_slash = 0;
		for (i=0; i<len; i++)
		{
			if (s[i] == '/')
			{
				s[i] = '\0';
				did_slash = true;
			}
			else if (s[i] == ' ' && did_slash)
			{
				s[i] = '\0';
			}
		}

		if (strlen(s) > MAX_QPATH-1)
			s[MAX_QPATH-1] = '\0';

		strcpy(modeldir, s);
		
		if (!modeldir[0])
			strcpy( modeldir, "male_thug" );
		
		memset(&(bot->s.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);
		
		bot->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/head.mdx" );
		bot->s.model_parts[bot->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &bot->s.model_parts[bot->s.num_parts-1] );
		if (!bot->s.model_parts[bot->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/head.mdx", &bot->s.model_parts[bot->s.num_parts-1] );

		bot->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/legs.mdx" );
		bot->s.model_parts[bot->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &bot->s.model_parts[bot->s.num_parts-1] );
		if (!bot->s.model_parts[bot->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/legs.mdx", &bot->s.model_parts[bot->s.num_parts-1] );

		bot->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/body.mdx" );
		bot->s.model_parts[bot->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &bot->s.model_parts[bot->s.num_parts-1] );
		if (!bot->s.model_parts[bot->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/body.mdx", &bot->s.model_parts[bot->s.num_parts-1] );

		bot->s.num_parts++;
		bot->s.model_parts[PART_GUN].modelindex = 255;
	}
	else	// make sure we can see their weapon
	{
		memset(&(bot->s.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);
		bot->s.model_parts[PART_GUN].modelindex = 255;
		bot->s.num_parts = PART_GUN+1;	// make sure old clients recieve the view weapon index
	}

	//KP_END


	// set the delta angle
	for (i=0 ; i<3 ; i++)
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);

	bot->s.angles[PITCH] = 0;
	bot->s.angles[YAW] = spawn_angles[YAW];
	bot->s.angles[ROLL] = 0;
	VectorCopy (bot->s.angles, client->ps.viewangles);
	VectorCopy (bot->s.angles, client->v_angle);


	if (bot->solid)
		KillBox(bot);

	gi.linkentity(bot);

	bot->think = ACEAI_Think;
	bot->nextthink = level.time + FRAMETIME;


	// we don't want players being backward-reconciled to the place they died
	if (antilag->value && bot->solid != SOLID_NOT)
		G_ResetHistory(bot);
	
	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon(bot);

	if (bot->solid != SOLID_NOT || bot->client->resp.enterframe == level.framenum)
	{
		// send effect
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(bot - g_edicts);
		gi.WriteByte(MZ_LOGIN);
		if (bot->solid != SOLID_NOT)
			gi.multicast(bot->s.origin, MULTICAST_PVS);
		else
			gi.unicast(bot, false);
	}

	
	{//ace
		bot->enemy = NULL;
		bot->movetarget = NULL;
		bot->acebot.state = BOTSTATE_MOVE;
		bot->acebot.bot_isPushed = false;

		// Set the current node
		bot->acebot.current_node = ACEND_FindClosestReachableNode(bot, BOTNODE_DENSITY, BOTNODE_ALL);
		bot->acebot.goal_node = bot->acebot.current_node;
		bot->acebot.next_node = bot->acebot.current_node;
		bot->acebot.next_move_time = level.time;
		bot->acebot.suicide_timeout = level.time + 15.0;
	}

	if (level.intermissiontime)
		MoveClientToIntermission(bot);

#endif	
}

///////////////////////////////////////////////////////////////////////
// Respawn the bot
///////////////////////////////////////////////////////////////////////
void ACESP_Respawn (edict_t *self)
{
	if (!enable_bots) //disabled in comp.ini
		return;

	CopyToBodyQue (self);

	ACESP_PutClientInServer (self,0);

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

		// hold in place briefly
	self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->client->ps.pmove.pm_time = 14;

	self->client->respawn_time = level.time;
	
}

///////////////////////////////////////////////////////////////////////
// Find a free client spot
///////////////////////////////////////////////////////////////////////
edict_t *ACESP_FindFreeClient (void)
{
	edict_t *bot;
	int	i;
	int max_count=0;
	
	// This is for the naming of the bots
	for (i = (int)maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i /*+ 1*/; //hypov8 bug invalid client number
		
		if(bot->count > max_count)
			max_count = bot->count;
	}

	// Check for free spot
	for (i = (int)maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i /*+ 1*/; //hypov8 bug invalid client number

		if (!bot->inuse)
			break;
	}

	bot->count = max_count + 1; // Will become bot name...

	if (bot->inuse)
		bot = NULL;
	
	return bot;
}

///////////////////////////////////////////////////////////////////////
// Set the name of the bot and update the userinfo
///////////////////////////////////////////////////////////////////////
void ACESP_SetName(edict_t *bot, char *name, char *skin)
{
	float rnd;
	char userinfo[MAX_INFO_STRING];
	char bot_skin[MAX_INFO_STRING];
	char bot_name[MAX_INFO_STRING];
	qboolean existingName = false;
	int		i;
	edict_t	*dood;

	//hypov8 fix <name clash> on bots
	if (name[0])	{
		for_each_player(dood, i)		{
			if (!strcmp(name, dood->client->pers.netname))	{
				existingName = true;
				break;	
			}
		}
	}

	// Set the name for the bot.
	// name
	if(existingName ||strlen(name) == 0)
		sprintf(bot_name,"GRBot_%d",bot->count);
	else

		strcpy(bot_name,name);
	
	// skin
	if(strlen(skin) == 0)
	{
		// randomly choose skin 
		rnd = random();
		if(rnd  < 0.05)
			sprintf(bot_skin,"female_chick/005 005 005");
		else if(rnd < 0.1)
			sprintf(bot_skin,"male_thug/010 010 010");
		else if(rnd < 0.15)
			sprintf(bot_skin,"male_thug/011 011 011");
		else if(rnd < 0.2)
			sprintf(bot_skin,"male_thug/012 012 012");
		else if(rnd < 0.25)
			sprintf(bot_skin,"female_chick/006 006 006");
		else if(rnd < 0.3)
			sprintf(bot_skin,"male_thug/013 013 013");
		else if(rnd < 0.35)
			sprintf(bot_skin,"male_thug/013 013 013");
		else if(rnd < 0.4)
			sprintf(bot_skin,"female_chick/020 020 020");
		else if(rnd < 0.45)
			sprintf(bot_skin, "male_runt/072 142 017");
		else if(rnd < 0.5)
			sprintf(bot_skin, "male_runt/003 005 004");
		else if(rnd < 0.55)
			sprintf(bot_skin, "male_runt/011 014 132");
		else if(rnd < 0.6)
			sprintf(bot_skin,"male_thug/017 017 017");
		else if(rnd < 0.65)
			sprintf(bot_skin,"female_chick/056 056 056");
		else if(rnd < 0.7)
			sprintf(bot_skin,"male_thug/300 300 300");
		else if(rnd < 0.75)
			sprintf(bot_skin,"male_thug/010 010 010");
		else if(rnd < 0.8)
			sprintf(bot_skin,"male_thug/008 008 008");
		else if(rnd < 0.85)
			sprintf(bot_skin,"male_thug/009 019 017");
		else if(rnd < 0.9)
			sprintf(bot_skin,"female_chick/032 032 032");
		else if(rnd < 0.95)
			sprintf(bot_skin, "male_runt/001 001 017");
		else 
			sprintf(bot_skin,"male_thug/004 004 004");
	}
	else
		strcpy(bot_skin,skin);

	// initialise userinfo
	memset (userinfo, 0, sizeof(userinfo));

	// add bot's name/skin/hand to userinfo
	Info_SetValueForKey (userinfo, "name", bot_name);
	Info_SetValueForKey (userinfo, "skin", bot_skin);
	Info_SetValueForKey (userinfo, "hand", "2"); // bot is center handed for now!

	Info_SetValueForKey (userinfo, "extras", "0000");
	Info_SetValueForKey (userinfo, "ver", "121");
	Info_SetValueForKey (userinfo, "fov", "90");
	Info_SetValueForKey (userinfo, "rate", "25000");
	Info_SetValueForKey (userinfo, "gl_mode", "1");
	Info_SetValueForKey (userinfo, "ip", "loopback");
	Info_SetValueForKey(userinfo, "msg", "0");

#if 1
	{
		edict_t	*doot;
		int j;
		// send joined info if bot was added during gameplay
		gi.dprintf("ACE: bot (%s) connected\n", bot_name);

		for_each_player_not_bot(doot, j)
			safe_cprintf(doot, PRINT_CHAT, "%s connected from %s\n", bot_name, bot->client->pers.country);
	}
#endif

	ACESP_ClientConnect(bot, userinfo);

	ACESP_SaveBots(); // make sure to save the bots
}

///////////////////////////////////////////////////////////////////////
// Spawn the bot
///////////////////////////////////////////////////////////////////////
void ACESP_SpawnBot (char *team, char *name, char *skin, char *userinfo)
{
	edict_t	*bot;

	if (!enable_bots) //disabled in comp.ini
		return;

	if (level.modeset != MATCH && level.modeset != PUBLIC || !level.is_spawn_bot)
	{
		safe_ConsolePrintf(PRINT_MEDIUM,"ACE: Cant spawn bot at this time\n");
		return;
	}
	
	bot = ACESP_FindFreeClient ();
	
	if (!bot)
	{
		safe_ConsolePrintf(PRINT_MEDIUM, "ACE: Server is full, increase Maxclients.\n");
		return;
	}

	// hypo add
	//bot->client->resp.is_spawn = true;
	//bot->client->pers.spectator = PLAYING;
	bot->flags &= ~FL_GODMODE;
	bot->health = 0;
	meansOfDeath = MOD_UNKNOWN;
	bot->acebot.new_target = -1;
	bot->acebot.old_target = -1;
	//bot->client->buttons &= ~BUTTON_ATTACK; //

	//end

	bot->yaw_speed = 100; // yaw speed
	bot->inuse = true;
	bot->acebot.is_bot = true;
	strncpy(bot->client->pers.country, "Botville", sizeof(bot->client->pers.country) - 1); //GeoIP2

	// To allow bots to respawn
	if(userinfo == NULL)
		ACESP_SetName(bot, name, skin);
	else
		ACESP_ClientConnect(bot, userinfo);
	
	G_InitEdict (bot);

	// they can connect
	bot->client = game.clients + (bot - g_edicts - 1); //hypov8 add

	InitClientResp (bot->client);

	ACESP_PutClientInServer (bot,0);

	// make sure all view stuff is valid
	ClientEndServerFrame (bot);
	
	ACEIT_PlayerAdded (bot); // let the world know we added another

	ACEAI_PickLongRangeGoal(bot); // pick a new goal

}

///////////////////////////////////////////////////////////////////////
// Remove a bot by name or all bots
///////////////////////////////////////////////////////////////////////
void ACESP_RemoveBot(char *name, qboolean saveBotFile)
{
	int i;
	qboolean freed=false;
	edict_t *bot;

	if (!enable_bots) //disabled in comp.ini
		return;

	if (level.modeset != MATCH && level.modeset != PUBLIC)
		return;

	for(i=0;i<maxclients->value;i++)
	{
		bot = g_edicts + i + 1;
		if(bot->inuse)
		{
			if (bot->acebot.is_bot && (strcmp(bot->client->pers.netname, name) == 0 || strcmp(name, "all") == 0 || strcmp(name, "single") == 0))
			{
				freed = true;
				ACESP_ClientDisconnect(bot);//add hypov8
				//ACEIT_PlayerRemoved (bot);
				safe_bprintf (PRINT_MEDIUM, "%s removed\n", bot->client->pers.netname);

				if (strcmp(name, "single") == 0) //hypov8 remove 1 bot then exit
					break;
			}
		}
	}

	if(!freed)	
		safe_bprintf (PRINT_MEDIUM, "%s not found\n", name);

	//hypov8 fix for player joining and server full. but keeping old bots
	if (saveBotFile)
		ACESP_SaveBots(); // Save them again
}

