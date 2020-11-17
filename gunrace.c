
#include "g_local.h"
#include "gunrace.h"

static grWeps_t grWepsOrig[GR_WEPS + 1] =
{	
	{	// wep 1
		"Pistol",	/* grName */		//.grName wep name. 'pickup name' from itemlist
		"Pistl",	/* grNameMenu*/		//scoreboard name. 5 max length
		"Magnum",	/* grNameHUD */		//wep name in hud
		"Bullets",	/* grAmmoName */	//ammo name used with wep
		50			/* grAmmo */		//fill clip
	},
	{// wep 2
		"Shotgun",
		"Shot",
		"Shotgun",
		"Shells",
		50		
	},
	{	// wep 3
		"Tommygun",
		"Tommy",
		"Tommygun",
		"Bullets",
		150	
	},
	{ 	// wep 4
		"heavy machinegun",
		"HMG",
		"H.M.G",
		"308cal",
		90	
	},
	{ 	// wep 5 //TODO
		"spas12",
		"SPAS",
		"Spas12",
		"Shells",
		50		
	},
	{ 	// wep 6 //TODO
		"mp5",
		"MP5",
		"MP5",
		"Bullets",
		90	
	},
	{	// wep 7 //TODO
	 	"ak47",
		"AK47",
		"AK-47",
		"Bullets",
		120		
	},
	{	// wep 8 //TODO
	 	"benelli",
		"benel",
		"Benelli",
		"Shells",
		50	
	},
	{	// wep 9
		"M41a",
		"M41a",
		"M41a",
		"Bullets",
		90		
	},
	{	// wep 10
		"Uzi",
		"Uzi",
		"Uzi's",
		"Bullets",
		240		
	},
	{ 	// wep 11
		"SuperShotgun",
		"SShot",
		"S.Shotgun",
		"Shells",
		50		
	},
	{	// wep 12
		"M60",
		"M60",
		"M-60",
		"308cal",
		180		
	},
	{	// wep 13 //GR_WEPS = 9 //index 8
		"Machete",
		"Macht",
		"Machete",
		NULL,
		0	
	},
	{	// end of list. +1 for next wep name in HUD 
		"-",
		"",
		"-",
		NULL,
		0
	}
};
//hypov8 todo add new weps

//kills to upgrage
int killcount[GR_WEPS];


//on each map load?
grWeps_t grWeps[GR_WEPS + 1];

static void gr_BuildGunRotationString(void)
{
	int i, j, k;
	int tmpUsed[GR_WEPS - 1];
	int wepOrder = (int)weaponorder->value;
	int gunCount = GR_WEPS - 1;

	//copy gun order default
	memcpy(grWeps, grWepsOrig, sizeof(grWepsOrig));
	memset(tmpUsed, -1, sizeof(tmpUsed));

	if (wepOrder == 1) //default
	{
		;//done
	}
	else if (wepOrder == 2) //reverse
	{
		int rev = gunCount;
		for (i = 0; i < gunCount; i++)
		{ 
			rev -= 1;
			memcpy(&grWeps[i], &grWepsOrig[rev], sizeof(grWeps_t));
		}
	}
	else if (wepOrder == 3) //random
	{
		for (i = 0; i < gunCount; i++)
		{
			for (j = 0; j <= 200; j++) //200 random try's
			{
				int tmpRand = rand() % gunCount;

				//check if allready used
				for (k = 0; k < gunCount; k++)	
				{			
					if (tmpUsed[k] == tmpRand)
						break; //gun in use

					if (k == gunCount-1) //none matching. use this gun
						tmpUsed[i] = tmpRand;
				}

				if (tmpUsed[i] != -1 )
				{  //not used, copy gun 
					memcpy(&grWeps[i], &grWepsOrig[tmpRand], sizeof(grWeps_t));
					break;
				}
			}
		}//do next wep
	}
	else if (wepOrder == 4) //machine gun //hypov8 todo: MG reverse?
	{
		for (i = 0; i < gunCount; i++)
		{
			if (i == 0 || i == 6)
				memcpy(&grWeps[i], &grWepsOrig[3-1], sizeof(grWeps_t)); // 3=tommy
			else if (i == 1 || i == 7)
				memcpy(&grWeps[i], &grWepsOrig[6-1], sizeof(grWeps_t)); // 6=mp5
			else if (i == 2 || i == 8)
				memcpy(&grWeps[i], &grWepsOrig[7-1], sizeof(grWeps_t)); // 7=ak47
			else if (i == 3 || i == 9)
				memcpy(&grWeps[i], &grWepsOrig[9-1], sizeof(grWeps_t)); // 9=m41a
			else if (i == 4 || i == 10)
				memcpy(&grWeps[i], &grWepsOrig[10-1], sizeof(grWeps_t)); // 10=uzi
			else if (i == 5 || i == 11)
				memcpy(&grWeps[i], &grWepsOrig[12-1], sizeof(grWeps_t)); // 12=m60
				//hypov8 todo add new weps
		}
	}
	else if (wepOrder == 5) //shotty
	{
		for (i = 0; i < gunCount; i++)
		{
			if (i == 0 || i == 4 || i== 8)
				memcpy(&grWeps[i], &grWepsOrig[2 - 1], sizeof(grWeps_t)); // 2=shotty
			else if (i == 1 || i == 5|| i == 9)
				memcpy(&grWeps[i], &grWepsOrig[5 - 1], sizeof(grWeps_t)); // 5=spas
			else if (i == 2 || i == 6|| i == 10)
				memcpy(&grWeps[i], &grWepsOrig[8 - 1], sizeof(grWeps_t)); // 8=bennli
			else if (i == 3 || i == 7|| i == 11)
				memcpy(&grWeps[i], &grWepsOrig[11 - 1], sizeof(grWeps_t)); // 11=sshoty

			//hypov8 todo add new weps
		}
	}
}


void SetupGunrace(void) //add hypov8
{
	int i,frags;
	char *fragCnt = weaponfrags->string;
	char *wepOrder = weaponorder->string;


	if (weaponorder->value < 1.0)
		wepOrder = "1";
	if (weaponorder->value > 5.0)
		wepOrder = "5";
	weaponorder = gi.cvar_forceset("weaponorder", wepOrder);//add goat random

	gr_BuildGunRotationString();

	if (weaponfrags->value > 5.0)
		fragCnt = "5";
	if (weaponfrags->value < 1.0)
		fragCnt = "1";
	weaponfrags = gi.cvar_forceset("weaponfrags", fragCnt);

	frags = (int)weaponfrags->value;
	killcount[0] = frags;
	for (i = 1; i < GR_WEPS; i++)
	{
		if (i == GR_WEPS - 1)
			killcount[GR_WEPS-1] = killcount[i - 1] + 1;
		else
			killcount[i] = frags + killcount[i-1];
	}

	//hypov8 force setting fraglimit, incase its used in code at any time
	fraglimit = gi.cvar_forceset("fraglimit", va("%d", killcount[GR_WEPS - 1]));
}

static void gr_Prints(edict_t *ent, qboolean finalWep, int newWep)
{

	if (finalWep)
	{
		edict_t	*dood;
		int i;
		char msg[128];

		for_each_player(dood, i)
		{
			if (dood == ent)
			{
				safe_cprintf(dood, PRINT_CHAT, "> You have the %s\n", grWeps[newWep].grWepName);
			}
			else
			{
				Com_sprintf(msg, sizeof(msg), "%s has the %s!!\n LAST KILL!", ent->client->pers.netname, grWeps[newWep].grWepName);
				safe_centerprintf(dood, "%s\n", msg); //hypov8 should we disable centre print
				safe_cprintf(dood, PRINT_CHAT, "> %s has the %s!!\n", ent->client->pers.netname, grWeps[newWep].grWepName);
			}

		}

		safe_cprintf(ent, PRINT_LOW, "--> Switching to Machete\n--> Extra Health added!\n");
	}
	else
		safe_cprintf(ent, PRINT_LOW, "--> Switching to %s!\n", grWeps[newWep].grWepName);
}

//fill ammo clip
void gr_fillWeaponClip(gclient_t	*client)
{
	gitem_t		*item;
	int			i, nNewAmmoAmount;

	if (!client)
		return;

	//	dont use ammo on Machete
	if (client->resp.curwepIndex != GR_WEPS-1)
	{
		for (i = 0; i < GR_WEPS; i++)
		{
			if (client->resp.curwepIndex == i)
			{
				item = FindItem(grWeps[i].grWepName);
				nNewAmmoAmount = grWeps[i].grAmmo;
				client->ammo_index = ITEM_INDEX(FindItem(item->ammo));
				client->pers.inventory[client->ammo_index] = nNewAmmoAmount;
				break; //fill ammo
			}
		}
	}
}


//player reached new level. switch weapon
static void gr_ChangeClientWeapon(edict_t *ent, int Oldweapon)
{
	gclient_t	*client;
	gitem_t		*item, *ammo;
	int			nNewAmmoAmount;
	int newWep = Oldweapon + 1;


	client = ent->client;
	//invalid client
	if (!client)
		return;

	// Remove the old weapon.
	client->pers.inventory[ITEM_INDEX(FindItem(grWeps[Oldweapon].grWepName))] = 0;

	item = FindItem(grWeps[newWep].grWepName);
	client->resp.curwepIndex = newWep;

	// Make the new weapon the current one.
	client->pers.selected_item = ITEM_INDEX(item);
	client->pers.inventory[client->pers.selected_item] = 1;
	
	//set new weapon
	client->newweapon = &itemlist[ITEM_INDEX(item)]; //hypov8 add

	// show icon and name on status bar
	client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(item->icon);
	client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS+ITEM_INDEX(item);
	client->pickup_msg_time = level.time + 5.5;

	//	dont use ammo on Machete
	if (client->resp.curwepIndex == GR_WEPS - 1)
	{
		gr_Prints(ent, true, newWep);
		client->pers.max_health = 200; //respawn values
		client->pers.health = 200; //respawn values
		ent->client->ps.stats[STAT_HEALTH] = ent->health;
		ent->health +=100; //add +100 health
	}
	else
	{
		// allocate new ammo:
		nNewAmmoAmount = grWeps[newWep].grAmmo;
		client->ammo_index = ITEM_INDEX(FindItem(item->ammo));
		client->pers.inventory[client->ammo_index] = nNewAmmoAmount;
		ammo = FindItem(item->ammo);
		AutoLoadWeapon(client, item, ammo);

		gr_Prints(ent, false, newWep);
	}

	ent->client->resp.countWepChange = level.framenum + 50; //new wep color

	//update scoreboard
	DeathmatchScoreboard(ent);

	//ChangeWeapon(ent); //hypov8 this will auto change. fixing RDF_NOLERP from previous gun
	gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/w_pkup.wav"), 1, ATTN_NORM, 0);
}

/*
===========
gr_CheckWepState

checking player scored
run every server frame (G_RunFrame)
============
*/
void gr_CheckWepState(void)//add hypov8
{
	edict_t	*ent;
	int		i,j;

	// get all current clients to change to new weapon
	for (i = 1; i <= maxclients->value; i++)
	{
		ent = g_edicts + i;

		if (!ent->inuse)
			continue;
		if (ent->solid == SOLID_NOT)
			continue;
		if ((teamplay->value) && (ent->client->pers.team == 0))
			continue;
		if (ent->client->pers.spectator == SPECTATING)
			continue;

		//cant change wep anymore
		if (ent->client->resp.curwepIndex == GR_WEPS - 1)
			continue;

		//hypo look through all score/kill counters
		for (j = GR_WEPS-1 ; j >=0 ; j--) // count backward, what if we killed 2 ppl
		{
			if (ent->client->resp.score >= killcount[j] && ent->client->resp.curwepIndex == j)
			{
				gr_ChangeClientWeapon(ent, j);
				break;
			}
		}

		//hypov8 fill ammo every frame. saves setting up wep cfg's
		gr_fillWeaponClip(ent->client);
	}
}



/*
===========
gr_RespawnSetWeps

used in InitClientPersistant
set's current wep. stats etc
============
*/
void gr_RespawnSetWeps(gclient_t * client)
{
	gitem_t		*item, *ammo;
	int j;

	client->pers.pistol_mods = WEAPON_MOD_DAMAGE;  //G()^T START WITH MOD

	//hypo look through all score/kill counters
	for (j = 0; j < GR_WEPS; j++)
	{
		int newWep = j + 1;

		if (client->resp.score < killcount[0])
		{
			item = FindItem(grWeps[j].grWepName);
			client->pers.selected_item = ITEM_INDEX(item);
			client->pers.inventory[client->pers.selected_item] = 1;
			client->newweapon = &itemlist[ITEM_INDEX(item)]; //hypov8 add
			client->ammo_index = ITEM_INDEX(FindItem(item->ammo)); //'newweapon' and 'ammo_index' get deleted!!!
			client->pers.inventory[client->ammo_index] = grWeps[j].grAmmo;
			client->pers.weapon = item;
			ammo = FindItem (item->ammo);
			AutoLoadWeapon( client, item, ammo );

			break; //found our score
		}


		if ((client->resp.score >= killcount[j]) && (client->resp.score < killcount[j+1]))
		{
			item = FindItem(grWeps[newWep].grWepName);
			client->pers.selected_item = ITEM_INDEX(item);
			client->pers.inventory[client->pers.selected_item] = 1;
			client->newweapon = &itemlist[ITEM_INDEX(item)]; //hypov8 add
			client->pers.weapon = item;

			if (client->resp.curwepIndex != GR_WEPS - 1) //only load ammo if not Machete
			{
				client->ammo_index = ITEM_INDEX(FindItem(item->ammo));
				client->pers.inventory[client->ammo_index] = grWeps[newWep].grAmmo;

				ammo = FindItem(item->ammo);
				AutoLoadWeapon(client, item, ammo);
			}
			break; //found our score
		}
	}

	if (client->resp.curwepIndex == GR_WEPS-1) //hypov8 this can be a cheat. changeing from spec>respawn
	{
		client->pers.max_health = 200;
		if (client->ps.pmove.pm_type != PM_SPECTATOR) //stop player going to spec and getting 200 hp
			client->pers.health = 200;					//should we store old health??
	}

}



//use in ClientBeginDeathmatch
void gr_ResetPlayerBeginDM(gclient_t *client)
{
	int j;

	//hypo look through all score/kill counters
	for (j = 0; j < GR_WEPS; j++)
	{
		int newWep = j + 1;
		if (client->resp.score < killcount[0])		{
			client->resp.killtonext = (killcount[0] - client->resp.score);
			client->resp.curwepIndex = j;
			break; //found our score
		}

		if ((client->resp.score >= killcount[j]) && (client->resp.score < killcount[j+1]))		{
			client->resp.killtonext = (killcount[newWep] - client->resp.score);
			client->resp.curwepIndex = newWep;
			break; //found our score
		}
	}

	if (client->resp.curwepIndex == GR_WEPS - 1) //hypov8 this can be a cheat. changeing from spec>respawn
	{
		client->pers.max_health = 200;
		if (client->ps.pmove.pm_type != PM_SPECTATOR) //stop player going to spec and getting 200 hp
			client->pers.health = 200;					//should we store old health??
	}
}


//used in ClientObituary(likks)
void gr_SetKillsToNext(gclient_t *client)
{
	int j;

	//safe_bprintf(PRINT_HIGH, "%s has %i kills\n", client->pers.netname, client->resp.score); //hypov8 todo send to who??

	if (client->resp.score >= killcount[GR_WEPS-1]){
		safe_bprintf(PRINT_HIGH, "Fraglimit hit.\n");
		safe_bprintf(PRINT_HIGH, "%s WON THE GUNRACE!\n", client->pers.netname);
		EndDMLevel();
		return;
	}


	//hypo look through all score/kill counters
	for (j = 0; j < GR_WEPS; j++)
	{
		if (client->resp.score < killcount[0]) //&&(attacker->client->resp.killtonext) > (killcount[0] - attacker->client->resp.score)
		{
			client->resp.killtonext = (killcount[0] - client->resp.score);
			break; //found our score
		}

		if (client->resp.score >= killcount[j] && client->resp.score < killcount[j + 1])
		{
			client->resp.killtonext = ((killcount[j + 1]) - (client->resp.score));
			break; //found our score
		}
	}
}



//update scoreboard regulary
void gr_ScoreBoard(void)
{
	edict_t	*ent;
	int		i;

	for (i = 1; i <= maxclients->value; i++)
	{
		ent = g_edicts + i;

		if (!ent->inuse)
			continue;

		if ((teamplay->value) && (ent->client->pers.team == 0))
			continue;

		//hypo dont include spectators
		if (ent->client->pers.spectator == SPECTATING && !ent->client->chase_target) //add hypov8 show spec'd stats
			continue;

		//only set scores in dm
		if ((level.modeset != MATCH && level.modeset != PUBLIC
			&& level.modeset !=  PUBLICSPAWN && level.modeset != MATCHSPAWN))
			continue;

		if (ent->client->showscores != NO_SCOREBOARD)
			continue;

		if (ent->client->resp.scoreboard_frame > level.framenum)
			continue;

		//set scoreboard
		ent->client->showscores = SCORE_GUNRACE_STATS;
	}
}

//set to gunrace scoreboard when NO_SCOREBOARD
void gr_setScoreboardGunrace(gclient_t *client)
{
	//ent->client->resp.scoreboard_hide = false;
	if (client->showscores == NO_SCOREBOARD
		&& client->resp.scoreboard_frame < level.framenum
		&& (level.modeset == MATCH || level.modeset == PUBLIC)
		&& (client->pers.spectator != SPECTATING || (client->pers.spectator == SPECTATING && client->chase_target))) //ad hypov8 update chase scorebord
	{
		client->showscores = SCORE_GUNRACE_STATS;
	}
}


//add hypov8
//check for machete. dont calculate bullets shots anymore
qboolean gr_isLastWep(gclient_t * client)
{
	if (client->resp.score >= killcount[GR_WEPS - 2])
		return true;
	return false;
}