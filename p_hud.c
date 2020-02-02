#include "g_local.h"

//GUNRACE_START
#include "gunrace.h"
//GUNRACE_END

static const char *gameheader[] =
{
	GAMEVERSION,
	"by MrDamage Hypo_V8 and G()^T.", //GUNRACE_ADD
	"Additional support from MH and CD", //GUNRACE_ADD
	NULL
};

int GetGameModeMessage(char *entry, int yofs)
{
	const char *p;

	if ((int)teamplay->value == 1)
	{
		if (dm_realmode->value == 2)
			p = "This server is running Rocket Bagman";
		else if (dm_realmode->value)
			p = "This server is running Realmode Bagman";
		else
			p = "This server is running Bagman";
	}
	else if ((int)teamplay->value == 4)
	{
		if (dm_realmode->value == 2)
			p = "This server is running Rocket Team Deathmatch";
		else if (dm_realmode->value)
			p = "This server is running Realmode Team Deathmatch";
		else
			p = "This server is running Team Deathmatch";
	}
	else
	{
		if (dm_realmode->value == 2)
			p = "This server is running Rocket Deathmatch";
		else if (dm_realmode->value)
			p = "This server is running Realmode Deathmatch";
		else
			p = "This server is running Standard Deathmatch Gunrace"; //GUNRACE_ADD
	}
	sprintf (entry, "xm %i yv %i dmstr 874 \"%s\" ",
		-5*strlen(p), yofs + -60-49, p);
	yofs += 20;

	if (level.modeset == MATCH)
		p = "in match mode. Please don't join in.";
	else if (level.modeset == MATCHSETUP)
		p = "in match setup mode.";
	else if (level.modeset == MATCHCOUNT)
		p = "and a match is about to start.";
	else
		p = "in public mode, so please join in.";
	sprintf (entry+strlen(entry), "xm %i yv %i dmstr 874 \"%s\" ",
		-5*strlen(p), yofs + -60-49, p);
	yofs += 20;

	if (level.modeset == PREGAME)
	{
		sprintf (entry+strlen(entry), "xm %i yv %i dmstr 874 \"The game will start shortly.\" ",
			-5*28, yofs + -60-49);
		yofs += 20;
	}

	{
		edict_t	*admin = GetAdmin();
		if (admin || level.modeset == MATCHSETUP)
		{
			char temp[128];
			if (admin)
				sprintf (temp, "Your admin is %s", admin->client->pers.netname);
			else
				strcpy (temp, "No one currently has admin");
			sprintf (entry+strlen(entry), "xm %i yv %i dmstr 874 \"%s\" ",
				-5*strlen(temp), yofs + -60-49, temp );
			yofs += 20;
		}
	}

	yofs += 10;
	return yofs;
}

void GetChaseMessage(edict_t *ent, char *entry)
{
	entry[0]=0;
	if (level.modeset==MATCH && no_spec->value && !ent->client->pers.admin && !ent->client->pers.rconx[0])
	{
		sprintf (entry, 
			"xm %i yb -50 dmstr 773 \"Spectating is disabled\" ",
			-5*22);
	}
	else if (!ent->client->chase_target)
	{
		sprintf (entry, 
			"xm %i yb -50 dmstr 773 \"[hit ACTIVATE to chase]\" ",
			-5*23);
	}
	else if (ent->client->chase_target->client)
	{
		if(ent->client->chasemode == FREE_CHASE)
			sprintf(entry, "xm -180 yb -68 dmstr 773 \"Chasing %s in Freelook Mode\" xm -260 yb -40 dmstr 552 \".. [ and ] cycles, JUMP changes mode, ACTIVATE quits ..\" ",
				ent->client->chase_target->client->pers.netname);
		else if(ent->client->chasemode == EYECAM_CHASE)
			sprintf(entry, "xm -180 yb -68 dmstr 773 \"Chasing %s in Eyecam Mode\" xm -260 yb -40 dmstr 552 \".. [ and ] cycles, JUMP changes mode, ACTIVATE quits ..\" ",
				ent->client->chase_target->client->pers.netname);
		else //if(ent->client->chasemode == LOCKED_CHASE)
			sprintf(entry, "xm -170 yb -68 dmstr 773 \"Chasing %s in Locked Mode\" xm -260 yb -40 dmstr 552 \".. [ and ] cycles, JUMP changes mode, ACTIVATE quits ..\" ",
				ent->client->chase_target->client->pers.netname);
	}
}

static const char *weapnames[8] = { "Pipe", "Pist", "Shot", "Tomm", "HMG", "GL", "RL", "Flam" };


/*
======================================================================

INTERMISSION

======================================================================
*/

void MoveClientToIntermission (edict_t *ent)
{
	if (ent->client->showscores == SCORE_REJOIN)
		ClientRejoin(ent, true); // auto-rejoin
	if (level.modeset != ENDGAMEVOTE || ent->client->resp.enterframe != level.framenum || ent->client->resp.time)
		ent->client->showscores = SCOREBOARD;
	else
		ent->client->showscores = SCORE_MAP_VOTE;
	ent->client->resp.scoreboard_frame = 0;

	VectorCopy (level.intermission_origin, ent->s.origin);
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0]*8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1]*8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2]*8;
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.blend[3] = 0;
	ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	// clean up powerup info
	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->client->breather_framenum = 0;
	ent->client->enviro_framenum = 0;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0;

	// RAFAEL
	ent->client->quadfire_framenum = 0;
	
	ent->viewheight = 0;
	ent->s.modelindex = 0;
//	ent->s.modelindex2 = 0;
//	ent->s.modelindex3 = 0;
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;

	ent->client->chase_target = 0;
	VectorClear (ent->client->ps.viewoffset);
	ent->client->newweapon = ent->client->pers.weapon = NULL;
	ChangeWeapon (ent);

}

void BeginIntermission (edict_t *targ)
{
	int		i, n;
	edict_t	*ent, *client;

	if (level.intermissiontime)
		return;		// already activated

/*
	// respawn any dead clients
	for (i=0 ; i<maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		if (client->health <= 0)
			respawn(client);
	}
*/
	level.intermissiontime = level.time;

	level.changemap = (targ ? targ->map : "");

	if (strstr(level.changemap, "*"))
	{
		{
			for (i=0 ; i<maxclients->value ; i++)
			{
				client = g_edicts + 1 + i;
				if (!client->inuse)
					continue;
				// strip players of all keys between units
				for (n = 0; n < MAX_ITEMS; n++)
				{
					if (itemlist[n].flags & IT_FLASHLIGHT)
						continue;

					if (itemlist[n].flags & IT_KEY)
						client->client->pers.inventory[n] = 0;
				}

				client->episode_flags = client->client->pers.episode_flags = 0;
			}
		}
	}

	level.exitintermission = 0;

	// find an intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if (!ent)
	{	// the map creator forgot to put in an intermission point...
		ent = G_Find (NULL, FOFS(classname), "info_player_start");
		if (!ent)
			ent = G_Find (NULL, FOFS(classname), "info_player_deathmatch");
	}
	else
	{	// chose one of four spots
		i = rand() & 3;
		while (i--)
		{
			ent = G_Find (ent, FOFS(classname), "info_player_intermission");
			if (!ent)	// wrap around the list
				ent = G_Find (ent, FOFS(classname), "info_player_intermission");
		}
	}

	VectorCopy (ent->s.origin, level.intermission_origin);
	VectorCopy (ent->s.angles, level.intermission_angle);

	// move all clients to the intermission point
	for (i=0 ; i<maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		MoveClientToIntermission (client);
	}

	// stop any looping sounds
	for (i=0 ; i<globals.num_edicts ; i++)
	{
		ent = g_edicts + i;
		ent->s.sound = 0;
	}

	// play a music clip
	gi.WriteByte(svc_stufftext);
//	gi.WriteString(va("play world/cypress%i.wav\n", 2+(rand()%4)));
	gi.WriteString("play world/cypress2.wav\n");
	gi.multicast(vec3_origin, MULTICAST_ALL);
}
//===================================================================
//
// Papa - The following are the various scoreboards that I use 
//
//		phear id's confusing format - check qdevels on planetquake if
//		you don't understand this crap
//
//		one hint - dmstr xxx is the rgb color of the text :)
//
//===================================================================
//===================================================================

void SpectatorScoreboardMessage (edict_t *ent)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j, k;
	edict_t	*player;
	char	*tag;

	string[0] = 0;
	stringlength = 0;

	Com_sprintf (entry, sizeof(entry),
		"xm %i yv %i dmstr 999 \"Spectators\" ",
		-5*10, -60-49);
	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	Com_sprintf (entry, sizeof(entry),
		"xm %i yv %i dmstr 663 \"NAME          ping\" ",
		-5*18, -60-21);
	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	for (k=i=0 ; i<maxclients->value ; i++)
	{
		player = g_edicts + 1 + i;
		if (!player->inuse || player->client->pers.spectator != SPECTATING)
			continue;

		if (curtime - player->client->pers.lastpacket >= 5000)
			tag = "666";
		else if (player->client->pers.rconx[0])
			tag = "096";
		else if (player->client->pers.admin > NOT_ADMIN)
			tag = "779";
		else if (player == ent)
			tag = "990";
		else
			tag = "999";	// fullbright

		Com_sprintf (entry, sizeof(entry), "yv %i dmstr %s \"%-13s %4i\" ",
			-60+k*17, tag, player->client->pers.netname, player->client->ping);
		j = strlen(entry);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, entry);
			stringlength += j;
		}
		k++;
	}

	if (k)
		k++;

	for (i=0 ; i<maxclients->value ; i++)
	{
		player = g_edicts + 1 + i;
		if (player->inuse || !player->client || !player->client->pers.connected || !(kpded2 || curtime - player->client->pers.lastpacket < 120000))
			continue;

		tag = "666";

		Com_sprintf (entry, sizeof(entry), "yv %i dmstr %s \"%-13s CNCT\" ",
			-60+k*17, tag, player->client->pers.netname);
		j = strlen(entry);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, entry);
			stringlength += j;
		}
		k++;
	}

	if (level.modeset == ENDGAMEVOTE)
	{
		static const char *votenote = "xm -230 yb -40 dmstr 552 \"[Hit your scoreboard key (f1) for the vote menu]\" ";
		j = strlen(votenote);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, votenote);
			stringlength += j;
		}
	}

	if (ent->client->pers.spectator == SPECTATING && (level.modeset == PUBLIC || level.modeset == MATCH))
	{
		GetChaseMessage(ent,entry);
		j = strlen(entry);
		if (j && stringlength + j < 1024)
		{
			strcpy (string + stringlength, entry);
			stringlength += j;
		}
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
}

//===================================================================
//===================================================================
//===================================================================

void VoteMapScoreboardMessage (edict_t *ent)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j, w;
	int		yofs;
	int		count[9];
	edict_t *player;
	const char	*selectheader[] =
		{
			"Please vote your choice for the next map.",
			"Hit the corresponding number (weapon key)",
			"or use [ and ] to place your vote.",
			NULL
		};
	const char	*basechoice[] =
		{
			"1 (Pipe)     ",
			"2 (Pistol)   ",
			"3 (Shotgun)  ",
			"4 (Tommygun) ",
			"5 (HMG)      ",
			"6 (GL)       ",
			"7 (Bazooka)  ",
			"8 (FlameThr) ",
			NULL
		};

	string[0] = 0;
	stringlength = 0;
	yofs = -60-49;
	if (ent->client->pers.screenwidth >= 1152)
		yofs -= 40;

	Com_sprintf (entry, sizeof(entry),"xm %i ", -5*41);
	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	for (i=0; selectheader[i]; i++)
	{
		Com_sprintf (entry, sizeof(entry),"yv %i dmstr 863 \"%s\" ",
			yofs, selectheader[i] );
		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;
		yofs += 20;
	}

	memset (&count, 0, sizeof(count));
	for_each_player_not_bot(player, i)
	{
		count[player->client->mapvote]++;
	}

	// if the screen is wide enough, show all map pics
	if (ent->client->pers.screenwidth >= 1152)
	{
		yofs += 30;

		w = (ent->client->pers.screenwidth - 200) / 4;
		if (w > 270)
			w = 270;

		for (i=0; i < 8; i++)
		{
			int n = (i & 1 ? 5 : 1) + i / 2;
			if (n > level.num_vote_set)
				continue;
			if (!(i & 1))
			{
				Com_sprintf (entry, sizeof(entry), "xm %i ", ((i / 2) - 2) * w + (w - 192) / 2);
				j = strlen(entry);
				strcpy (string + stringlength, entry);
				stringlength += j;
			}
			if (level.vote_winner && n != level.vote_winner)
			{
				char *col = (ent->client->mapvote == n ? "770" : "777");
				if (count[n])
					Com_sprintf (entry, sizeof(entry), "yv %i dmstr %s \"%d. %s\" yv %i dmstr %s \"%d %s\" ",
						yofs + (i & 1 ? 190 : 0), col, n, maplist[level.vote_set[n]],
						yofs + (i & 1 ? 190 : 0) + 19, col, count[n], count[n] == 1 ? "vote" : "votes");
	else
					Com_sprintf (entry, sizeof(entry), "yv %i dmstr %s \"%d. %s\" ",
						yofs + (i & 1 ? 190 : 0), col, n, maplist[level.vote_set[n]]);
			}
			else
			{
				char *col = (ent->client->mapvote == n ? "990" : "999");
				if (count[n])
					Com_sprintf (entry, sizeof(entry), "yv %i dmstr %s \"%d. %s\" yv %i dmstr %s \"%d %s\" yv %i picn %s ",
						yofs + (i & 1 ? 190 : 0), col, n, maplist[level.vote_set[n]],
						yofs + (i & 1 ? 190 : 0) + 19, col, count[n], count[n] == 1 ? "vote" : "votes",
						yofs + (i & 1 ? 190 : 0) + 39, level.vote_nopic[n] ? "mm/nopic" : maplist[level.vote_set[n]]);
				else
					Com_sprintf (entry, sizeof(entry), "yv %i dmstr %s \"%d. %s\" yv %i picn %s ",
						yofs + (i & 1 ? 190 : 0), col, n, maplist[level.vote_set[n]],
						yofs + (i & 1 ? 190 : 0) + 39, level.vote_nopic[n] ? "mm/nopic" : maplist[level.vote_set[n]]);
			}
			j = strlen(entry);
			strcpy (string + stringlength, entry);
			stringlength += j;
		}

		if (count[0])
		{
			if (ent->client->mapvote)
				Com_sprintf (entry, sizeof(entry), "xm %i yv %i dmstr 777 \"         %d players have not voted\" ",
								-5*24, yofs + 380, count[0]);
					else
						Com_sprintf (entry, sizeof(entry), "xm %i yv %i dmstr 990 \"You have not voted\" ",
								-5*18, yofs + 380);
			j = strlen(entry);
			if (stringlength + j < 1024)
			{
				strcpy (string + stringlength, entry);
				stringlength += j;
			}
		}
	}
	else
	{
		yofs += 10;

		if (ent->client->mapvote == 0)
			Com_sprintf (entry, sizeof(entry), "yv %i dmstr %s \"-->      %d players have not voted\" ",
					yofs, level.vote_winner ? "770" : "990", count[0]);
		else if (count[0])
			Com_sprintf (entry, sizeof(entry), "yv %i dmstr 777 \"         %d players have not voted\" ",
					yofs, count[0]);
		else
			entry[0] = 0;
		if (entry[0])
		{
			j = strlen(entry);
			strcpy (string + stringlength, entry);
			stringlength += j;
		}

	yofs += 30;

		for (i=1; i <= level.num_vote_set; i++)
		{
			if (ent->client->mapvote == i)
					Com_sprintf (entry, sizeof(entry), "yv %i dmstr %s \"--> %s %d %s - %s\" ",
						yofs, level.vote_winner && i != level.vote_winner ? "770" : "990", basechoice[i-1], count[i], count[i] == 1 ? "vote " : "votes", maplist[level.vote_set[i]]);
			else
					Com_sprintf (entry, sizeof(entry), "yv %i dmstr %s \"    %s %d %s - %s\" ",
						yofs, level.vote_winner && i != level.vote_winner ? "777" : "999", basechoice[i-1], count[i], count[i] == 1 ? "vote " : "votes", maplist[level.vote_set[i]]);
			j = strlen(entry);
			if (stringlength + j < 1024)
			{
				strcpy (string + stringlength, entry);
				stringlength += j;
			}
			yofs += 20;
		}

		//if client has voted for a map, print this maps levelshot
		if (ent->client->mapvote > 0 && !level.vote_nopic[ent->client->mapvote])
		{
			yofs += 15;
			//print the pcx levelshot on screen
			//name of pcx must be same as bsp -- pcx files must be in pics dir
			Com_sprintf (entry, sizeof(entry),
				"xm %i yv %i picn %s ",
					-5*20, yofs, maplist[level.vote_set[ent->client->mapvote]]);
			j = strlen(entry);
			if (stringlength + j < 1024)
			{
				strcpy(string + stringlength, entry);
				stringlength += j;
			}
		}
	}

	{
		static const char *votenote = "xm -230 yb -40 dmstr 552 \"[Hit your scoreboard key (f1) for the scoreboard]\" ";
		j = strlen(votenote);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, votenote);
			stringlength += j;
		}
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
}

//===================================================================
//===================================================================
//===================================================================

void MOTDScoreboardMessage (edict_t *ent)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j;
	int		yofs;
	const char	*seperator = "==================================";

	string[0] = 0;
	stringlength = 0;

	yofs = 110 - num_MOTD_lines * 10;
	if (yofs < 0 )
		yofs = 0;

	Com_sprintf (entry, sizeof(entry), "xm 65 yv %i picn /pics/mmod/gr_score.tga ", yofs - 75 + -60-49); //hypov8
	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	for (i=0; gameheader[i]; i++)
	{
		Com_sprintf (entry, sizeof(entry),
			"xm %i yv %i dmstr 863 \"%s\" ",
			-5*strlen(gameheader[i]), yofs + -60-49, gameheader[i] );
		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;
		yofs += 20;
	}
	yofs += 10;

	Com_sprintf (entry, sizeof(entry),
		"xm %i yv %i dmstr 772 \"%s\" ",
		-5*strlen(seperator), yofs + -60-49, seperator );
	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;
	yofs += 30;

	if (num_MOTD_lines)
	{
		for (i=0; i<num_MOTD_lines; i++)
		{
			Com_sprintf (entry, sizeof(entry),
				"xm %i yv %i dmstr 953 \"%s\" ",
				-5*strlen(MOTD[i].textline), yofs + -60-49, MOTD[i].textline );
			j = strlen(entry);
			if (stringlength + j < 1024)
			{
				strcpy (string + stringlength, entry);
				stringlength += j;
			}
			yofs += 20;
		}
		yofs += 10;
		Com_sprintf (entry, sizeof(entry),
			"xm %i yv %i dmstr 772 \"%s\" ",
			-5*strlen(seperator), yofs + -60-49, seperator );
		j = strlen(entry);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, entry);
			stringlength += j;
		}
		yofs += 30;
	}

	i=GetGameModeMessage(entry, yofs);
	j = strlen(entry);
	if (stringlength + j < 1024)
	{
		strcpy (string + stringlength, entry);
		stringlength += j;
		yofs = i;
	}

	if (ent->client->pers.version < 121)
	{
		yofs += 30;
		Com_sprintf (entry, sizeof(entry), "xm %i yv %i dmstr 990 \"Warning: You are using an old version of Kingpin.\" "
			"xm %i yv %i dmstr 990 \"You should download and install the v1.21 patch.\" ",
			-5*49, yofs + -60-49, -5*49, yofs + -60-49+20);
		j = strlen(entry);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, entry);
			stringlength += j;
		}
	}
	else
	{
		int rate = atoi(Info_ValueForKey(ent->client->pers.userinfo, "rate"));
		if (rate < 15000)
		{
			yofs += 30;
			Com_sprintf (entry, sizeof(entry), "xm %i yv %i dmstr 990 \"Warning: You currently have 'rate' set to only %d.\" "
				"xm %i yv %i dmstr 990 \"If you have broadband, at least 15000 is recommended.\" ",
				-5*52, yofs + -60-49, rate, -5*52, yofs + -60-49+20);
			j = strlen(entry);
			if (stringlength + j < 1024)
			{
				strcpy (string + stringlength, entry);
				stringlength += j;
			}
		}
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
}

//===================================================================
//===================================================================
//===================================================================

void RejoinScoreboardMessage (edict_t *ent)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j;
	int		yofs;
	const char	*seperator = "==================================";
	const char	*rejoinheader[] =
		{
			"You were just playing on this server.",
			"Would you like to continue where you left off?",
			NULL
		};
	const char	*choices[] =
		{
			"1 - Yes",
			"2 - No",
			NULL
		};

	string[0] = 0;
	stringlength = 0;
	yofs = 100;

	Com_sprintf (entry, sizeof(entry), "xm 65 yv %i picn /pics/mmod/gr_score.tga ", yofs - 75 + -60-49); //hypov8
	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	for (i=0; gameheader[i]; i++)
	{
		Com_sprintf (entry, sizeof(entry),
			"xm %i yv %i dmstr 863 \"%s\" ",
			-5*strlen(gameheader[i]), yofs + -60-49, gameheader[i] );
		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;
		yofs += 20;
	}
	yofs += 10;

	Com_sprintf (entry, sizeof(entry),
		"xm %i yv %i dmstr 772 \"%s\" ",
		-5*strlen(seperator), yofs + -60-49, seperator );
	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;
	yofs += 30;

	for (i=0; rejoinheader[i]; i++)
	{
		Com_sprintf (entry, sizeof(entry),
			"xm %i yv %i dmstr 999 \"%s\" ",
			-5*strlen(rejoinheader[i]), yofs + -60-49, rejoinheader[i] );
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
		yofs += 20;
	}
	yofs += 30;

	for (i=0; choices[i]; i++)
	{
		Com_sprintf (entry, sizeof(entry), "xm %i yv %i dmstr 999 \"    %s\" ",
			-5*15, yofs + -60-49, choices[i]);
		j = strlen(entry);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, entry);
			stringlength += j;
		}
		yofs += 20;
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
}

/*
==================
GrabDaLootScoreboardMessage

==================
*/
void GrabDaLootScoreboardMessage (edict_t *ent)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j, k;
	int		sorted[MAX_CLIENTS] = {0};
	int		sortedscores[MAX_CLIENTS];
	int		score, total, real_total;
	int		x;
	gclient_t	*cl;
	edict_t		*cl_ent;
	char	*tag;
	int		team;
	const char *header;
	const char	*headerb = "NAME         ping hits depst ";
	const char	*headerd = "NAME         ping hits deaths";
	const char	*headera = "NAME         ping  acc   fav ";
	const char	*headers = "NAME          acc  fav stole ";
	int		yofs=0, avping = 0, tp = 0, tc = 0;
	int		tmax;
	int setup = (level.modeset == PREGAME || (level.modeset == MATCHSETUP && (!level.intermissiontime || level.framenum >= level.startframe + 150)));

// ACEBOT_ADD
	if (ent->acebot.is_bot)
		return;
// ACEBOT_END

	if (ent->client->showscores == SCOREBOARD)
		header = teamplay->value == 4 ? headerd : headerb;
	else
		header = teamplay->value == 4 ? headera : headers;

	x = (-1*strlen(header) - 2) * 10;	// 10 pixels per char

	string[0] = 0;
	stringlength = 0;

	if (!ent->client->showscores)
		goto skipscores;

	Com_sprintf (entry, sizeof(entry), "xm %i yt 5 dmstr 990 \"Map: %s\" ", -5*strlen(level.mapname), level.mapname);
	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	if (ent->client->showscores == SCOREBOARD)
	{
		if (setup || level.modeset == MATCHCOUNT || (ent->client->pers.spectator == SPECTATING && level.modeset == MATCH))
		{
			yofs=GetGameModeMessage(entry, yofs);
			j = strlen(entry);
			strcpy (string + stringlength, entry);
			stringlength += j;
		}
		
		if (setup || (ent->client->pers.spectator == SPECTATING && level.modeset != MATCHCOUNT && level.modeset != MATCH && !level.intermissiontime))
		{
			// print the team selection header
			Com_sprintf (entry, sizeof(entry),
				"xm %i yv %i dmstr 999 \"Press the corresponding number to join a team:\" ",
				-5*46, yofs + -60-49);
			j = strlen(entry);
			strcpy (string + stringlength, entry);
			stringlength += j;
			yofs += 30;

			for (team=1; team<=2; team++)
			{
				Com_sprintf (entry, sizeof(entry),
					"xm %i yv %i dmstr 999 \"%i - %s\" ",
					-9*10, yofs + -60-49, team, team_names[team]);
				j = strlen(entry);
				strcpy (string + stringlength, entry);
				stringlength += j;
				yofs += 20;
			}
			yofs += 10;
		}
		else if (level.modeset == MATCHSETUP && level.intermissiontime)
		{
			Com_sprintf (entry, sizeof(entry), "xm %i yv %i dmstr 999 \"Match Result\" ",
				-5*12, yofs + -60-49);
			j = strlen(entry);
			strcpy (string + stringlength, entry);
			stringlength += j;
			yofs += 30;
		}
	}

	tmax = (1024 - 300 - stringlength) / (ent->client->showscores == SCOREBOARD ? 24 : 48) / 2;

	for (team=1; team<=2; team++)
	{
		// Team header
		Com_sprintf (entry, sizeof(entry),
			"xm %i yv %i tscore %i xm %i dmstr 677 \"%s\" teampic %i ",
			x+14*10, yofs + -60-49, team, x, team_names[team], team );
		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;

		// sort the clients by score
		total = 0;
		for (i=0 ; i<game.maxclients ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || game.clients[i].pers.team != team)
				continue;

			if (teamplay->value==4) 
				score = (game.clients[i].resp.score<<8) - game.clients[i].resp.deposited;
			else
				score = (game.clients[i].resp.deposited<<4) + game.clients[i].resp.score;

			for (j=0 ; j<total ; j++)
			{
				if (score > sortedscores[j])
					break;
			}
			for (k=total ; k>j ; k--)
			{
				sorted[k] = sorted[k-1];
				sortedscores[k] = sortedscores[k-1];
			}
			sorted[j] = i;
			sortedscores[j] = score;
			total++;
		}

		real_total = total;
		if (total>tmax) total=tmax;

		Com_sprintf (entry, sizeof(entry),
			"yv %i dmstr 663 \"%s\" ",
			yofs + -60-21, header );
		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;

		for (i=0 ; i<total ; i++)
		{
			cl = &game.clients[sorted[i]];
			cl_ent = g_edicts + 1 + sorted[i];

			if (cl_ent == ent)
				tag = "990";
			else if (cl_ent->client->pers.rconx[0])
				tag = "096";
			else if (cl_ent->client->pers.admin > NOT_ADMIN)
				tag = "779";
			else
				tag = "999";	// fullbright

			if (ent->client->showscores == SCOREBOARD)
			{
				Com_sprintf (entry, sizeof(entry),
					"yv %i ds %s %i %i %i %i ",
					yofs + -60+i*17, tag, sorted[i], cl->ping, cl->resp.score, cl->resp.deposited);
			}
			else
			{
				int fc = 0;
				const char *fn = "-";
				for (j=0; j<8; j++)
				{
					if (cl->resp.fav[j] > fc)
					{
						fc = cl->resp.fav[j];
						fn = weapnames[j];
					}
				}
				if (teamplay->value == 4)
					Com_sprintf (entry, sizeof(entry),
						"yv %i dmstr %s \"%-13s%4i %4i  %4s\" ",
						yofs + -60+i*17, tag, cl->pers.netname, cl->ping,
						cl->resp.accshot ? cl->resp.acchit * 1000 / cl->resp.accshot : 0, fn);
				else
					Com_sprintf (entry, sizeof(entry),
						"yv %i dmstr %s \"%-13s%4i %4s %5i\" ",
						yofs + -60+i*17, tag, cl->pers.netname,
						cl->resp.accshot ? cl->resp.acchit * 1000 / cl->resp.accshot : 0, fn, cl->resp.stole);
			}
			tp += cl->ping;
			tc++;
			j = strlen(entry);
			if (stringlength + j > 1024)
				break;
			strcpy (string + stringlength, entry);
			stringlength += j;
		}

		if (real_total > i || (ent->client->showscores == SCOREBOARD && real_total > 1))
		{	// show the nuber of undisplayed players
			avping = tp / tc;
			Com_sprintf (entry, sizeof(entry),
				ent->client->showscores == SCOREBOARD ? "yv %i dmstr 777 \"(%i players, %i avg)\" " : "yv %i dmstr 777 \"(%i players)\" ",
				yofs + -60+i*17 + 6, real_total, avping );
			j = strlen(entry);
			if (stringlength + j > 1024)
				break;
			strcpy (string + stringlength, entry);
			stringlength += j;
		}

		x = -8;
		avping = tp = tc = 0;
	}

	if (level.modeset == ENDGAMEVOTE)
	{
		static const char *votenote = "xm -230 yb -40 dmstr 552 \"[Hit your scoreboard key (f1) for the vote menu]\" ";
		j = strlen(votenote);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, votenote);
			stringlength += j;
		}
	}

skipscores:
	
	if (ent->client->pers.spectator == SPECTATING && (level.modeset == PUBLIC || level.modeset == MATCH))
	{
		GetChaseMessage(ent,entry);
		j = strlen(entry);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, entry);
			stringlength += j;
		}
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
}

//GUNRACE_START
//START OF G()^T MESSAGEBOARD
void GoatScoreboardMessage (edict_t *ent)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	/* These three are used for storing and keeping track of the string that will eventually be written to gi. 
	Stringlength is just to make sure we don't create a string that is too long for the array */ 

	int		j,  newColour;
	int xScore, xKillToNext, xWepIndex; //add hypov8

	xScore = ent->client->resp.score;
	xKillToNext = ent->client->resp.killtonext;
	xWepIndex = ent->client->resp.curwepIndex;

	if (ent->client->chase_target){
		xScore = ent->client->chase_target->client->resp.score;
		xKillToNext = ent->client->chase_target->client->resp.killtonext;
		xWepIndex = ent->client->chase_target->client->resp.curwepIndex;
	}

	newColour = 990; //yellow
	if (ent->client->resp.countWepChange > level.framenum)
		newColour = 900; //red

	/* A string for the dog tag (shows who you are and who last killed you */ 
	string[0] = 0;
	stringlength = 0;

	Com_sprintf(entry, sizeof(entry),

		//=================
		// frags
		//=================
		"hud xr	-45 "
		"yt	15 "
		"dmstr 990 %i " //hypov8 'frags' causing issue

		//=================
		// frag to next
		//=================
		"xr	-45 "
		"yt	45 "
		"dmstr 990 %i "

		//=================
		// weapon title
		//=================
		"xr -130 "//hypov8 menu x_right
		"yt	20 string \"Kills:\" "
		"yt	50 string \"to Next:\" "
		"yt	80 string \"Current Weapon\" "
		"yt	130 string \"Next Weapon\" "

		//=================
		// weapon names
		//=================
		"xr -130 "//hypov8 menu x_right
		"yt 90 dmstr %i \"%s\" "// curweap
		"yt 140 dmstr 990 \"%s\" " // nexweap
		, xScore
		, xKillToNext
		, newColour
		, grWeps[xWepIndex].grWepNameHUD //ent->client->resp.curweap
		, grWeps[xWepIndex + 1].grWepNameHUD //ent->client->resp.nexweap
		);



	j = strlen(entry);
	if (stringlength+j < 1024)
	{
		strcpy(string + stringlength, entry);
		stringlength += j;
	}

	if (ent->client->message_frame > level.framenum)
	{
		int i, score, rank = 1;
		gclient_t	*cl;
		edict_t		*cl_ent;
		char	string2[1400];
		int entscore = (ent->client->resp.score<<8) - ent->client->resp.deposited;

		for (i=0 ; i<game.maxclients ; i++)
		{
			cl = &game.clients[i];
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl->pers.spectator == SPECTATING)
				continue;
			score = (cl->resp.score<<8) - cl->resp.deposited;
			if (score > entscore)
				rank++;
		}
		if (ent->client->message_count > 1)
			Com_sprintf (entry, sizeof(entry), "%s FRAG!", ent->client->message_count == 2 ? "DOUBLE" : ent->client->message_count == 3 ? "TRIPLE" : "MULTI");
		else
			Com_sprintf (entry, sizeof(entry), "%s %s", ent->client->message_bonus ? "REVENGE Frag on" : "You fragged", ent->client->message_name);
		Com_sprintf (string2, sizeof(string2), "xm %i yv -50 dmstr 999 \"%s\" ", -5*strlen(entry), entry);
		j = strlen(string2);
		if (rank <= 20)
		{
			Com_sprintf (entry, sizeof(entry), "%d%s place", rank, rank == 1 ? "st" : rank == 2 ? "nd" : rank == 3 ? "rd" : "th");
			Com_sprintf (string2 + j, sizeof(string2) - j, "xm %i yv -29 dmstr 888 \"%s\" ", -5* strlen(entry), entry);
			j = strlen(string2);
		}

		//check overflow
		if (stringlength+j < 1024)
		{
			strcpy(string + stringlength, string2);
			stringlength += j;
		}
	}



	if (ent->client->pers.spectator == SPECTATING && (level.modeset == PUBLIC || level.modeset == MATCH))
	{
		GetChaseMessage(ent,entry);
		j = strlen(entry);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, entry);
			stringlength += j;
		}
	}
	
	gi.WriteByte (svc_layout);
	gi.WriteString (string);

}
//END OF G()^T MESSAGEBOARD
//GUNRACE_

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage (edict_t *ent)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j, k;
	int		sorted[MAX_CLIENTS] = {0};
	int		sortedscores[MAX_CLIENTS];
	int		score, total, realtotal;
	gclient_t	*cl;
	edict_t		*cl_ent;
	char	*tag;
	int		tmax;

	//GUNRACE_ADD
	char	entry2[1024];
	char	weaponordertype[16];
	int		mode = (int)weaponorder->value;
	//GUNRACE_END

	// ACEBOT_ADD
	if (ent->acebot.is_bot)
		return;
	// ACEBOT_END

	//GUNRACE_ADD
	if (mode == 1)			strcpy(weaponordertype, "Mode: Standard");
	else if (mode == 2)		strcpy(weaponordertype, "Mode: Reverse");
	else if (mode == 3)		strcpy(weaponordertype, "Mode: Random");
	else if (mode == 4)		strcpy(weaponordertype, "Mode: MachGuns");
	else if (mode == 5)		strcpy(weaponordertype, "Mode: Shotty");
	//GUNRACE_END

	string[0] = 0;
	stringlength = 0;

	if (!ent->client->showscores)
	{
//MH death msg
#if 0
		if (ent->client->message_frame > level.framenum)
		{
			int rank = 1;
			int entscore = (ent->client->resp.score<<8) - ent->client->resp.deposited;
			for (i=0 ; i<game.maxclients ; i++)
			{
				cl = &game.clients[i];
				cl_ent = g_edicts + 1 + i;
				if (!cl_ent->inuse || cl->pers.spectator == SPECTATING)
					continue;
				score = (cl->resp.score<<8) - cl->resp.deposited;
				if (score > entscore)
					rank++;
			}
			if (ent->client->message_count > 1)
				Com_sprintf (entry, sizeof(entry), "%s FRAG!", ent->client->message_count == 2 ? "DOUBLE" : ent->client->message_count == 3 ? "TRIPLE" : "MULTI");
			else
				Com_sprintf (entry, sizeof(entry), "You fragged %s%s", ent->client->message_name, ent->client->message_bonus ? " and got a bonus hit!" : "");
			Com_sprintf (string, sizeof(string), "hud xm %i yv -50 dmstr 999 \"%s\" ", -5*strlen(entry), entry);
			stringlength = strlen(string);
			if (rank <= 20)
			{
				Com_sprintf (entry, sizeof(entry), "%d%s place", rank, rank == 1 ? "st" : rank == 2 ? "nd" : rank == 3 ? "rd" : "th");
				Com_sprintf (string + stringlength, sizeof(string) - stringlength, "xm %i yv -29 dmstr 888 \"%s\" ", -5*strlen(entry), entry);
				stringlength = strlen(string);
			}
		}
#endif
		goto skipscores;
	}
	
		
	// sort the clients by score
	total = 0;
	for (i=0 ; i<game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || (cl_ent->client->pers.spectator == SPECTATING && !level.intermissiontime))
			continue;

		if (fph_scoreboard)
			score = game.clients[i].resp.time ? game.clients[i].resp.score * 36000 / game.clients[i].resp.time : 0;
		else
			score = (game.clients[i].resp.score<<8) - game.clients[i].resp.deposited;

		for (j=0 ; j<total ; j++)
		{
			if (score > sortedscores[j])
				break;
		}
		for (k=total ; k>j ; k--)
		{
			sorted[k] = sorted[k-1];
			sortedscores[k] = sortedscores[k-1];
		}
		sorted[j] = i;
		sortedscores[j] = score;
		total++;
	}

	realtotal = total;

	Com_sprintf (entry, sizeof(entry), "xm %i yt 5 dmstr 990 \"Map: %s\" ", -5*strlen(level.mapname), level.mapname);
	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	//GUNRACE_ADD
	Com_sprintf(entry2, sizeof(entry2), " xm %i yt 25 dmstr 990 \"%s\" ", -5 * strlen(level.mapname), weaponordertype);
	j = strlen(entry2);
	strcpy(string + stringlength, entry2);
	stringlength += j;
	//GUNRACE_END


	// Send the current leader
	if (total && game.clients[sorted[0]].resp.time)
	{
		Com_sprintf (entry, sizeof(entry),
			"leader %i ",
			sorted[0] );
		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	// header
	if (ent->client->showscores == SCOREBOARD)
	{
		if (fph_scoreboard)
			Com_sprintf (entry, sizeof(entry),
				"xr %i yv %i dmstr 663 \"NAME         ping hits   fph\" ",
				-36*10 - 10, -60+-21 );
		else
			Com_sprintf (entry, sizeof(entry),
				"xr %i yv %i dmstr 663 \"NAME         ping time  hits\" ",
				-36*10 - 10, -60+-21 );
	}
	else
//GUNRACE_START
#if 0
//GUNRACE_END
		Com_sprintf (entry, sizeof(entry),
			"xr %i yv %i dmstr 663 \"NAME        deaths  acc  fav\" ",
			-36*10 - 10, -60+-21 );

//GUNRACE_START
#else
		Com_sprintf (entry, sizeof(entry),
			"xr %i yv %i dmstr 663 \"NAME        deaths  acc   wep\" ",
			-36*10 - 10, -60+-21 );
#endif
//GUNRACE_END
	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	tmax = (1024 - stringlength) / 54; //GUNRACE_ADD //hypov8 increase max player string length
	if (total > tmax) 
		total = tmax - 1;	//allows for "(# players)" added to end of list

	for (i=0 ; i<total ; i++)
	{
		cl = &game.clients[sorted[i]];
		cl_ent = g_edicts + 1 + sorted[i];

		if (cl_ent == ent)
			tag = "990";
		else if (cl_ent->client->pers.rconx[0])
			tag = "096";
		else if (cl_ent->client->pers.admin > NOT_ADMIN)
			tag = "779";
// ACEBOT_ADD
		else if (cl_ent->acebot.is_bot) 
			tag = "966";
// ACEBOT_END
		else  
			tag = "999";	// fullbright

		if (ent->client->showscores == SCOREBOARD)
		{
			if (fph_scoreboard)
				Com_sprintf (entry, sizeof(entry),
					"yv %i ds %s %i %i %i %i ",
					-60+i*17, tag, sorted[i], cl->ping, cl->resp.score, cl->resp.time ? cl->resp.score * 36000 / cl->resp.time : 0);
			else
				Com_sprintf (entry, sizeof(entry),
					"yv %i ds %s %i %i %i %i ",
					-60+i*17, tag, sorted[i], cl->ping, cl->resp.time/600, cl->resp.score);
		}
		else //SCOREBOARD2
		{
//GUNRACE_START
#if 0
//GUNRACE_END
			int fc = 0;
			const char *fn = "-";
			for (j = 0; j<8; j++)
			{
				if (cl->resp.fav[j] > fc)
				{
					fc = cl->resp.fav[j];
					fn = weapnames[j];
				}
			}
			Com_sprintf(entry, sizeof(entry),
				"yv %i dmstr %s \"%-13s %4i %4i %4s\" ",
				-60 + i * 17, tag, cl->pers.netname, cl->resp.deposited, cl->resp.accshot ? cl->resp.acchit * 1000 / cl->resp.accshot : 0, fn);
//GUNRACE_START
#else
			Com_sprintf(entry, sizeof(entry),
				"yv %i dmstr %s \"%-13s %4i %4i %5s\" ",
				-60 + i * 17, tag, cl->pers.netname, cl->resp.deposited,
				cl->resp.accshot ? cl->resp.acchit * 1000 / cl->resp.accshot : 0, grWeps[cl->resp.curwepIndex].grWepNameMenu); //hypov8 display current weapon	
#endif
//GUNRACE_END

		}
		j = strlen(entry);
		if (stringlength + j < 990){ //add hypov8. should never happen. failsafe
			strcpy(string + stringlength, entry);
			stringlength += j;
		}
	}

	if (realtotal > i)
	{
		Com_sprintf (entry, sizeof(entry),
			"yv %i dmstr 777 \"(%i players)\" ",
			-60+i*17+6, realtotal );
		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	if (level.modeset == ENDGAMEVOTE)
	{
		static const char *votenote = "xm -230 yb -40 dmstr 552 \"[Hit your scoreboard key (f1) for the vote menu]\" ";
		j = strlen(votenote);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, votenote);
			stringlength += j;
		}
	}

skipscores:
	
	if (ent->client->pers.spectator == SPECTATING && (level.modeset == PUBLIC || level.modeset == MATCH))
	{
		GetChaseMessage(ent,entry);
		j = strlen(entry);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, entry);
			stringlength += j;
		}
	}
	

	gi.WriteByte (svc_layout);
	gi.WriteString (string);

}




/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the 1400 byte message limit!
==================
*/

// Papa - Here is where i determine what scoreboard to display

void DeathmatchScoreboard (edict_t *ent)
{
	
	if (ent->client->showscores == SCORE_MAP_VOTE && level.modeset != ENDGAMEVOTE)
		ent->client->showscores = NO_SCOREBOARD;

// ACEBOT_ADD
	if (ent->acebot.is_bot)
		ent->client->showscores = NO_SCOREBOARD;
// ACEBOT_END

	if (ent->client->showscores == SCORE_MOTD)
		MOTDScoreboardMessage (ent);
	else if (ent->client->showscores == SCORE_REJOIN)
		RejoinScoreboardMessage (ent);
	else if (ent->client->showscores == SPECTATORS)
		SpectatorScoreboardMessage (ent);
	else if (ent->client->showscores == SCORE_MAP_VOTE)
		VoteMapScoreboardMessage(ent);
	//GUNRACE_START
	else if((ent->client->showscores == SCORE_GUNRACE_STATS && ent->client->pers.spectator != SPECTATING)
		|| (ent->client->showscores == SCORE_GUNRACE_STATS && ent->client->pers.spectator == SPECTATING && ent->client->chase_target)) //add hypov8 show spec'd stats
		GoatScoreboardMessage(ent);
	//GUNRACE_END
	else if (teamplay->value)
		GrabDaLootScoreboardMessage (ent);
	else
		DeathmatchScoreboardMessage (ent);
// ACEBOT_ADD
	if (!ent->acebot.is_bot)
// ACEBOT_END
	gi.unicast (ent, !ent->client->resp.scoreboard_frame);

	ent->client->resp.scoreboard_frame = level.framenum;
	if (ent->client->showscores == SCORE_REJOIN)
		ent->client->resp.scoreboard_frame += 10000; // no need to refresh rejoin message
}

/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/

// Papa - This is the start of the scoreboard command, this sets the showscores value

void Cmd_Score_f (edict_t *ent)
{
	int		i;
	edict_t	*dood;

	ent->client->showinventory = false;
	ent->client->message_frame = 0; //MH Score
	if (ent->client->showscores == SCORE_REJOIN)
	{
		ClientRejoin(ent, false);
		return;
	}

	if (ent->client->showscores == SCOREBOARD)
	{
		for_each_player(dood, i)
		{
			if (dood->client->pers.spectator != SPECTATING && dood->client->resp.time)
			{
				ent->client->showscores = SCOREBOARD2;
				break;
			}
		}
		if (ent->client->showscores != SCOREBOARD2) goto skipscoreboard2;

	}
	else if (ent->client->showscores == SCOREBOARD2)
	{
skipscoreboard2:
		ent->client->showscores = NO_SCOREBOARD;	
		
		
		if (teamplay->value || !level.intermissiontime)
		{
			for (i=0 ; i<maxclients->value ; i++)
			{
				dood = g_edicts + 1 + i;
				if (dood->client && ((dood->inuse && dood->client->pers.spectator == SPECTATING) || (!dood->inuse && dood->client->pers.connected && (kpded2 || curtime - dood->client->pers.lastpacket < 120000))))
				{
					ent->client->showscores = SPECTATORS;
					break;
				}
			}
		}
	}
	else if (ent->client->showscores == SPECTATORS)
		ent->client->showscores = NO_SCOREBOARD;
	else
		ent->client->showscores = SCOREBOARD; 

	

	if (ent->client->showscores == NO_SCOREBOARD && (level.intermissiontime
			|| (level.modeset == MATCH && no_spec->value && !ent->client->pers.admin && !ent->client->pers.rconx[0])))
	{
		if (level.modeset == ENDGAMEVOTE)
			ent->client->showscores = SCORE_MAP_VOTE;
		else
			ent->client->showscores = SCOREBOARD; 

	}

//GUNRACE_START
	gr_setScoreboardGunrace(ent->client);
//GUNRACE_END

	ent->client->resp.scoreboard_frame = 0;

//GUNRACE_START
	DeathmatchScoreboard(ent);
//GUNRACE_END
}



/*
==================
Cmd_Help_f

Display the current help message
==================
*/
void Cmd_Help_f (edict_t *ent, int page)
{
	// ACEBOT_ADD //new kpded.exe
	if (ent->acebot.is_bot) return;
	// ACEBOT_END
	// this is for backwards compatability
	Cmd_Score_f (ent);
}


//=======================================================================

/*
===============
G_SetStats
===============
*/
void G_SetStats(edict_t *ent)
{
	gitem_t		*item;


	// if chasecam, show stats of player we are following
	if (ent->client->chase_target && ent->client->chase_target->client)
	{
		memcpy(ent->client->ps.stats, ent->client->chase_target->client->ps.stats, sizeof(ent->client->ps.stats));
		ent->client->ps.stats[STAT_LAYOUTS] = true;


		// keep own score
		if (teamplay->value)//hypov8. not used in GR hud //GUNRACE_ADD
		{
			ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;
			ent->client->ps.stats[STAT_DEPOSITED] = ent->client->resp.deposited;
		}

		return;
	}

	//
	// health
	//
	// JOSEPH 23-MAR-99
	{
		int     index1, index2, index3;

		item = FindItem("Cash");
		index1 = ITEM_INDEX(item);
		item = FindItem("Large Cash Bag");
		index2 = ITEM_INDEX(item);
		item = FindItem("Small Cash Bag");
		index3 = ITEM_INDEX(item);

		if (!((ent->client->ps.stats[STAT_PICKUP_STRING] == CS_ITEMS + index1) ||
			(ent->client->ps.stats[STAT_PICKUP_STRING] == CS_ITEMS + index2) ||
			(ent->client->ps.stats[STAT_PICKUP_STRING] == CS_ITEMS + index3)))
			ent->client->ps.stats[STAT_CASH_PICKUP] = 0;

		if (ent->client->invincible_framenum > level.framenum &&
			((ent->client->invincible_framenum - level.framenum) & 4) &&
			ent->client->pers.spectator != SPECTATING)
		{
			item = FindItem("Helmet Armor Heavy");
			ent->client->ps.stats[STAT_ARMOR1] = 2023;
			item = FindItem("Jacket Armor heavy");
			ent->client->ps.stats[STAT_ARMOR2] = 2023;
			item = FindItem("Legs Armor Heavy");
			ent->client->ps.stats[STAT_ARMOR3] = 2023;
		}
		else
		{
			// JOSEPH 1-APR-99-B
			item = FindItem("Helmet Armor");
			ent->client->ps.stats[STAT_ARMOR1] = ent->client->pers.inventory[ITEM_INDEX(item)];
			item = FindItem("Jacket Armor");
			ent->client->ps.stats[STAT_ARMOR2] = ent->client->pers.inventory[ITEM_INDEX(item)];
			item = FindItem("Legs Armor");
			ent->client->ps.stats[STAT_ARMOR3] = ent->client->pers.inventory[ITEM_INDEX(item)];
			item = FindItem("Helmet Armor Heavy");
			if (ent->client->pers.inventory[ITEM_INDEX(item)])
				ent->client->ps.stats[STAT_ARMOR1] = ent->client->pers.inventory[ITEM_INDEX(item)] + 1024;
			item = FindItem("Jacket Armor heavy");
			if (ent->client->pers.inventory[ITEM_INDEX(item)])
				ent->client->ps.stats[STAT_ARMOR2] = ent->client->pers.inventory[ITEM_INDEX(item)] + 1024;
			item = FindItem("Legs Armor Heavy");
			if (ent->client->pers.inventory[ITEM_INDEX(item)])
				ent->client->ps.stats[STAT_ARMOR3] = ent->client->pers.inventory[ITEM_INDEX(item)] + 1024;
			// END JOSEPH		
		}
	}
	// END JOSEPH

	ent->client->ps.stats[STAT_HEALTH] = ent->health;

	//
	// ammo
	//

	//if (!((int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value))
//		ent->client->ps.stats[STAT_CLIP] = ent->client->pers.weapon_clip[ent->client->clip_index];
	//else
	//	ent->client->ps.stats[STAT_CLIP] = 0;


	// JOSEPH 28-APR-99
	if (!ent->client->ammo_index || (int)dmflags->value & DF_INFINITE_AMMO) //GUNRACE_ADD //no need to show ammo if unlimited
	{
		ent->client->ps.stats[STAT_AMMO_ICON] = 0;
		ent->client->ps.stats[STAT_AMMO] = 0;
	}
	else
	{
		item = &itemlist[ent->client->ammo_index];
		ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex(item->icon);
		ent->client->ps.stats[STAT_AMMO] = ent->client->pers.inventory[ent->client->ammo_index];
	}

	// RAFAEL 01-11-99
	// JOSEPH 9-MAR-99
	if (ent->client->ammo_index)
	{
		item = &itemlist[ent->client->ammo_index];
	}
	else
	{
		item = NULL;
	}

	if ((item) && (item->pickup_name) && (((!strcmp(item->pickup_name, "Gas")))))
	{
		ent->client->ps.stats[STAT_CLIP] = -1;
	}
	else
	{
		ent->client->ps.stats[STAT_CLIP] = ent->client->pers.weapon_clip[ent->client->clip_index];
	}



	// END JOSEPH

	//
	// money
	//

	// Ridah, 26-may-99, show frag count
	if (teamplay->value != 1)
		ent->client->ps.stats[STAT_CASH] = ent->client->resp.score;
	else	// show cash
		ent->client->ps.stats[STAT_CASH] = ent->client->pers.currentcash;

	ent->client->ps.stats[STAT_FORCE_HUD] = 0;
	// END JOSEPH

	// JOSEPH 4-MAR-99
	ent->client->ps.stats[STAT_HIDE_HUD] = 0;
	// END JOSEPH    

	ent->client->ps.stats[STAT_SWITCH_CAMERA] = 0;
	// END JOSEPH    

	// JOSEPH 2-FEB-99
	if (level.time > ent->client->hud_enemy_talk_time)
	{
		ent->client->ps.stats[STAT_HUD_ENEMY_TALK] = 0;
	}
	else if ((ent->client->hud_enemy_talk_time - level.time) > 1.0)
	{
		//	ent->client->ps.stats[STAT_HUD_ENEMY_TALK_TIME] = 255;	//GUNRACE_DISABLE?	
	}
	else
	{
		//		ent->client->ps.stats[STAT_HUD_ENEMY_TALK_TIME] =	//GUNRACE_DISABLE?	
		//			(short)((255.0/1.0)*(ent->client->hud_enemy_talk_time - level.time));	//GUNRACE_DISABLE?	
		//		if (ent->client->ps.stats[STAT_HUD_ENEMY_TALK_TIME] < 0)	//GUNRACE_DISABLE?	
		//			ent->client->ps.stats[STAT_HUD_ENEMY_TALK_TIME] = 0;	//GUNRACE_DISABLE?	
	}

	if (level.time > ent->client->hud_self_talk_time)
	{
		ent->client->ps.stats[STAT_HUD_SELF_TALK] = 0;
	}
	else if ((ent->client->hud_self_talk_time - level.time) > 1.0)
	{
		ent->client->ps.stats[STAT_HUD_SELF_TALK_TIME] = 255;
	}
	else
	{
		ent->client->ps.stats[STAT_HUD_SELF_TALK_TIME] =
			(short)((255.0 / 1.0)*(ent->client->hud_self_talk_time - level.time));
		if (ent->client->ps.stats[STAT_HUD_SELF_TALK_TIME] < 0)
			ent->client->ps.stats[STAT_HUD_SELF_TALK_TIME] = 0;
	}
	// END JOSEPH	

	// JOSEPH 4-FEB-99-C
	ent->client->ps.stats[STAT_HUD_INV] = 0;

	if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Battery"))])
		ent->client->ps.stats[STAT_HUD_INV] |= 1;

	if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Coil"))])
		ent->client->ps.stats[STAT_HUD_INV] |= 2;

	// JOSEPH 17-MAR-99
	if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Watch"))])
		ent->client->ps.stats[STAT_HUD_INV] |= 4;
	// END JOSEPH

	if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Safe docs"))])
		ent->client->ps.stats[STAT_HUD_INV] |= 8;

	if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Fuse"))])
		ent->client->ps.stats[STAT_HUD_INV] |= 16;

	if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Valve"))])
		ent->client->ps.stats[STAT_HUD_INV] |= 32;
	// END JOSEPH

	// JOSEPH 10-JUN-99
	if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Lizzy Head"))])
		ent->client->ps.stats[STAT_HUD_INV] |= 64;

	if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Whiskey"))])
		ent->client->ps.stats[STAT_HUD_INV] |= 128;

	if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Oil Can"))])
		ent->client->ps.stats[STAT_HUD_INV] |= 256;

	if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Ticket"))])
		ent->client->ps.stats[STAT_HUD_INV] |= 512;
	// END JOSEPH

	//
	// pickup message
	//
	// JOSEPH 25-JAN-99
	if (level.time > ent->client->pickup_msg_time)
	{
		ent->client->ps.stats[STAT_PICKUP_ICON] = 0;
		ent->client->ps.stats[STAT_PICKUP_STRING] = 0;
		ent->client->ps.stats[STAT_PICKUP_COUNT] = 0;
	}
	else if ((ent->client->pickup_msg_time - level.time) > 1.5)
	{
		ent->client->ps.stats[STAT_PICKUP_COUNT] = 255;
	}
	else
	{
		ent->client->ps.stats[STAT_PICKUP_COUNT] =
			(short)((255.0 / 1.5)*(ent->client->pickup_msg_time - level.time));
		if (ent->client->ps.stats[STAT_PICKUP_COUNT] < 0)
			ent->client->ps.stats[STAT_PICKUP_COUNT] = 0;
	}
	// END JOSEPH


	// Papa - Here is the Timer for the hud
	{
		int framenum = level.framenum - level.startframe;
		if (level.modeset == PREGAME)
			ent->client->ps.stats[STAT_TIMER] = ((359 - framenum) / 10);

		else if (level.modeset == MATCHCOUNT)
			ent->client->ps.stats[STAT_TIMER] = ((159 - framenum) / 10);

		else if (level.modeset == ENDGAME)
			ent->client->ps.stats[STAT_TIMER] = ((209 - framenum) / 10);

		else if (level.modeset == ENDGAMEVOTE)
			ent->client->ps.stats[STAT_TIMER] = ((309 - framenum) / 10);

		else if ((level.modeset == MATCH) || (level.modeset == PUBLIC) && (int)timelimit->value)
			if (framenum > (((int)timelimit->value * 600) - 600))
				ent->client->ps.stats[STAT_TIMER] = ((((int)timelimit->value * 600) + 9 - framenum) / 10);
			else
				ent->client->ps.stats[STAT_TIMER] = ((((int)timelimit->value * 600) - framenum) / 600);
		else
			ent->client->ps.stats[STAT_TIMER] = 0;

		if (ent->client->ps.stats[STAT_TIMER] < 0)
			ent->client->ps.stats[STAT_TIMER] = 0;
	}

	// END JOSEPH

	//
	// selected item
	//
	ent->client->ps.stats[STAT_SELECTED_ITEM] = ent->client->pers.selected_item;

	//
	// layouts
	//
	ent->client->ps.stats[STAT_LAYOUTS] = (ent->client->pers.spectator == SPECTATING);
#if 0
	if (ent->client->pers.health <= 0 || level.intermissiontime	|| ent->client->showscores)
		ent->client->ps.stats[STAT_LAYOUTS] |= 1;
	if (ent->client->showinventory && ent->client->pers.health > 0)
		ent->client->ps.stats[STAT_LAYOUTS] |= 2;
#else //MH Score
	if (ent->client->showscores || ent->deadflag)
		ent->client->ps.stats[STAT_LAYOUTS] = 1;
	else if (ent->client->showinventory && ent->solid != SOLID_NOT)
		ent->client->ps.stats[STAT_LAYOUTS] = 2;
	else if (ent->client->message_frame > level.framenum)
		ent->client->ps.stats[STAT_LAYOUTS] = 1;
#endif
	// ACEBOT_ADD
	if (ent->acebot.is_bot)
		ent->client->ps.stats[STAT_LAYOUTS] = 0;
	// ACEBOT_END

	//
	// frags
	//
	ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;
	ent->client->ps.stats[STAT_DEPOSITED] = ent->client->resp.deposited;


	////////////////////////////////////////////////////////////////
	// BEGIN HITMEN
	//
	// Sort the clients by score so we can display everyones position
	// in the game
	//default off
	ent->client->ps.stats[STAT_POSITION] = 0;
	ent->client->ps.stats[STAT_PLAYERS] = 0;

	//only set scores in dm
	if ((level.modeset == MATCH || level.modeset == PUBLIC
		|| level.modeset == PUBLICSPAWN || level.modeset == MATCHSPAWN)
		&& ent->client->pers.spectator == PLAYING)
	{
		int		i, j, k, score, len, Position;
		int total = 0;
		int		sortedscores[MAX_CLIENTS];
		int		sorted[MAX_CLIENTS];
		edict_t		*cl_ent;
		gclient_t	*cl;


		for (i = 0; i < game.maxclients; i++)
		{
			cl_ent = g_edicts + 1 + i;

			if (!cl_ent->inuse)
				continue;
			if (cl_ent->client->pers.spectator == SPECTATING)
				continue;
			//if (cl_ent->movetype == MOVETYPE_NOCLIP)
			//return;

			score = game.clients[i].resp.score;

			for (j = 0; j < total; j++)
			{
				if (score > sortedscores[j])
					break;
			}

			for (k = total; k > j; k--)
			{
				sorted[k] = sorted[k - 1];
				sortedscores[k] = sortedscores[k - 1];
			}

			sorted[j] = i;
			sortedscores[j] = score;
			total++;
		}

		if (total > 0)
		{
			for (len = 0; len < 16; len++)
			{
				if ((game.clients[sorted[0]].pers.netname[len]) == 0)
					break;
			}

			Position = 0;
			for (i = 0; i < total; i++)
			{
				cl = &game.clients[sorted[i]];
				cl_ent = g_edicts + 1 + sorted[i];

				if (cl_ent == ent)
				{
					Position = i + 1;
					break;
				}
			}

			ent->client->ps.stats[STAT_POSITION] = Position;
			ent->client->ps.stats[STAT_PLAYERS] = total;
		}
	}
	// GUNRACE_END


	
	// Teamplay
	if (teamplay->value)
	{
		int i;

		// show team scores
		for (i=0; i<2; i++)
		{
			ent->client->ps.stats[STAT_TEAM1_SCORE + i] = team_cash[1+i];	// set score

			ent->client->ps.stats[STAT_TEAM1_FLASH + i] = 0;	// set normal

			if (last_safe_withdrawal[i+1] > last_safe_deposit[i+1])
			{
				if (last_safe_withdrawal[i+1] > (level.time - 3.0))
					ent->client->ps.stats[STAT_TEAM1_FLASH + i] = 2;	// flash red
			}
			else
			{
				if (last_safe_deposit[i+1] > (level.time - 3.0))
					ent->client->ps.stats[STAT_TEAM1_FLASH + i] = 1;	// flash green
			}
		}

		if (teamplay->value == 1)
		{
			// show bagged cash
			ent->client->ps.stats[STAT_BAGCASH] = ent->client->pers.bagcash;
			ent->client->ps.stats[23] = 0;
			if (ent->client->pers.team)
			{
				vec3_t dir, start, end;
				trace_t tr;
				AngleVectors( ent->client->ps.viewangles, dir, NULL, NULL );
				VectorCopy( ent->s.origin, start );
				start[2] += ent->viewheight;
				VectorMA( start, 4000, dir, end );
				tr = gi.trace( start, NULL, NULL, end, ent, MASK_SHOT );
				if (tr.fraction < 1 && tr.ent->client && ent->client->pers.team == tr.ent->client->pers.team)
					ent->client->ps.stats[23] = tr.ent->client->pers.bagcash + tr.ent->client->pers.currentcash;
			}
		}

		// team indicators (ifeq/bit statusbar options aren't supported by Kingpin so need to use 2 stat slots)
		ent->client->ps.stats[24] = (ent->client->pers.team==1);
		ent->client->ps.stats[25] = (ent->client->pers.team==2);
	}
}

// JOSEPH 16-DEC-98

// HYPOV8_ADD
void PrintScoreMatchEnd(void)
{
	int i;

	if (!dedicated)
		return;

	gi.dprintf(" \n");
	gi.dprintf("--== results ==--\n");
	gi.dprintf("map                 : %s\n", level.mapname);
	if (enable_bots){
	gi.dprintf("botskill(WEB) 0-10  : %d\n", ACECM_ReturnBotSkillWeb());
	gi.dprintf("sv_botskill 0-4     : %s\n", sv_botskill->string);
	}
	gi.dprintf("num frags ping deaths acc  name           address               ver \n");
	gi.dprintf("--- ----- ---- ------ ---- -------------- --------------------- ----\n");
	for (i = 1; i <= maxclients->value; i++)
	{
		if (g_edicts[i].inuse && g_edicts[i].client)
		{
			int fc = 0;
			gclient_t *c = g_edicts[i].client;

					//  cli frg pin die acc name  ip    ver
			gi.dprintf("%3d %5d %4d %6d %4i %-14s %-23s %.2f\n"
				, i - 1									//client num
				, c->resp.score							//frags
				, c->ping								//ping
				, c->resp.deposited						//deaths DM
				, c->resp.accshot ? c->resp.acchit * 1000 / c->resp.accshot : 0 //acc
				, c->pers.netname						//player name
				, c->pers.ip							//ip
				, (float)c->pers.version / 100.0);		//version
		}
	}
	gi.dprintf("--==   end   ==--\n");
	gi.dprintf(" \n");
}
// HYPOV8_END