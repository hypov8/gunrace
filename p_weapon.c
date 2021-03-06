// g_weapon.c

#include "g_local.h"
#include "m_player.h"


static qboolean	is_quad;
// RAFAEL
static qboolean is_quadfire;
static byte		is_silenced;

void weapon_grenade_fire (edict_t *ent, qboolean held);
// RAFAEL
void weapon_trap_fire (edict_t *ent, qboolean held);

static void P_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t	_distance;

	VectorCopy (distance, _distance);
/*	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)*/
		_distance[1] = 0;
	G_ProjectSource (point, _distance, forward, right, result);
}


#define MAX_PISTOL_ROUNDS		12  //GUNRACE_CHANGE
#define MAX_TOMMYGUN_ROUNDS		60  //GUNRACE_CHANGE
#define MAX_SHOTGUN_ROUNDS		12 //GUNRACE_CHANGE
#define MAX_BARMACHINEGUN_ROUNDS 	30
#define MAX_GRENADELAUNCHER_ROUNDS	3
#define MAX_ROCKETLAUNCHER_ROUNDS	5
#define	MAX_FLAMEGUN_ROUNDS			50
//GUNRACE_START
#define MAX_SSHOTGUN_ROUNDS		16
#define MAX_M41AGUN_ROUNDS		30
#define MAX_UZI_ROUNDS		    80
#define MAX_M60_ROUNDS			60
#define MAX_BENELLI_ROUNDS		12
#define MAX_SPAS12_ROUNDS		12
#define MAX_MP5_ROUNDS			30
#define MAX_AK47_ROUNDS			40
	//hypov8 todo add new weps
//GUNRACE_END


// this is here so we can simplify the auto-reload code. it's in the same order as clip_t
int	auto_rounds[] =
{
	0,
	MAX_PISTOL_ROUNDS,
	MAX_SHOTGUN_ROUNDS,
	MAX_TOMMYGUN_ROUNDS,
	MAX_BARMACHINEGUN_ROUNDS,
	MAX_GRENADELAUNCHER_ROUNDS,
#if 0 //GUNRACE_DISABLED
	//MAX_ROCKETLAUNCHER_ROUNDS,
	//MAX_FLAMEGUN_ROUNDS,
#endif
//GUNRACE_START
	MAX_SSHOTGUN_ROUNDS,
	MAX_M41AGUN_ROUNDS,
	MAX_UZI_ROUNDS,
	MAX_M60_ROUNDS,
	MAX_BENELLI_ROUNDS,
	MAX_SPAS12_ROUNDS,
	MAX_MP5_ROUNDS,
	MAX_AK47_ROUNDS,
	//hypov8 todo add new weps
	//hypov8 needs to match clip_t order
//GUNRACE_END
	0
};

int QweryClipIndex (gitem_t *item);

void AutoLoadWeapon( gclient_t *client, gitem_t *weapon, gitem_t *ammo )
{
	int clip_index;
	int	ammo_index;

	clip_index = QweryClipIndex(weapon);
	if (clip_index != CLIP_NONE && !(client->pers.weapon_clip[clip_index]))
	{
		ammo_index = ITEM_INDEX(ammo);

		if (client->pers.inventory[ammo_index] >= auto_rounds[clip_index])
		{
			client->pers.weapon_clip[clip_index] = auto_rounds[clip_index];
			client->pers.inventory[ammo_index] -= auto_rounds[clip_index];
		}
		else	// not quite enough ammo, so just load all ammo
		{
			client->pers.weapon_clip[clip_index] = client->pers.inventory[ammo_index];
			client->pers.inventory[ammo_index] = 0;
		}
	}
}

qboolean Pickup_Weapon (edict_t *ent, edict_t *other)
{
	int			index;
	gitem_t		*ammo;
	int			auto_reload=false;

	// GUNRACE ADD
	if (deathmatch_value) //add hypov8 bugfix can spawn weapon when in spec
		return false;
	// GUNRACE END

	index = ITEM_INDEX(ent->item);

	if ( ( ((int)(dmflags->value) & DF_WEAPONS_STAY) || coop_value) 
		&& other->client->pers.inventory[index])
	{
		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM) ) )
			return false;	// leave the weapon for others to pickup
	}

	// Ridah, start with gun loaded
	if (!(other->client->pers.inventory[index]))
		auto_reload = true;

	//other->client->pers.inventory[index]++;
	other->client->pers.inventory[index] = 1;

	//if they pick weapon up make them mortal
	other->client->invincible_framenum = 0;

	if (ent->item->ammo && !(ent->spawnflags & DROPPED_ITEM) )
	{
		// give them some ammo with it
		ammo = FindItem (ent->item->ammo);
		if ( (int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value )
			Add_Ammo (other, ammo, 1000);
		else
			Add_Ammo (other, ammo, ammo->quantity);

		if (auto_reload)
			AutoLoadWeapon( other->client, ent->item, ammo );

		if (! (ent->spawnflags & DROPPED_PLAYER_ITEM) )
		{
			if (deathmatch_value)
			{
				if ((int)(dmflags->value) & DF_WEAPONS_STAY)
					ent->flags |= FL_RESPAWN;
				else
					SetRespawn (ent, 30);
			}
			if (coop_value)
				ent->flags |= FL_RESPAWN;
		}
	}
	else if (!(strcmp (ent->classname, "weapon_crowbar")))
	{
		if (! (ent->spawnflags & DROPPED_PLAYER_ITEM) )
		{
			if (deathmatch_value)
			{
				if ((int)(dmflags->value) & DF_WEAPONS_STAY)
					ent->flags |= FL_RESPAWN;
				else
					SetRespawn (ent, 30);
			}
			if (coop_value)
				ent->flags |= FL_RESPAWN;
		}
	}
	
	if (other->client->pers.weapon == FindItem ("SPistol"))
			other->client->pers.silencer_shots = 20;

	/*
	if (other->client->pers.weapon != ent->item && 
		(other->client->pers.inventory[index] == 1) &&
		( !deathmatch_value || other->client->pers.weapon == FindItem("Pipe") ) )
		other->client->newweapon = ent->item;
	*/
	
	if (deathmatch_value)
	{
		if (other->client->pers.weapon != ent->item && auto_reload && (other->client->pers.weapon == FindItem ("Pistol") || other->client->pers.weapon == FindItem ("Pipe") ))
			other->client->newweapon = ent->item;
	}
	else
	{
	if (other->client->pers.weapon != ent->item && auto_reload)
		other->client->newweapon = ent->item;
	}
	
	
	return true;
}

/*
Change Clip Index 
*/
void ChangeClipIndex (edict_t *ent)
{

	if (!strcmp(ent->client->pers.weapon->pickup_name, "Pipe"))
		ent->client->clip_index = CLIP_NONE;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "Crowbar"))
		ent->client->clip_index = CLIP_NONE;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "Pistol"))
		ent->client->clip_index = CLIP_PISTOL;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "SPistol"))
		ent->client->clip_index = CLIP_PISTOL;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "Shotgun"))
		ent->client->clip_index = CLIP_SHOTGUN;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "Tommygun"))
		ent->client->clip_index = CLIP_TOMMYGUN;
	//else if (!strcmp(ent->client->pers.weapon->pickup_name, "FlameThrower"))
		//ent->client->clip_index = CLIP_FLAMEGUN; //GUNRACE_DISABLED
	//else if (!strcmp(ent->client->pers.weapon->pickup_name, "Bazooka"))
		//ent->client->clip_index = CLIP_ROCKETS; //GUNRACE_DISABLED
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "Grenade Launcher"))
		ent->client->clip_index = CLIP_GRENADES;
	// JOSEPH 16-APR-99
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "Heavy machinegun"))
		ent->client->clip_index = CLIP_308;
	// END JOSEPH
	//GUNRACE_START

	else if (!strcmp(ent->client->pers.weapon->pickup_name, "SuperShotgun"))
		ent->client->clip_index = CLIP_SSHOTGUN;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "M41a"))
		ent->client->clip_index = CLIP_M41A;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "Uzi"))
		ent->client->clip_index = CLIP_UZI;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "M60"))
		ent->client->clip_index = CLIP_M60;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "Machete"))
		ent->client->clip_index = CLIP_MACHETE;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "benelli"))
		ent->client->clip_index = CLIP_BENELLI;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "spas12"))
		ent->client->clip_index = CLIP_SPAS12;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "mp5"))
		ent->client->clip_index = CLIP_MP5;
	else if (!strcmp(ent->client->pers.weapon->pickup_name, "ak47"))
		ent->client->clip_index = CLIP_AK47;
	//hypov8 todo add new weps
	//GUNRACE_END
//GUNRACE_TODO
}


int QweryClipIndex (gitem_t *item)
{

	if (!strcmp(item->pickup_name, "Pipe"))
		return CLIP_NONE;
	else if (!strcmp(item->pickup_name, "Crowbar"))
		return CLIP_NONE;
	else if (!strcmp(item->pickup_name, "Pistol"))
		return CLIP_PISTOL;
	else if (!strcmp(item->pickup_name, "SPistol"))
		return CLIP_PISTOL;
	else if (!strcmp(item->pickup_name, "Shotgun"))
		return CLIP_SHOTGUN;
	else if (!strcmp(item->pickup_name, "Tommygun"))
		return CLIP_TOMMYGUN;
#if 0  //GUNRACE_DISABLED
	//else if (!strcmp(item->pickup_name, "FlameThrower"))
		//return CLIP_FLAMEGUN;
	//else if (!strcmp(item->pickup_name, "Bazooka"))
		//return CLIP_ROCKETS;
#endif
	else if (!strcmp(item->pickup_name, "Grenade Launcher"))
		return CLIP_GRENADES;
	// JOSEPH 16-APR-99
	else if (!strcmp(item->pickup_name, "Heavy machinegun"))
		return CLIP_308;
	// END JOSEPH
	//GUNRACE_START
	else if (!strcmp(item->pickup_name, "SuperShotgun"))
		return CLIP_SSHOTGUN;
	else if (!strcmp(item->pickup_name, "M41a"))
		return CLIP_M41A;
	else if (!strcmp(item->pickup_name, "Uzi"))
		return CLIP_UZI;
	else if (!strcmp(item->pickup_name, "M60"))
		return CLIP_M60;
	else if (!strcmp(item->pickup_name, "Machete"))
		return CLIP_MACHETE;
	else if (!strcmp(item->pickup_name, "benelli"))
		return CLIP_BENELLI;
	else if (!strcmp(item->pickup_name, "spas12"))
		return CLIP_SPAS12;
	else if (!strcmp(item->pickup_name, "mp5"))
		return CLIP_MP5;
	else if (!strcmp(item->pickup_name, "ak47"))
		return CLIP_AK47;
	//hypov8 todo add new weps
	//GUNRACE_END
//GUNRACE_TODO

	return (0);
}


/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon (edict_t *ent)
{
	int i;

	if (ent->client->grenade_time)
	{
		ent->client->grenade_time = level.time;
		ent->client->weapon_sound = 0;
		weapon_grenade_fire (ent, false);
		ent->client->grenade_time = 0;
	}

	ent->client->pers.lastweapon = ent->client->pers.weapon;
	ent->client->pers.weapon = ent->client->newweapon;
	ent->client->newweapon = NULL;
	ent->client->machinegun_shots = 0;

	// set visible model
	if (ent->s.modelindex == 255)
	{
		if (ent->client->pers.weapon)
			i = (((QweryClipIndex(ent->client->pers.weapon)+1) & 0xff) << 8);
		else
			i = 0;
		ent->s.skinnum = (ent - g_edicts - 1) | i;
	}

	if (ent->client->pers.weapon && ent->client->pers.weapon->ammo)
		ent->client->ammo_index = ITEM_INDEX(FindItem(ent->client->pers.weapon->ammo));
	else
		ent->client->ammo_index = 0;

	if (!ent->client->pers.weapon)
	{	// dead
		ent->client->ps.gunindex = 0;
		return;
	}

	ent->client->weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.gunframe = 0;
	
	// ent->client->flashlight = false;

	if (ent->client->pers.weapon)
		ChangeClipIndex (ent);
	
	{
		int i;
	
		if (!strcmp (ent->client->pers.weapon->pickup_name , "SPistol"))
		{
			if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE)
				ent->client->ps.gunindex = gi.modelindex("models/weapons/silencer_mdx/magnum.mdx");
			else
				ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
		}
		else if (!strcmp (ent->client->pers.weapon->pickup_name , "Pistol"))
		{
			if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE)
				ent->client->ps.gunindex = gi.modelindex("models/weapons/v_colt/magnum.mdx");
			else
				ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
		}
		else if (!strcmp (ent->client->pers.weapon->pickup_name, "Shotgun"))
			ent->client->ps.gunindex = gi.modelindex("models/weapons/shotgun/shotgun.mdx");
		else if (!strcmp (ent->client->pers.weapon->pickup_name, "Heavy machinegun"))
		{
			if (ent->client->pers.pistol_mods & WEAPON_MOD_ROF/* WEAPON_MOD_COOLING_JACKET*/) //GUNRACE_ADD
				ent->client->ps.gunindex = gi.modelindex("models/weapons/v_hmgcool/hmgcool.mdx");
			else
				ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
		}
		else
			ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);

		//reset gun stat
		memset(&(ent->client->ps.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);
		ent->client->ps.num_parts = 0;

		ent->client->ps.num_parts++;
		// ent->client->ps.model_parts[PART_HEAD].modelindex = gi.modelindex(ent->client->pers.weapon->view_model);
		ent->client->ps.model_parts[PART_GUN_BODY].modelindex = ent->client->ps.gunindex;//gi.modelindex(ent->client->pers.weapon->view_model);
		for (i=0; i<MAX_MODELPART_OBJECTS; i++)
			ent->client->ps.model_parts[PART_GUN_BODY].skinnum[i] = 0; // will we have more than one skin???

		// HACK, set arm model
		if (!strcmp(ent->client->pers.weapon->pickup_name, "Shotgun"))
		{
			// Ridah, HACK: changed this to use PART_LEGS, since PART_BODY sometimes doesn't show up
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/shotgun/hand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0; // will we have more than one skin???
		}
		//GUNRACE_START
		//G()^T Supershotgun
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "SuperShotgun"))
		{
			/*ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_MISC].modelindex = gi.modelindex("models/weapons/v_shotg2/v_spr.mdx");	//sprite	
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_MISC].skinnum[i] = 0;*/ //hypov8 note can only display 1 sprite at a time
		}
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "Machete")) //hypov8
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_machete/hand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0;
		}

		else if (!strcmp(ent->client->pers.weapon->pickup_name, "M41a"))
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_m41a/lhand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0;

			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_R_HAND].modelindex = gi.modelindex("models/weapons/v_m41a/rhand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_R_HAND].skinnum[i] = 0;
		}
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "Uzi"))
		{
			//null;
		}
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "M60"))
		{
			ent->client->ps.num_parts++; //arms
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_m60/v_arms.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0;

			ent->client->ps.num_parts++; //fingers
			ent->client->ps.model_parts[PART_GUN_FINGERS].modelindex = gi.modelindex("models/weapons/v_m60/v_thumb.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_FINGERS].skinnum[i] = 0;
		}
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "spas12"))
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_spas12/v_hand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0;
		}
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "benelli"))
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_benelli/v_hand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0;
		}
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "mp5"))
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_mp5/v_hand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0;
		}
		//ak47
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "ak47"))
		{
			ent->client->ps.num_parts++;		//magazine
			ent->client->ps.model_parts[PART_GUN_CLIP].modelindex = gi.modelindex("models/weapons/v_ak47/v_clip.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_CLIP].skinnum[i] = 0;

			ent->client->ps.num_parts++;		//arms
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_ak47/v_hand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0;
		}

		//hypov8 todo add new weps
		//GUNRACE_END
		// JOSEPH 12-OCT-98
		// JOSEPH 5-FEB-99-B
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "Pipe"))
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/blakjak/hand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0; // will we have more than one skin???
		}
		// END JOSEPH
		// JOSEPH 19-JAN-99
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "Crowbar"))
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/crowbar/hand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0; // will we have more than one skin???
		}
		// END JOSEPH

		else if (!strcmp(ent->client->pers.weapon->pickup_name, "FlameThrower"))
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_flamegun/hand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0; // will we have more than one skin???
		}
		// END JOSEPH
		// JOSEPH 4-MAR-99
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "Bazooka"))
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_rocket_launcher/hand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0; // will we have more than one skin???

			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_MISC].modelindex = gi.modelindex("models/weapons/v_rocket_launcher/shell_a.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_MISC].skinnum[i] = 0; // will we have more than one skin???		

			//posible conflict - tical
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_CLIP].modelindex = gi.modelindex("models/weapons/v_rocket_launcher/clip.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_CLIP].skinnum[i] = 0; // will we have more than one skin???		
		}
		// END JOSEPH
		else if (!strcmp (ent->client->pers.weapon->pickup_name , "SPistol"))
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/silencer_mdx/handl.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0; // will we have more than one skin???
			
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_R_HAND].modelindex = gi.modelindex("models/weapons/silencer_mdx/handr.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_R_HAND].skinnum[i] = 0; // will we have more than one skin???
		
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_CLIP].modelindex = gi.modelindex("models/weapons/silencer_mdx/clip.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_CLIP].skinnum[i] = 0;

			//ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = (1<<0 | 1<<1);
		}
		else if (!strcmp (ent->client->pers.weapon->pickup_name , "Pistol"))
		{
#if 1 
//GUNRACE_START	//hypov8 left hand is now left gun(sprite fix)
			if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE)
				ent->client->ps.model_parts[PART_GUN_MISC].modelindex = gi.modelindex("models/weapons/v_colt/magnum_l.mdx");
			else
				ent->client->ps.model_parts[PART_GUN_MISC].modelindex = gi.modelindex("models/weapons/v_colt/pistol_l.mdx");

			ent->client->ps.num_parts++;
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_MISC].skinnum[i] = 0; // will we have more than one skin???
//GUNRACE_END
#else
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_colt/handl.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0; // will we have more than one skin???

#endif			
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_R_HAND].modelindex = gi.modelindex("models/weapons/v_colt/handr.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_R_HAND].skinnum[i] = 0; // will we have more than one skin???
		
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_CLIP].modelindex = gi.modelindex("models/weapons/v_colt/clip.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_CLIP].skinnum[i] = 0;
		}
		else if (!strcmp(ent->client->pers.weapon->pickup_name, "Tommygun"))
		{
			// Ridah, hacked this, we're still having disappearing hands after going from the Pistol ?!
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_tomgun/lhand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0; // will we have more than one skin???

			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_R_HAND].modelindex = gi.modelindex("models/weapons/v_tomgun/rhand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_R_HAND].skinnum[i] = 0; // will we have more than one skin???
		
		}
		// JOSEPH 16-APR-99
		else if (!strcmp (ent->client->pers.weapon->pickup_name , "Heavy machinegun"))
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_hmg/lhand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0; // will we have more than one skin???

			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_CLIP].modelindex = gi.modelindex("models/weapons/v_hmg/clip.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_CLIP].skinnum[i] = 0; // will we have more than one skin???
			
		}
		// END JOSEPH
		else if (!strcmp (ent->client->pers.weapon->pickup_name , "Grenade Launcher"))
		{
			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_L_HAND].modelindex = gi.modelindex("models/weapons/v_grenade_launcher/lhand.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_L_HAND].skinnum[i] = 0; // will we have more than one skin???

			ent->client->ps.num_parts++;
			ent->client->ps.model_parts[PART_GUN_CLIP].modelindex = gi.modelindex("models/weapons/v_grenade_launcher/clip.mdx");
			for (i=0; i<MAX_MODELPART_OBJECTS; i++)
				ent->client->ps.model_parts[PART_GUN_CLIP].skinnum[i] = 0; // will we have more than one skin???

			//ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = (1<<0 | 1<<1);
		}
	}

//	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
#if 0
	ent->client->anim_priority = ANIM_PAIN;
	if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		int rnd;

		rnd = rand()%3;

		if (rnd==0)
		{
			ent->s.frame = FRAME_crouch_pain1_01;
			ent->client->anim_end = FRAME_crouch_pain1_08;
		}
		else if (rnd==1)
		{
			ent->s.frame = FRAME_crouch_pain2_01;
			ent->client->anim_end = FRAME_crouch_pain2_06;
		}
		else if (rnd==2)
		{
			ent->s.frame = FRAME_crouch_pain3_01;
			ent->client->anim_end = FRAME_crouch_pain3_06;
		}
	}
	else
	{
		int rnd;

		/*
		rnd = rand()%3;

		if (rnd==0)
		{
			ent->s.frame = FRAME_pain1_01;
			ent->client->anim_end = FRAME_pain1_05;
		}
		else if (rnd==1)
		{
			ent->s.frame = FRAME_pain2_01;
			ent->client->anim_end = FRAME_pain2_05;
		}
		else if (rnd==2)
		{
			ent->s.frame = FRAME_pain3_01;
			ent->client->anim_end = FRAME_pain3_05;
		}
		*/
		rnd = rand()%6;

		switch (rnd)
		{
		case 0:
			ent->s.frame = FRAME_pain_Rarm_01;
			ent->client->anim_end = FRAME_pain_Rarm_10;
			break;
		case 1:
			ent->s.frame = FRAME_pain_Larm_01;
			ent->client->anim_end = FRAME_pain_Larm_09;
			break;
		case 2:
			ent->s.frame = FRAME_pain_chest_01;
			ent->client->anim_end = FRAME_pain_chest_09;
			break;
		case 3:
			ent->s.frame = FRAME_pain_head_01;
			ent->client->anim_end = FRAME_pain_head_10;
			break;
		case 4:
			ent->s.frame = FRAME_pain_Rleg_01;
			ent->client->anim_end = FRAME_pain_Rleg_10;
			break;
		case 5:
			ent->s.frame = FRAME_pain_Lleg_01;
			ent->client->anim_end = FRAME_pain_Lleg_08;
			break;
		}

	}

#endif
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange (edict_t *ent)
{
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("grenade launcher"))] 
		&& (ent->client->pers.weapon_clip[CLIP_GRENADES] || ent->client->pers.inventory[ITEM_INDEX(FindItem("grenades"))]) )
	{
		ent->client->newweapon = FindItem ("grenade launcher");
		return;
	}

	// JOSEPH 16-APR-99
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("heavy machinegun"))]
		&& (ent->client->pers.weapon_clip[CLIP_308] || ent->client->pers.inventory[ITEM_INDEX(FindItem("308cal"))]) )
	{
		ent->client->newweapon = FindItem ("heavy machinegun");
		return;
	}
	// END JOSEPH
#if 0  //GUNRACE_DISABLED
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("flamethrower"))]
		&& ent->client->pers.inventory[ITEM_INDEX(FindItem("gas"))] )
	{
		ent->client->newweapon = FindItem ("flamethrower");
		return;
	}
	
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("bazooka"))]
		&& (ent->client->pers.weapon_clip[CLIP_ROCKETS] || ent->client->pers.inventory[ITEM_INDEX(FindItem("rockets"))]) )
	{
		ent->client->newweapon = FindItem ("Bazooka");
		return;
	}
#endif
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("tommygun"))]
		&& (ent->client->pers.weapon_clip[CLIP_TOMMYGUN] || ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]) )
	{
		ent->client->newweapon = FindItem ("tommygun");
		return;
	}
	
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("shotgun"))]
		&& (ent->client->pers.weapon_clip[CLIP_SHOTGUN] || ent->client->pers.inventory[ITEM_INDEX(FindItem("shells"))]) )
	{
		ent->client->newweapon = FindItem ("shotgun");
		return;
	}
	//GUNRACE_START	
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("Supershotgun"))]
		&& (ent->client->pers.weapon_clip[CLIP_SSHOTGUN] || ent->client->pers.inventory[ITEM_INDEX(FindItem("shells"))]) )
	{
		ent->client->newweapon = FindItem ("Supershotgun");
		return;
	}

	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("M41a"))]
		&& (ent->client->pers.weapon_clip[CLIP_SSHOTGUN] || ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]) )
	{
		ent->client->newweapon = FindItem ("M41a");
		return;
	}

	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("Uzi"))]
		&& (ent->client->pers.weapon_clip[CLIP_SSHOTGUN] || ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]) )
	{
		ent->client->newweapon = FindItem ("Uzi");
		return;
	}

	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("M60"))]
		&& (ent->client->pers.weapon_clip[CLIP_SSHOTGUN] || ent->client->pers.inventory[ITEM_INDEX(FindItem("308cal"))]) )
	{
		ent->client->newweapon = FindItem ("M60");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("benelli"))]
		&& (ent->client->pers.weapon_clip[CLIP_SSHOTGUN] || ent->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))]) )
	{
		ent->client->newweapon = FindItem ("benelli");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("spas12"))]
		&& (ent->client->pers.weapon_clip[CLIP_SSHOTGUN] || ent->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))]) )
	{
		ent->client->newweapon = FindItem ("spas12");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("mp5"))]
		&& (ent->client->pers.weapon_clip[CLIP_SSHOTGUN] || ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]) )
	{
		ent->client->newweapon = FindItem ("mp5");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("ak47"))]
		&& (ent->client->pers.weapon_clip[CLIP_SSHOTGUN] || ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]) )
	{
		ent->client->newweapon = FindItem ("ak47");
		return;
	}

	//hypov8 todo add new weps
	//GUNRACE_END			
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("pistol"))]
		&& (ent->client->pers.weapon_clip[CLIP_PISTOL] || ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]) )
	{
		ent->client->newweapon = FindItem ("pistol");
		return;
	}

	// JOSEPH 5-FEB-99-B
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("crowbar"))])
		ent->client->newweapon = FindItem ("Crowbar");
	else	
		ent->client->newweapon = FindItem ("Pipe");
	// END JOSEPH
}


/*
Think_FlashLight
*/
void Think_FlashLight (edict_t *ent)
{

	trace_t tr;
	vec3_t	forward;
	vec3_t	end;
	vec3_t	mins, maxs;
	edict_t	*light;
	vec3_t	angles, origin;

	if (ent->health < 1)
		return;

// ACEBOT_ADD
	if (ent->acebot.is_bot)
		ent->client->flashlight = false; //skip flastlight
// ACEBOT_END

	if (ent->client->flashlight)
	{
		VectorCopy (ent->client->v_angle, angles);
		VectorCopy (ent->s.origin, origin);
		AngleVectors (angles, forward, NULL, NULL);
		VectorCopy (origin, end);
		VectorMA (end, 8194, forward, end);

		VectorSet (mins, -8, -8, -8 );
		VectorSet (maxs, 8, 8, 8);

		tr = gi.trace (ent->s.origin, mins, maxs, end, ent, MASK_SHOT);

		if (((tr.ent) && (tr.ent->s.renderfx2 & RF2_SURF_ALPHA)) || ((tr.contents & MASK_ALPHA))) 
		{
			vec3_t	start;
			VectorCopy (tr.endpos, start);
			VectorMA (start, 8, forward, start);
			tr = gi.trace (start, mins, maxs, end, ent, MASK_SHOT);
		}

		light = G_Spawn();
		VectorCopy (tr.endpos, light->s.origin);
		light->s.renderfx |= RF_BEAM;
		light->s.effects |= EF_FLASHLIGHT;
		light->nextthink = level.time + 0.1;
		light->think = G_FreeEdict;
		gi.linkentity (light);
	}


}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/		
void Think_Weapon (edict_t *ent)
{
	// make sure flamethrower is disabled when it should be
	ent->s.renderfx2 &= ~RF2_FLAMETHROWER;

	// if just died, put the weapon away
	if (ent->health < 1)
	{
		ent->client->newweapon = NULL;
		ChangeWeapon (ent);
	}
	
	if (ent->client->pers.weapon && ent->client->pers.weapon->weaponthink)
	{
		is_quad = (ent->client->quad_framenum > level.framenum);
		// RAFAEL
		is_quadfire = (ent->client->quadfire_framenum > level.framenum);
		//if (ent->client->pers.silencer_shots)
		//	is_silenced = MZ_SILENCED;
		//else
		is_silenced = 0;
		ent->client->pers.weapon->weaponthink (ent);
	}

}


/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;
	int			clip_index;

	// see if we're already using it
	if (item == ent->client->pers.weapon)
		return;

	
	if (item->ammo && !g_select_empty->value && !(item->flags & IT_AMMO))
	{
		ammo_item = FindItem(item->ammo);
		ammo_index = ITEM_INDEX(ammo_item);
		
		clip_index = QweryClipIndex (item);

		if (!ent->client->pers.inventory[ammo_index] && !ent->client->pers.weapon_clip [clip_index])
		{
			cprintf (ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}
		/*
		if (ent->client->pers.inventory[ammo_index] < item->quantity)
		{
			cprintf (ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}
		*/
	}

	// change to this weapon when down
	ent->client->newweapon = item;
}

// RAFAEL 14-APR-98
void Use_Weapon2 (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;
	int			index;
	int			clip_index;


	// JOSEPH 5-FEB-99-B
	if (strcmp (item->pickup_name, "Pipe") == 0)
	{
		// Ridah, changed this so it defaults to Crowbar if we have it
		if (ent->client->pers.weapon != FindItem ("Crowbar"))
		{
			item = FindItem ("Crowbar");
			index = ITEM_INDEX (item);
			if (!ent->client->pers.inventory[index])
			{
				item = FindItem ("Pipe");
			}
		}
	}
	// END JOSEPH

	else if (strcmp (item->pickup_name, "Pistol") == 0)
	{
		if (item == ent->client->pers.weapon)
		{
			item = FindItem ("SPistol");
			index = ITEM_INDEX (item);
			if (!ent->client->pers.inventory[index])
			{
				item = FindItem ("Pistol");
			}
		}
	}
	
	
	// see if we're already using it
	if (item == ent->client->pers.weapon)
		return;
	
	if (item->ammo)
	{
		ammo_item = FindItem(item->ammo);
		ammo_index = ITEM_INDEX(ammo_item);
		clip_index = QweryClipIndex (item);

		if (!ent->client->pers.inventory[ammo_index] && !g_select_empty->value && !ent->client->pers.weapon_clip [clip_index])
		{
			cprintf (ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}
	}

	// change to this weapon when down
	ent->client->newweapon = item;
	
}
// END 14-APR-98

/*
================
Drop_Weapon
================
*/
void Drop_Weapon (edict_t *ent, gitem_t *item)
{
	int		index;

	if ((int)(dmflags->value) & DF_WEAPONS_STAY)
		return;
// GUNRACE ADD
	if (deathmatch_value) //add hypov8 bugfix can spawn weapon when in spec
		return;
// GUNRACE END

	index = ITEM_INDEX(item);
	// see if we're already using it
	if ( ((item == ent->client->pers.weapon) || (item == ent->client->newweapon))&& (ent->client->pers.inventory[index] == 1) )
	{
		cprintf (ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	Drop_Item (ent, item);
	ent->client->pers.inventory[index]--;
}


/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
#define FRAME_FIRE_FIRST		(FRAME_ACTIVATE_LAST + 1)
//#define FRAME_IDLE_FIRST		(FRAME_FIRE_LAST + 1) //deleted goat
#define FRAME_IDLE_FIRST		(FRAME_RELOAD_LAST + 1) //add hypov8
#define FRAME_DEACTIVATE_FIRST	(FRAME_IDLE_LAST + 1)

#define FRAME_OFFSET_FOR_SPISTOL	43
#define FRAME_OFFSET_FOR_SPISTOL_WITH_SILENCER 70

void Weapon_Generic (edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_RELOAD_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int *pause_frames, int *fire_frames, void (*fire)(edict_t *ent))
{
	int		n;
	qboolean stopWepChange = false; //add hypov8 todo
// RAFAEL 01-11-99

	if (ent->client->weaponstate ==	WEAPON_RELOADING_SPISTOL)
	{
		if (ent->client->ps.gunframe == FRAME_OFFSET_FOR_SPISTOL)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate ==	WEAPON_RELOADING)
	{
		if (ent->client->ps.gunframe == FRAME_RELOAD_LAST) // add hypov8 was FRAME_FIRE_LAST
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_RELOAD_CYCLE)
	{
		/*
		if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;		
		}
		else
		*/
		{
			ent->client->ps.gunframe += 2; //GUNRACE_ADD //FREDZ fast shotgun reload
			return;
		}
	}

	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->pers.weapon == FindItem("SPistol"))
		{
			if (ent->client->pers.silencer_shots && ent->client->ps.gunframe == FRAME_OFFSET_FOR_SPISTOL_WITH_SILENCER)
			{
				ChangeWeapon (ent);
				return;
			}
			if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
			{
				ChangeWeapon (ent);
				return;
			}
		}
		else
		{
			if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
			{
				ChangeWeapon (ent);
				return;
			}
			else if ((ent->client->weaponstate != WEAPON_FIRING) && (ent->client->ps.gunframe < FRAME_DEACTIVATE_FIRST))
			{
				ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
				return;
			}
		}
/*
		else if ((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4)
		{
			ent->client->anim_priority = ANIM_REVERSE;
			if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4+1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304+1;
				ent->client->anim_end = FRAME_pain301;
				
			}
		}
*/
		ent->client->ps.gunframe++;

		// Ridah, faster weapon changing in deathmatch
		if (deathmatch_value && ent->client->ps.gunframe < FRAME_DEACTIVATE_LAST)
			ent->client->ps.gunframe++;

		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		ent->client->ps.gunframe++;

		// Ridah, faster weapon changing in deathmatch
		if (deathmatch_value && ent->client->ps.gunframe < FRAME_ACTIVATE_LAST)
			ent->client->ps.gunframe++;

		return;
	}
	//GUNRACE_START
	//hypov8 fix for hmg and pistol multi shot not finishing all rounds
	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		if (ent->client->pers.weapon == FindItem("Pistol"))
			if (ent->client->ps.gunframe < FRAME_DEACTIVATE_LAST -2 ) //lowerGun1 - 2)
				stopWepChange = true;

		if ( ent->client->pers.weapon == FindItem("Heavy machinegun"))
			if (ent->client->ps.gunframe < FRAME_DEACTIVATE_LAST -2 ) //lowerHMG1 - 2)
				stopWepChange = true;
	}
	//GUNRACE_END

	
	if ((ent->client->newweapon) /*&& (ent->client->weaponstate != WEAPON_FIRING)*/)
	{
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
/*
		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
		{
			ent->client->anim_priority = ANIM_REVERSE;
			if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4+1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304+1;
				ent->client->anim_end = FRAME_pain301;
				
			}
		}
*/
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			
			if (ent->client->clip_index == CLIP_PISTOL && ent->client->pers.weapon_clip[ent->client->clip_index] >= 1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			else if (ent->client->clip_index == CLIP_SHOTGUN && ent->client->pers.weapon_clip[ent->client->clip_index] >= 1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			//GUNRACE_START
			else if (ent->client->clip_index == CLIP_SSHOTGUN && ent->client->pers.weapon_clip[ent->client->clip_index] >= 2)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			else if (ent->client->clip_index == CLIP_M41A && ent->client->pers.weapon_clip[ent->client->clip_index] >= 1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			else if (ent->client->clip_index == CLIP_UZI && ent->client->pers.weapon_clip[ent->client->clip_index] >= 1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			else if (ent->client->clip_index == CLIP_M60 && ent->client->pers.weapon_clip[ent->client->clip_index] >= 1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			else if (ent->client->clip_index == CLIP_BENELLI && ent->client->pers.weapon_clip[ent->client->clip_index] >= 1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			else if (ent->client->clip_index == CLIP_SPAS12 && ent->client->pers.weapon_clip[ent->client->clip_index] >= 1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			else if (ent->client->clip_index == CLIP_MP5 && ent->client->pers.weapon_clip[ent->client->clip_index] >= 1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			else if (ent->client->clip_index == CLIP_AK47 && ent->client->pers.weapon_clip[ent->client->clip_index] >= 1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			//hypov8 todo add new weps
			//GUNRACE_END
			else if (ent->client->clip_index == CLIP_TOMMYGUN && ent->client->pers.weapon_clip[ent->client->clip_index] >=1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			else if (ent->client->clip_index == CLIP_308 && ent->client->pers.weapon_clip[ent->client->clip_index] >=1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			else if (ent->client->clip_index == CLIP_GRENADES && ent->client->pers.weapon_clip[ent->client->clip_index] >=1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
#if 0  //GUNRACE_DISABLED
			else if (ent->client->clip_index == CLIP_ROCKETS && ent->client->pers.weapon_clip[ent->client->clip_index] >=1)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
#endif
			else if ((!ent->client->ammo_index) || 
				( ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity))
			{
				//GUNRACE_START
				ent->client->ps.gunframe = FRAME_FIRE_FIRST + 2; //hypov8 machete. start at frame 7
				//GUNRACE_END
				//ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
			}
			else 
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
		}
		else
		{
			if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
			{
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			if (pause_frames)
			{
				for (n = 0; pause_frames[n]; n++)
				{
					if (ent->client->ps.gunframe == pause_frames[n])
					{
						if (rand()&15)
							return;
					}
				}
			}

			ent->client->ps.gunframe++;
			return;
		}
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		for (n = 0; fire_frames[n]; n++)
		{
			if (ent->client->ps.gunframe == fire_frames[n])
			{
				if (ent->client->quad_framenum > level.framenum)
					gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
/*
				if (ent->client->anim_priority <= ANIM_JUMP 
					&& (!ent->groundentity || (!ent->client->anim_run && !ent->client->anim_slide)))
				{
					// start the animation
					ent->client->anim_priority = ANIM_ATTACK;
					if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
					{
						if (ent->s.frame != FRAME_crch_shoot_01)
							ent->s.frame = FRAME_crch_shoot_01-1;
						ent->client->anim_end = FRAME_crch_shoot_05;
					}
					else
					{
						if (ent->s.frame != FRAME_shoot_01)
							ent->s.frame = FRAME_shoot_01-1;
						ent->client->anim_end = FRAME_shoot_04;
					}
				}
*/
				fire (ent);
				break;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.gunframe++;

		// JOSEPH 5-FEB-99-B
		// Blackjack hack
        if ((ent->client->ps.gunframe == 9) &&
			(!((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK)) &&
			(!strcmp(ent->client->pers.weapon->pickup_name, "Pipe")))
		{
			ent->client->ps.gunframe = FRAME_IDLE_FIRST+1;
			ent->client->weaponstate = WEAPON_READY;
		}
		// END JOSEPH

		// JOSEPH 19-JAN-99
		// Crowbar hack
        else if ((ent->client->ps.gunframe == 9) &&
			(!((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK)) &&
			(!strcmp(ent->client->pers.weapon->pickup_name, "Crowbar")))
		{
			ent->client->ps.gunframe = FRAME_IDLE_FIRST+1;
			ent->client->weaponstate = WEAPON_READY;
		}
		// END JOSEPH

		//GUNRACE_START
		//stop 2nd knife attack if not holding attack
        else if ((ent->client->ps.gunframe == 11) &&
			(!((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK)) &&
			(!strcmp(ent->client->pers.weapon->pickup_name, "Machete")))
		{
			ent->client->ps.gunframe = FRAME_IDLE_FIRST+1;
			ent->client->weaponstate = WEAPON_READY;
		}
		//GUNRACE_END



		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST+1)
			ent->client->weaponstate = WEAPON_READY;
	}
}



/*
======================================================================

GRENADE

======================================================================
*/

#define GRENADE_TIMER		3.0
#define GRENADE_MINSPEED	400
#define GRENADE_MAXSPEED	800

void weapon_grenade_fire (edict_t *ent, qboolean held)
{
	vec3_t	offset;
	vec3_t	forward, right;
	vec3_t	start;
	int		damage = 125;
	float	timer;
	int		speed;
	float	radius;

	radius = damage+40;
	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	timer = ent->client->grenade_time - level.time;
	speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER);
	fire_grenade2 (ent, start, forward, damage, speed, timer, radius, held);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;

	ent->client->grenade_time = level.time + 1.0;
/*
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = FRAME_crattak1-1;
		ent->client->anim_end = FRAME_crattak3;
	}
	else
	{
		ent->client->anim_priority = ANIM_REVERSE;
		ent->s.frame = FRAME_wave08;
		ent->client->anim_end = FRAME_wave01;
	}
*/
}

void Weapon_Grenade (edict_t *ent)
{
	if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY))
	{
		ChangeWeapon (ent);
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		ent->client->weaponstate = WEAPON_READY;
		ent->client->ps.gunframe = 16;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if (ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->ps.gunframe = 1;
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0;
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
			return;
		}

		if ((ent->client->ps.gunframe == 29) || (ent->client->ps.gunframe == 34) || (ent->client->ps.gunframe == 39) || (ent->client->ps.gunframe == 48))
		{
			if (rand()&15)
				return;
		}

		if (++ent->client->ps.gunframe > 48)
			ent->client->ps.gunframe = 16;
		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		if (ent->client->ps.gunframe == 5)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/grenade_launcher/hgrena1b.wav"), 1, ATTN_NORM, 0);

		if (ent->client->ps.gunframe == 11)
		{
			if (!ent->client->grenade_time)
			{
				ent->client->grenade_time = level.time + GRENADE_TIMER + 0.2;
				ent->client->weapon_sound = gi.soundindex("weapons/grenade_launcher/hgrenc1b.wav");
			}

			// they waited too long, detonate it in their hand
			if (!ent->client->grenade_blew_up && level.time >= ent->client->grenade_time)
			{
				ent->client->weapon_sound = 0;
				weapon_grenade_fire (ent, true);
				ent->client->grenade_blew_up = true;
			}

			if (ent->client->buttons & BUTTON_ATTACK)
				return;

			if (ent->client->grenade_blew_up)
			{
				if (level.time >= ent->client->grenade_time)
				{
					ent->client->ps.gunframe = 15;
					ent->client->grenade_blew_up = false;
				}
				else
				{
					return;
				}
			}
		}

		if (ent->client->ps.gunframe == 12)
		{
			ent->client->weapon_sound = 0;
			weapon_grenade_fire (ent, false);
		}

		if ((ent->client->ps.gunframe == 15) && (level.time < ent->client->grenade_time))
			return;

		ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == 16)
		{
			ent->client->grenade_time = 0;
			ent->client->weaponstate = WEAPON_READY;
		}
	}
}



// JOSEPH 18-DEC-98
void Think_Eject_Shell (edict_t *ent)
{
	if (!ent->misstime--)
	{	
		ent->nextthink = 0;
		G_FreeEdict(ent);
	}
	else
	{
		if (ent->misstime <= 20)
		{
			if (ent->misstime == 20)
			{
				ent->s.renderfx2 |= RF2_PASSALPHA;
				ent->s.effects = 1;		// this is full alpha now
			}
			
			ent->s.effects += (255/20);
		
			if (ent->s.effects > 255)
				ent->s.effects = 255;
		}

//		ent->s.angles[0] += ent->avelocity[0];
		ent->s.angles[1] += ent->avelocity[1];
//		ent->s.angles[2] += ent->avelocity[2];
		ent->nextthink = level.time + 0.1;
	}
}
// END JOSEPH

// JOSEPH 7-JAN-99
void Eject_Shotgun_Shell (edict_t *ent)
{
	edict_t	*dropped = NULL;
	vec3_t	forward, right, up;
	vec3_t	offset;

	if (!(ent->client))
		return;

	if (deathmatch_value)
		return;

	//gi.dprintf("angles = %s\n", vtos(ent->s.angles));

	dropped = G_Spawn();
	
	if (!dropped)
		return;

	dropped->s.modelindex = gi.modelindex("models/weapons/sshell_md2/tris.md2");
		
	dropped->movetype = MOVETYPE_BOUNCE;
	
	dropped->mins[0] = -4;
	dropped->mins[1] = -4;
	dropped->mins[2] = -2;
	
	dropped->maxs[0] =  4;
	dropped->maxs[1] =  4;
	dropped->maxs[2] =  4;

	AngleVectors (ent->client->v_angle, forward, right, up);

	dropped->s.angles[YAW] = ent->client->v_angle[YAW];

	VectorSet(offset, 0, 16, ent->viewheight -2 );
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, dropped->s.origin);

	VectorScale (forward, 5 + ((rand()&15)-8), dropped->velocity);
    VectorMA (dropped->velocity, 50 + (rand()&31), right, dropped->velocity);
	dropped->velocity[2] = 150 + (rand()&31);

	// Ridah, changed this so they don't stick in the ground so much
	VectorSet(dropped->avelocity, 0/*((rand()&7)-4)*/, ((rand()&15)-8), ((rand()&7)-4));

	dropped->think = Think_Eject_Shell;
	dropped->misstime = 40;
	dropped->nextthink = level.time + 0.1;

	dropped->s.renderfx2 |= RF2_NOSHADOW;

	gi.linkentity (dropped);

	return;
}
// END JOSEPH




//======================================================================



// JOSEPH 12-OCT-98
void Blackjack_Hit (edict_t *ent, vec3_t vorigin, int damage)
{
	vec3_t		start;
	vec3_t		forward, right;
	int			kick = 2;
	vec3_t		offset;

	if (is_quad)
		damage *= 4;
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	
	VectorSet(offset, 0, 0, ent->viewheight);
	
	VectorAdd (offset, vorigin, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if (fire_blackjack (ent, start, forward, damage, kick, MOD_BLACKJACK))
		ent->client->ps.weapon_usage++;

	if (ent->client->ps.weapon_usage > 20)
	{
		ent->client->ps.rdflags &= ~RDF_SKINPLUS;
		ent->client->ps.rdflags |= RDF_SKINPLUSPLUS;
	}
	else if (ent->client->ps.weapon_usage > 10)
		ent->client->ps.rdflags |= RDF_SKINPLUS;
	
	
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_BLACKJACK_SWING);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
}

void Weapon_Blackjack_Hit (edict_t *ent)
{
	int		damage;

	if (deathmatch_value)
		damage = 10;
	else
		damage = 8;

	Blackjack_Hit (ent, vec3_origin, damage); 
   	ent->client->ps.gunframe++;	
}




void Weapon_Blackjack (edict_t *ent)
{
	static int	pause_frames[]	= {0, 0};
	static int	fire_frames[]	= {8, 11, 0};
	
	
	if (ent->client->ps.gunframe < 5)
	{
		ent->client->ps.rdflags &= ~RDF_SKINPLUS;
		ent->client->ps.rdflags &= ~RDF_SKINPLUSPLUS;
	}
	
	Weapon_Generic (ent, 5, 13, 31, 36, pause_frames, fire_frames, Weapon_Blackjack_Hit);

	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0;
	
}
// END JOSEPH

// JOSEPH 19-JAN-99
void Crowbar_Hit (edict_t *ent, vec3_t vorigin, int damage)
{
	vec3_t		start;
	vec3_t		forward, right;
	int			kick = 2;
	vec3_t		offset;

	if (is_quad)
		damage *= 4;
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	
	VectorSet(offset, 0, 0, ent->viewheight);
	
	VectorAdd (offset, vorigin, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if (fire_crowbar (ent, start, forward, damage, kick, MOD_CROWBAR))
		ent->client->ps.weapon_usage++;

	if (ent->client->ps.weapon_usage > 20)
	{
		ent->client->ps.rdflags &= ~RDF_SKINPLUS;
		ent->client->ps.rdflags |= RDF_SKINPLUSPLUS;
	}
	else if (ent->client->ps.weapon_usage > 10)
		ent->client->ps.rdflags |= RDF_SKINPLUS;
	
	
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_CROWBAR_SWING);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
}

void Weapon_Crowbar_Hit (edict_t *ent)
{
	int		damage;

	if (deathmatch_value)
		damage = 50;	// This should be very powerful in deathmatch
	else
		damage = 12;// - (int)(skill->value-1);

	Crowbar_Hit (ent, vec3_origin, damage); 
   	ent->client->ps.gunframe++;	
}

void Weapon_Crowbar (edict_t *ent)
{
	static int	pause_frames[]	= {0, 0};
	static int	fire_frames[]	= {8, 11, 0};
	
	
	if (ent->client->ps.gunframe < 5)
	{
		ent->client->ps.rdflags &= ~RDF_SKINPLUS;
		ent->client->ps.rdflags &= ~RDF_SKINPLUSPLUS;
	}
	
	Weapon_Generic (ent, 5, 13, 31, 36, pause_frames, fire_frames, Weapon_Crowbar_Hit);

	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0;
	
}
// END JOSEPH 



// JOSEPH 7-JAN-99
void Eject_Pistol_Shell (edict_t *ent)
{
	edict_t	*dropped = NULL;
	vec3_t	forward, right, up;
	vec3_t	offset;

	if (!(ent->client))
		return;

	if (deathmatch_value)
		return;

	//gi.dprintf("angles = %s\n", vtos(ent->s.angles));

	dropped = G_Spawn();
	
	if (!dropped)
		return;

	dropped->s.modelindex = gi.modelindex("models/weapons/shell_md2/tris.md2");
		
	dropped->movetype = MOVETYPE_BOUNCE;

	dropped->mins[0] = -4;
	dropped->mins[1] = -4;
	dropped->mins[2] = -2;
	
	dropped->maxs[0] =  4;
	dropped->maxs[1] =  4;
	dropped->maxs[2] =  4;

	AngleVectors (ent->client->v_angle, forward, right, up);

	dropped->s.angles[YAW] = ent->client->v_angle[YAW];

	VectorSet(offset, 0, 16, ent->viewheight -2 );
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, dropped->s.origin);

	VectorScale (forward, 5 + ((rand()&15)-8), dropped->velocity);
    VectorMA (dropped->velocity, 50 + (rand()&31), right, dropped->velocity);
	dropped->velocity[2] = 150 + (rand()&31);

	// Ridah, changed this so they don't stick in the ground so much
	VectorSet(dropped->avelocity, 0/*((rand()&7)-4)*/, ((rand()&15)-8), ((rand()&7)-4));
	
	dropped->think = Think_Eject_Shell;
	dropped->misstime = 40;
	dropped->nextthink = level.time + 0.1;

	dropped->s.renderfx2 |= RF2_NOSHADOW;

	gi.linkentity (dropped);

	return;
}

// JOSEPH 7-JAN-99
void Eject_Tommy_Shell (edict_t *ent)
{
	edict_t	*dropped = NULL;
	vec3_t	forward, right, up;
	vec3_t	offset;

	if (!(ent->client))
		return;

	if (deathmatch_value)
		return;

	//gi.dprintf("angles = %s\n", vtos(ent->s.angles));

	dropped = G_Spawn();

	if (!dropped)
		return;
	
	dropped->s.modelindex = gi.modelindex("models/weapons/shell_md2/tris.md2");
		
	dropped->movetype = MOVETYPE_BOUNCE;
	
	dropped->mins[0] = -4;
	dropped->mins[1] = -4;
	dropped->mins[2] = -2;
	
	dropped->maxs[0] =  4;
	dropped->maxs[1] =  4;
	dropped->maxs[2] =  4;

	AngleVectors (ent->client->v_angle, forward, right, up);

	dropped->s.angles[YAW] = ent->client->v_angle[YAW];

	VectorSet(offset, 0, 16, ent->viewheight -2 );
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, dropped->s.origin);

	VectorScale (forward, 5 + ((rand()&15)-8), dropped->velocity);
    VectorMA (dropped->velocity, 50 + (rand()&31), right, dropped->velocity);
	dropped->velocity[2] = 150 + (rand()&31);

	// Ridah, changed this so they don't stick in the ground so much
	VectorSet(dropped->avelocity, 0/*((rand()&7)-4)*/, ((rand()&15)-8), ((rand()&7)-4));

	dropped->think = Think_Eject_Shell;
	dropped->misstime = 40;
	dropped->nextthink = level.time + 0.1;
	
	dropped->s.renderfx2 |= RF2_NOSHADOW;

	gi.linkentity (dropped);

	return;
}
// END JOSEPH

// WEAPON PISTOL
void Pistol_Fire (edict_t *ent, vec3_t vorigin, int damage)
{
	vec3_t		start;
	vec3_t		forward, right;
	int			kick = 2;
	vec3_t		offset;

	if (is_quad)
		damage *= 4;
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	
	VectorSet(offset, 0, 0, ent->viewheight);
	
	VectorAdd (offset, vorigin, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);

	ent->client->kick_angles[0] = -4;
    
	fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_PISTOL);
	
	if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE)
	{
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_PISTOL_DAMAGE_MOD | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
	}
	else
	{
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_PISTOL | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
	}
	
	if (! ( (int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value ) )
	{
		ent->client->pers.weapon_clip[ent->client->clip_index]--;
	}
}


void Weapon_Pistol_Fire(edict_t *ent)
{
	int		damage;
	int		rounds;


	if (ent->waterlevel >= 2)
	{
		// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 29;
		}
		return;
	}

	if (deathmatch_value)
		damage = 15;
	else
		damage = 10;


	if (ent->client->ps.gunframe == 6)
	{
		if (ent->client->pers.pistol_mods & WEAPON_MOD_ROF)
		{
			if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE)
			{
				damage *= 2;
			}
			ent->client->ps.gunframe = 8;
		}
		else
		{
			if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE)
			{
				damage *= 2;
			}
			ent->client->ps.gunframe++;
		}
		Pistol_Fire(ent, vec3_origin, damage);
		Eject_Pistol_Shell(ent);
	}
	else if (ent->client->ps.gunframe == 10)
	{
		if (!ent->client->pers.weapon_clip[ent->client->clip_index])
		{
			ent->client->reload_weapon = false;
			ent->client->weaponstate = WEAPON_RELOADING;

			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_PISTOL_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else
				rounds = MAX_PISTOL_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			ent->client->ps.gunframe++;
			if (ent->client->pers.pistol_mods & WEAPON_MOD_RELOAD)
				;//gi.dprintf ("need fast reload sound\n");
			else
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/pistol/clip_out.wav"), 1, ATTN_NORM, 0);

			if (!ent->client->pers.weapon_clip[ent->client->clip_index])
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange(ent);
			}
		}
		else
			ent->client->ps.gunframe = 29;
	}

}

void Weapon_Pistol(edict_t *ent)
{
	int		rounds;
	//static int	pause_frames[]	= {46, 0, 0};
	static int	pause_frames[] = { 0, 0, 0 };
	static int	fire_frames[] = { 6, 10, 0 };

	if (ent->client->reload_weapon)
	{
		if (ent->client->ps.gunframe >= 28 && ent->client->ps.gunframe <= 44)
		{
			if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_PISTOL_ROUNDS)
			{
				ent->client->ps.gunframe = 10;
				ent->client->weaponstate = WEAPON_RELOADING;

				ent->client->pers.inventory[ent->client->ammo_index] += ent->client->pers.weapon_clip[ent->client->clip_index];

				if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_PISTOL_ROUNDS)
				{
					rounds = ent->client->pers.inventory[ent->client->ammo_index];
				}
				else
					rounds = MAX_PISTOL_ROUNDS;

				ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
				ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			}

			ent->client->reload_weapon = false;
		}
		else
			ent->client->reload_weapon = false;
	}
	else if (!ent->client->pers.weapon_clip[ent->client->clip_index] && ent->client->pers.inventory[ent->client->ammo_index])
	{
		if (ent->client->ps.gunframe >= 28 && ent->client->ps.gunframe <= 44)
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = 10;
			ent->client->weaponstate = WEAPON_RELOADING;

			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_PISTOL_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else
				rounds = MAX_PISTOL_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
		}
	}

	if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE)
		ent->client->ps.gunindex = gi.modelindex("models/weapons/v_colt/magnum.mdx");

	Weapon_Generic(ent, 5, 28, 44, 49, pause_frames, fire_frames, Weapon_Pistol_Fire);

	if (ent->client->pers.pistol_mods & WEAPON_MOD_RELOAD)
	{
		if (ent->client->ps.gunframe == 21)
			ent->client->ps.gunframe = 26;
	}

	if (ent->client->ps.gunframe >= 10 && ent->client->ps.gunframe <= 28)
	{
		ent->client->ps.model_parts[PART_GUN_R_HAND].invisible_objects = 0;
	}
	else
		ent->client->ps.model_parts[PART_GUN_R_HAND].invisible_objects = (1 << 0 | 1 << 1);

	if (ent->client->ps.gunframe == 1)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/pistol/holster.wav"), 1, ATTN_NORM, 0);

	if (ent->client->ps.gunframe == 11)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/pistol/clip_out.wav"), 1, ATTN_NORM, 0);

	if (ent->client->ps.gunframe == 18)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/pistol/clip_in.wav"), 1, ATTN_NORM, 0);

	if (ent->client->ps.gunframe >= 10 && ent->client->ps.gunframe <= 24)
	{
		ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = 0;
	}
	else
		ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = (1 << 0 | 1 << 1);

}

void SPistol_Fire (edict_t *ent, vec3_t vorigin, int damage)
{
	vec3_t		start;
	vec3_t		forward, right;
	int			kick = 2;
	vec3_t		offset;

	// is_silenced = MZ_SILENCED;

	if (is_quad)
		damage *= 4;
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	
	VectorSet(offset, 0, 0, ent->viewheight);
	
	VectorAdd (offset, vorigin, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	
	ent->client->kick_angles[0] = -4;
    
	fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_SILENCER);
	
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_SPISTOL | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value ) )
		ent->client->pers.weapon_clip[ent->client->clip_index]--;

	if (!ent->client->pers.weapon_clip[ent->client->clip_index] && !ent->client->pers.inventory[ ent->client->ammo_index])
		{
			if (level.time >= ent->pain_debounce_time)
			{
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent->pain_debounce_time = level.time + 1;
			}
			NoAmmoWeaponChange (ent);
		}
    
	Eject_Pistol_Shell(ent);
}


void Weapon_SPistol_Fire (edict_t *ent)
{
	int		damage;
	int		rounds;


	if (ent->waterlevel >= 2)
	{
		//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 50;
		}
		return;
	}

	if (deathmatch_value)
		damage = 15;
	else
		damage = 10;

	if (ent->client->ps.gunframe == 21)
	{
		if (ent->client->pers.pistol_mods & WEAPON_MOD_ROF)
		{
			ent->client->ps.gunframe = 23;	
		}
		else
		{
			ent->client->ps.gunframe++;	
		}

		if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE)
		{
			damage *= 2;
		}

		SPistol_Fire (ent, vec3_origin, damage); 
		Eject_Pistol_Shell(ent);
		ent->client->pers.silencer_shots--;

		if (!ent->client->pers.silencer_shots)
			ent->client->ps.gunframe = 43;

	}
	else if (ent->client->ps.gunframe == 25)
	{
		if (!ent->client->pers.weapon_clip[ent->client->clip_index])
		{
			ent->client->reload_weapon = false;
			ent->client->weaponstate = WEAPON_RELOADING;
			
			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_PISTOL_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else 
				rounds = MAX_PISTOL_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			ent->client->ps.gunframe++;	
			if (ent->client->pers.pistol_mods & WEAPON_MOD_RELOAD)
				;//gi.dprintf ("need fast reload sound\n");
			else
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/pistol/clip_out.wav"), 1, ATTN_NORM, 0);

			if (! ent->client->pers.weapon_clip[ent->client->clip_index])
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
		}
		else
			ent->client->ps.gunframe = 50;
	}
	
	else if (ent->client->ps.gunframe == 44) 
	{
		if (!(ent->client->pers.silencer_shots))
			ent->client->ps.gunframe++;
		else
			ent->client->ps.gunframe = 50;
	}
	

}


void Weapon_SPistol (edict_t *ent)
{
	static int	pause_frames[]	= {0, 0, 0};
	static int	fire_frames[]	= {21, 25, 44, 0};
	int		rounds;

// gi.dprintf ("silencer shots left: %d\n", ent->client->pers.silencer_shots);

	if (ent->client->ps.gunframe < 22)
	{
		ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0;
	}
	else if (ent->client->weaponstate == WEAPON_RELOADING_SPISTOL && ent->client->ps.gunframe < 40)
	{
		if (ent->client->ps.gunframe < 39)
		ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = 0;
		else
			ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = (1<<0 | 1<<1);

		ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0;
	}
	else
	{
		ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = (1<<0 | 1<<1);
		ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = (1<<0 | 1<<1);
	}

	if (ent->client->ps.gunframe == 49)
	{
		if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("pistol"))] )
		{
			ent->client->newweapon = FindItem ("pistol");
			
		}
		else
			NoAmmoWeaponChange (ent);

		ent->client->ps.gunframe = 71;
		ent->client->weaponstate = WEAPON_DROPPING;
		{
			gitem_t *it;
			int		index;
			it = FindItem ("SPistol");
			index = ITEM_INDEX (it);
			ent->client->pers.inventory[index] = 0;
		}
	}
	else if (ent->client->reload_weapon)
	{
		if (ent->client->ps.gunframe >= 50 && ent->client->ps.gunframe <= 65)
		{
		
			if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_PISTOL_ROUNDS)
			{
				ent->client->ps.gunframe = 25;
				ent->client->weaponstate = WEAPON_RELOADING_SPISTOL;
				
				ent->client->pers.inventory[ent->client->ammo_index] += ent->client->pers.weapon_clip[ent->client->clip_index];	

				if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_PISTOL_ROUNDS)
				{
					rounds = ent->client->pers.inventory[ent->client->ammo_index];
				}
				else 
					rounds = MAX_PISTOL_ROUNDS;

				ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
				ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			}

			ent->client->reload_weapon = false;
		}
	}
	else if (!ent->client->pers.weapon_clip[ent->client->clip_index] && ent->client->pers.inventory[ent->client->ammo_index])
	{
		
		if (ent->client->ps.gunframe >= 50 && ent->client->ps.gunframe <= 65)
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = 25;
			ent->client->weaponstate = WEAPON_RELOADING_SPISTOL;

			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_PISTOL_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else 
				rounds = MAX_PISTOL_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
		}
	}

	if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE)
		ent->client->ps.gunindex = gi.modelindex("models/weapons/silencer_mdx/magnum.mdx");
			

	Weapon_Generic (ent, 20, 49, 65, 75, pause_frames, fire_frames, Weapon_SPistol_Fire);
	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0; //hypov8 unhide
	ent->client->ps.model_parts[PART_GUN_R_HAND].invisible_objects = 0; //hypov8 unhide
	ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = 0; //hypov8 unhide
	//hypov8 un-used. missing invisible_objects code
		
	if (ent->client->ps.gunframe == 1)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/pistol/silencerattatch.wav"), 1, ATTN_NORM, 0);
	
}


// JOSEPH 26-AUG-98 
/*
======================================================================

TOMMYGUN 

======================================================================
*/

void Tommygun_Fire (edict_t *ent)
{
	int	i;
	vec3_t		start;
	//vec3_t	fireorigin;
	vec3_t		forward, right;
	vec3_t		angles;
	int			damage = 14; //was 8(+mplayer add)
	int			kick = 2;
	vec3_t		offset;
	int			wpFlags = MZ_MACHINEGUN; //tommygun sound

	if (ent->waterlevel >= 2)
	{
		// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 30;
		}
		return;
	}

	// First shot
	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		// ent->client->machinegun_shots = 0;
		ent->client->ps.gunframe = 30;
		return;
	}

	/*if (deathmatch_value)
		damage *= 1.75;*/ //hypov8 =14

	if (ent->client->pers.inventory[ent->client->ammo_index] == 0 && ent->client->pers.weapon_clip[ent->client->clip_index] == 0) 
		return;

	if (ent->client->ps.gunframe == 5)
		ent->client->ps.gunframe = 4;
	else
		ent->client->ps.gunframe = 5;

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	// Tommy is too powerful in real-mode
	if (dm_realmode->value)
		damage /= 2;

	// Bounce
	for (i=1 ; i<3 ; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.22;
		ent->client->kick_angles[i] = crandom() * 0.6;
	}
	ent->client->kick_origin[0] = crandom() * 0.22;
	// ent->client->kick_angles[0] = ent->client->machinegun_shots * -.5;

	// get start / end positions
	VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors (angles, forward, right, NULL);
	
	VectorSet(offset, 0, 0, ent->viewheight-1);
	
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	{	//add hypov8. Crouch has better accurecy
		int spreadH = DEFAULT_BULLET_HSPREAD;
		int spreadV = DEFAULT_BULLET_VSPREAD;

		if ((ent->client->ps.pmove.pm_flags & PMF_DUCKED)){
			spreadH = 180;
			spreadV = 280;
		}

		//GURACE_START
		if (ent->client->pers.weapon == FindItem("mp5"))
		{
			spreadH = 35;
			spreadV = 35;
			kick = 1;
			fire_bullet(ent, start, forward, damage, kick, spreadH, spreadV, MOD_MP5);
			wpFlags = MZ_PISTOL;
		}
		else if (ent->client->pers.weapon == FindItem("ak47"))
		{
			spreadH = 120;
			spreadV = 150;
			kick = 4;
			damage = 20;
			fire_bullet(ent, start, forward, damage, kick, spreadH, spreadV, MOD_MP5);
			wpFlags = MZ_ROCKET; // MZ_SPISTOL;
		}
		else
		{
			damage = 14;
			//hypov8 todo add new weps //machineguns
			//GUNRACE_END
			fire_bullet(ent, start, forward, damage, kick, spreadH, spreadV, MOD_MACHINEGUN);
		}
	}

	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	
	gi.WriteByte (wpFlags | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	// Disable for unlimited ammo
	if (! ( (int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value ) )
	{
		ent->client->pers.weapon_clip [ent->client->clip_index]--;

		if (!(ent->client->pers.inventory[ent->client->ammo_index]) && !ent->client->pers.weapon_clip [ent->client->clip_index])
		{
			NoAmmoWeaponChange (ent);
			ent->client->ps.gunframe = 30;		
		}
		else if (!ent->client->pers.weapon_clip [ent->client->clip_index])
		{
			ent->client->reload_weapon = true;
			ent->client->ps.gunframe = 30;		
			// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/machinegun/machgcock.wav"), 1, ATTN_NORM, 0);
		}
	}

	if (!deathmatch_value)	// uses up bandwidth
		Eject_Tommy_Shell(ent);
}

void Weapon_Tommygun (edict_t *ent)
{
	int		rounds;
	static int	pause_frames[]	= {44, 0, 0};
	static int	fire_frames[]	= {4, 5, 0};
	
	if (ent->client->pers.inventory[ent->client->ammo_index] == 0 && ent->client->pers.weapon_clip[ent->client->clip_index] == 0
		&& ent->client->weaponstate != WEAPON_DROPPING) 
	{
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
	}
	else if (ent->client->reload_weapon)
	{
		if (ent->client->ps.gunframe >= 30 && ent->client->ps.gunframe <= 40)
		{
			if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_TOMMYGUN_ROUNDS)
			{
				ent->client->ps.gunframe = 6;
				ent->client->weaponstate = WEAPON_RELOADING;
				
				ent->client->pers.inventory[ent->client->ammo_index] += ent->client->pers.weapon_clip[ent->client->clip_index];	

				if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_TOMMYGUN_ROUNDS)
				{
					rounds = ent->client->pers.inventory[ent->client->ammo_index];
				}
				else 
					rounds = MAX_TOMMYGUN_ROUNDS;

				ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
				ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			//GURACE_START
				if (ent->client->pers.weapon == FindItem("mp5"))
					gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/mp5/cock.wav"), 1, ATTN_NORM, 0);
				else if (ent->client->pers.weapon == FindItem("ak47"))
					gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/ak47/cock.wav"), 1, ATTN_NORM, 0);
				else
			//GURACE_END
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/machinegun/machgcock.wav"), 1, ATTN_NORM, 0);
			}

			ent->client->reload_weapon = false;
		}
	}
	else if (!ent->client->pers.weapon_clip[ent->client->clip_index] && ent->client->pers.inventory[ent->client->ammo_index])
	{
		if (ent->client->ps.gunframe >= 30 && ent->client->ps.gunframe <= 40)
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = 6;
			ent->client->weaponstate = WEAPON_RELOADING;

			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_TOMMYGUN_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else 
				rounds = MAX_TOMMYGUN_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
		//GURACE_START
			if (ent->client->pers.weapon == FindItem("mp5"))
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/mp5/cock.wav"), 1, ATTN_NORM, 0);
			else if (ent->client->pers.weapon == FindItem("ak47"))
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/ak47/cock.wav"), 1, ATTN_NORM, 0);
			else
		//GURACE_END
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/machinegun/machgcock.wav"), 1, ATTN_NORM, 0);
		}
	}

	Weapon_Generic (ent, 3, 29, 40, 46, pause_frames, fire_frames, Tommygun_Fire);
	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0; //hypov8 unhide
	ent->client->ps.model_parts[PART_GUN_R_HAND].invisible_objects = 0; //hypov8 unhide

}
// END JOSEPH



/*
======================================================================

  FLAMETHROWER

======================================================================
*/

void FlameThrower_Fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		angles;
	int			damage = 2;
	int			kick = 0;
	static int flamesnd = 0;
//	vec3_t		offset;

	if (ent->waterlevel >= 2)
	{
		// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 14;
			ent->s.renderfx2 &= ~RF2_FLAMETHROWER;
		}
		return;
	}

	if (deathmatch_value && rand()%2)
		damage = 1;

	if (ent->health <= 0)
		return;

	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		// ent->client->machinegun_shots = 0;
		ent->client->ps.gunframe = 14;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/flame_thrower/flameend.wav"), 1, ATTN_NORM, 0);
		flamesnd = 0;
		ent->s.renderfx2 &= ~RF2_FLAMETHROWER;
		return;
	}

	ent->client->ps.gunframe ++ ;

	if (ent->client->ps.gunframe == 14)
		ent->client->ps.gunframe = 6;
	
	if (ent->client->pers.inventory[ent->client->ammo_index] < 1)
	{
		ent->client->ps.gunframe = 6; 
		
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
		ent->s.renderfx2 &= ~RF2_FLAMETHROWER;
		return;
	}

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	// get start / end positions
	VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors (angles, forward, right, NULL);
	
	VectorCopy( ent->s.origin, start );
	
	fire_flamethrower (ent, start, forward, damage, kick, MOD_FLAMETHROWER);

	ent->s.renderfx2 |= RF2_FLAMETHROWER;

	flamesnd++;
	
	if (flamesnd > 2)
		flamesnd = 0;

	if (flamesnd == 1)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/flame_thrower/flame2.wav"), 1, ATTN_NORM, 0);
	else if (flamesnd == 2)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/flame_thrower/flame3.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/flame_thrower/flame1.wav"), 1, ATTN_NORM, 0);


	if (! ( (int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value ) )
	{
		if ((ent->client->pers.inventory[ent->client->ammo_index] -= 2) < 0)
			ent->client->pers.inventory[ent->client->ammo_index] = 0;
	}
}

void Weapon_FlameThrower (edict_t *ent)
{
	static int	pause_frames[]	= {18, 24, 0};

	static int	fire_frames[]	= {6, 7, 8, 9, 10, 11, 12, 13};

	Weapon_Generic (ent, 5, 13, 25, 31, pause_frames, fire_frames, FlameThrower_Fire);
	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0; //hypov8 unhide
	
}



//GURACE_START
#define SHOTGUN_RAISE_END 5

#define SHOTGUN_SHOOT_START 6
#define SHOTGUN_SHOOT_2ND 8 //8/9?
#define SHOTGUN_SHOOT_3RD 11
#define SHOTGUN_SHOOT_4TH 13
#define SHOTGUN_SHOOT_END 16

#define SHOTGUN_RELOAD_CYCLE_START  17
#define SHOTGUN_RELOAD_CYCLE_END 

#define SHOTGUN_RELOAD_CYCLE2_START  21
#define SHOTGUN_RELOAD_CYCLE2_END 

#define SHOTGUN_RELOAD_START 29
#define SHOTGUN_RELOAD_END 32

#define SHOTGUN_IDLE_START 33
#define SHOTGUN_IDLE_END 40

#define SHOTGUN_LOWER_START 41
#define SHOTGUN_LOWER_END 45
//GUNRACE_END
/*
	SHOTGUN
*/
void shotgun_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage = 20; //hypov8 was 16
	int			kick = 50;
	int wpFlags = MZ_SHOTGUN;

	if (ent->waterlevel >= 2)
	{
		// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = SHOTGUN_IDLE_START;
		}
		return;
	}

	/*if (deathmatch_value)
		damage *= 2;*/

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorSet(offset, 0, 0,  ent->viewheight-2);

	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	
	{	//add hypov8. Crouch has better accurecy
		int spreadH = 500;
		int spreadV = 500;

		if ((ent->client->ps.pmove.pm_flags & PMF_DUCKED)){
			spreadH = 300;
			spreadV = 300;
		}

//GURACE_START
		if (ent->client->pers.weapon == FindItem("spas12"))
		{
			spreadH += 600;
			spreadV += 600;
			damage = 18;
			fire_shotgun(ent, start, forward, damage, kick, spreadH, spreadV, 18, MOD_SPAS12);
			wpFlags = MZ_GRENADE;
		}
		else if (ent->client->pers.weapon == FindItem("benelli"))
		{
			spreadH = 200;
			spreadV = 200;
			damage = 90; //single shot
			fire_shotgun(ent, start, forward, damage, kick, spreadH, spreadV, 1, MOD_BENELLI);
			wpFlags = MZ_RAILGUN;
		}
		else
		{
			damage = 20;
			//hypov8 todo add new weps //shotguns
			//GUNRACE_END
			fire_shotgun(ent, start, forward, damage, kick, spreadH, spreadV, DEFAULT_SAWED_SHOTGUN_COUNT, MOD_SHOTGUN);
		}
	}

	VectorScale (forward, -2, ent->client->kick_origin);
	

// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(wpFlags | is_silenced); //MZ_SHOTGUN
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	ent->client->ps.gunframe++;
	
	if (! ( (int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value ) )
		ent->client->pers.weapon_clip[ent->client->clip_index]--;
	
	Eject_Shotgun_Shell(ent);
}

void Weapon_Shotgun_Fire (edict_t *ent)
{
	if (ent->client->ps.gunframe == SHOTGUN_SHOOT_START)
	{
		if (ent->client->pers.weapon_clip[ent->client->clip_index])
			shotgun_fire (ent);
		else
		{
			ent->client->ps.gunframe = SHOTGUN_RELOAD_CYCLE_START;
			ent->client->reload_weapon = true;
			ent->client->weaponstate = WEAPON_RELOAD_CYCLE;
		}
	}
//GURACE_START
	else if (ent->client->ps.gunframe == SHOTGUN_SHOOT_2ND
		|| ent->client->ps.gunframe == SHOTGUN_SHOOT_3RD
		||ent->client->ps.gunframe == SHOTGUN_SHOOT_4TH)
	{
		if (ent->client->pers.weapon == FindItem("benelli"))
			ent->client->ps.gunframe = 15;
		else if (ent->client->pers.weapon == FindItem("spas12"))
			ent->client->ps.gunframe += 1; //advance 1 frames
		else
			ent->client->ps.gunframe += 2; //advance 2 frames
		//ent->client->ps.gunframe += 1;
	}
//GUNRACE_END
	else if (ent->client->ps.gunframe == SHOTGUN_SHOOT_END)
	{
		if (!ent->client->pers.weapon_clip[ent->client->clip_index])
		{
			if (ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->ps.gunframe ++;
				ent->client->reload_weapon = true;
				ent->client->weaponstate = WEAPON_RELOAD_CYCLE;
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
		}
		else
		{
			ent->client->ps.gunframe = SHOTGUN_IDLE_START;
		}
	}
}

void Weapon_Shotgun (edict_t *ent)
{
	static int	pause_frames[]	= {0, 0};
	//static int	fire_frames[] = { 6, 16, 0 };
	static int	fire_frames[] = { SHOTGUN_SHOOT_START, SHOTGUN_SHOOT_2ND, SHOTGUN_SHOOT_3RD, SHOTGUN_SHOOT_4TH, SHOTGUN_SHOOT_END, 0 }; //GUNRACE_ADD modded 2x
	static int  kick_frames[]   = {-12, -9, -6, -4, -1};

	if ((ent->client->ps.gunframe >= SHOTGUN_SHOOT_START) & (ent->client->ps.gunframe <= 9)) //TODO
	{
		ent->client->kick_angles[0] = kick_frames[ent->client->ps.gunframe-SHOTGUN_SHOOT_START];
	}
	
	if (ent->client->ps.gunframe == SHOTGUN_SHOOT_START)
		ent->client->ps.rdflags |= RDF_NOLERP;
	else
		ent->client->ps.rdflags &= ~RDF_NOLERP;

	if (ent->client->reload_weapon)
	{
		if (ent->client->buttons & BUTTON_ATTACK && ent->client->pers.weapon_clip[ent->client->clip_index] >= 1 && ent->client->weaponstate != WEAPON_FIRING)
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = SHOTGUN_IDLE_START;
			ent->client->weaponstate = WEAPON_READY;
		}
		else if (ent->client->newweapon)
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = SHOTGUN_IDLE_START;
			ent->client->weaponstate = WEAPON_DROPPING;
		}
		else
		{
			if (ent->client->ps.gunframe == SHOTGUN_RELOAD_START)
			{
				if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_SHOTGUN_ROUNDS)
				{
					if (ent->client->pers.inventory[ent->client->ammo_index])
					{
						ent->client->pers.inventory[ent->client->ammo_index]--;
						ent->client->pers.weapon_clip[ent->client->clip_index]++;
// GUNRACE ADD //hypov8 reload shotty 2x
					if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_SHOTGUN_ROUNDS)
						ent->client->pers.weapon_clip[ent->client->clip_index]++;
// GUNRACE END
						if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_SHOTGUN_ROUNDS)
							ent->client->ps.gunframe = SHOTGUN_RELOAD_CYCLE2_START; 
						else
						{
							ent->client->reload_weapon = false;
							ent->client->ps.gunframe = SHOTGUN_RELOAD_START; 
							ent->client->weaponstate = WEAPON_RELOADING;
						}
					}
					else
					{
						ent->client->reload_weapon = false;
						ent->client->weaponstate = WEAPON_RELOADING;
					}
				}
				else
				{
					ent->client->reload_weapon = false;
					ent->client->weaponstate = WEAPON_RELOADING;
				}
			}
			else if (ent->client->ps.gunframe >= SHOTGUN_IDLE_START && ent->client->ps.gunframe <= SHOTGUN_IDLE_END)
			{
				if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_SHOTGUN_ROUNDS &&
					ent->client->pers.inventory[ent->client->ammo_index])
				{
					ent->client->weaponstate = WEAPON_RELOAD_CYCLE;
					ent->client->ps.gunframe = SHOTGUN_RELOAD_CYCLE_START;
				}
			}
		}
	}
	else if (!ent->client->pers.weapon_clip[ent->client->clip_index] && ent->client->weaponstate != WEAPON_DROPPING
		&& !ent->client->buttons & BUTTON_ATTACK)
	{
		ent->client->reload_weapon = true;
		ent->client->weaponstate = WEAPON_RELOAD_CYCLE;
		ent->client->ps.gunframe = SHOTGUN_RELOAD_CYCLE_START;
		
		if (!ent->client->pers.inventory[ent->client->ammo_index])
		{
			if (level.time >= ent->pain_debounce_time)
			{
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent->pain_debounce_time = level.time + 1;
			}
			NoAmmoWeaponChange (ent);
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = SHOTGUN_IDLE_START;
			ent->client->weaponstate = WEAPON_READY;
		}
	}
	
	if (ent->client->ps.gunframe == SHOTGUN_RELOAD_CYCLE2_START)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/shotgun/shotgload.wav"), 1, ATTN_NORM, 0);
	}

	Weapon_Generic (ent, SHOTGUN_RAISE_END, SHOTGUN_RELOAD_END , SHOTGUN_IDLE_END, SHOTGUN_LOWER_END, pause_frames, fire_frames, Weapon_Shotgun_Fire);
	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0; //hypov8 unhide

	/*
	if (ent->client->ps.gunframe == 1)
	{
		ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0;
	}
	*/


}


/*
BAR .308
*/
void weapon_barmachinegun_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage;
	int			kick;


	if (ent->waterlevel >= 2)
	{
		// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 43;
		}
		return;
	}

	if (deathmatch_value)
	{	
		// tone down if too much in deathmatch
		damage = 50;
		kick = 50;
	}
	else
	{
		damage = 25;
		kick = 50;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;
	VectorSet(offset, 0, 0,  ent->viewheight-1);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	fire_bullet (ent, start, forward, damage, kick, 0, 0, MOD_BARMACHINEGUN);

	// send muzzle flash
#if 1 //GUNRACE_START
	if (ent->client->ps.gunframe == 5)
	{
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte(MZ_BARMACHINEGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
	}
#else //GUNRACE_END
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	if (ent->client->ps.gunframe == 5)
		gi.WriteByte(MZ_BARMACHINEGUN | is_silenced);
	else
		gi.WriteByte (MZ_BARMACHINEGUN2 | is_silenced);

	gi.multicast(ent->s.origin, MULTICAST_PVS);
#endif

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value ) )
	{
		ent->client->pers.weapon_clip [ent->client->clip_index]--;

		if (!(ent->client->pers.inventory[ent->client->ammo_index]) && !ent->client->pers.weapon_clip [ent->client->clip_index])
		{
			NoAmmoWeaponChange (ent);
			ent->client->ps.gunframe = 43;		
		}
		else if (ent->client->pers.inventory[ent->client->ammo_index] && !(ent->client->pers.weapon_clip [ent->client->clip_index]))
		{
			ent->client->reload_weapon = true;
			ent->client->ps.gunframe = 43;		
		}
		
	}
    Eject_Tommy_Shell(ent);
}

void barmachinegun_fire (edict_t *ent)
{


	if ((ent->client->ps.gunframe == 13) /*was 19*//*  && !(ent->client->buttons & BUTTON_ATTACK)*/)
	{
		ent->client->ps.gunframe = 43;
		return;
	}
	else if (ent->client->ps.gunframe == 11) //was 12
	{
		if (ent->client->pers.pistol_mods & WEAPON_MOD_ROF /*WEAPON_MOD_COOLING_JACKET*/) //GUNRACE_ADD
		{
			ent->client->ps.gunframe = 43;
		}
	}
	else if (ent->client->ps.gunframe == 5 
			|| ent->client->ps.gunframe == 7 
			|| ent->client->ps.gunframe == 9)
	{
		weapon_barmachinegun_fire (ent);
	
		if (ent->client->pers.pistol_mods & WEAPON_MOD_ROF /*WEAPON_MOD_COOLING_JACKET*/) //GUNRACE_ADD
		{
			ent->client->pers.hmg_shots --;

			if (!ent->client->pers.hmg_shots)
			{
				ent->client->pers.pistol_mods &= ~WEAPON_MOD_ROF /*WEAPON_MOD_COOLING_JACKET*/; //GUNRACE_ADD
				
				ent->client->newweapon = ent->client->pers.weapon;
				
				if (ent->client->pers.inventory[ent->client->ammo_index] || ent->client->pers.weapon_clip[ent->client->clip_index])
					ChangeWeapon (ent);
				else
					NoAmmoWeaponChange (ent);
			}
		}
	}
	
	ent->client->ps.gunframe++;
	
}

#define MAX_BARGUN_ROUNDS	30

void Weapon_Barmachinegun (edict_t *ent)
{
	static int	pause_frames[]	= {0, 0};
	//static int	fire_frames[]	= {5, 6, 7, 8, 9, 12, 19, 0};
	static int	fire_frames[]	= {5, 6, 7, 8, 9, 11, 13, 0};
	int			rounds;
	
	if (ent->client->pers.inventory[ent->client->ammo_index] == 0 && ent->client->pers.weapon_clip[ent->client->clip_index] == 0
		&& ent->client->weaponstate != WEAPON_DROPPING) 
	{
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
	}
	else if (ent->client->reload_weapon)
	{
		if (ent->client->ps.gunframe >= 43 && ent->client->ps.gunframe <= 53)
		{
			if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_BARGUN_ROUNDS)
			{
				ent->client->ps.gunframe = 20;
				ent->client->weaponstate = WEAPON_RELOADING;
				
				ent->client->pers.inventory[ent->client->ammo_index] += ent->client->pers.weapon_clip[ent->client->clip_index];	

				if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_BARGUN_ROUNDS)
				{
					rounds = ent->client->pers.inventory[ent->client->ammo_index];
				}
				else 
					rounds = MAX_BARGUN_ROUNDS;

				ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
				ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hmg/hmgcock.wav"), 1, ATTN_NORM, 0);
			}

			ent->client->reload_weapon = false;
		}
	}
	else if (!ent->client->pers.weapon_clip[ent->client->clip_index] && ent->client->pers.inventory[ent->client->ammo_index])
	{
		if (ent->client->ps.gunframe >= 43 && ent->client->ps.gunframe <= 53)
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = 20;
			ent->client->weaponstate = WEAPON_RELOADING;

			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_BARGUN_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else 
				rounds = MAX_BARGUN_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hmg/hmgcock.wav"), 1, ATTN_NORM, 0);
		}
	}

	
	Weapon_Generic (ent, 4, 42, 53, 59, pause_frames, fire_frames, barmachinegun_fire);
	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0; //hypov8 unhide
	ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = 0; //hypov8 unhide
 	// gi.dprintf ("Frame: %d\n", ent->client->ps.gunframe);
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void Eject_Grenade_Shell (edict_t *ent)
{
	edict_t	*dropped = NULL;
	vec3_t	forward, right, up;
	vec3_t	offset;

	if (!(ent->client))
		return;

	if (deathmatch_value)
		return;

	//gi.dprintf("angles = %s\n", vtos(ent->s.angles));

	dropped = G_Spawn();
	
	if (!dropped)
		return;

	dropped->s.modelindex = gi.modelindex("models/weapons/grenade_shell/tris.md2");
		
	dropped->movetype = MOVETYPE_BOUNCE;
	
	dropped->mins[0] = -4;
	dropped->mins[1] = -4;
	dropped->mins[2] = -2;
	
	dropped->maxs[0] =  4;
	dropped->maxs[1] =  4;
	dropped->maxs[2] =  4;

	AngleVectors (ent->client->v_angle, forward, right, up);

	dropped->s.angles[YAW] = ent->client->v_angle[YAW];

	VectorSet(offset, 0, 16, ent->viewheight -2 );
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, dropped->s.origin);

	VectorScale (forward, 5 + ((rand()&15)-8), dropped->velocity);
    VectorMA (dropped->velocity, 50 + (rand()&31), right, dropped->velocity);
	dropped->velocity[2] = 150 + (rand()&31);

	// Ridah, changed this so they don't stick in the ground so much
	VectorSet(dropped->avelocity, 0/*((rand()&7)-4)*/, ((rand()&15)-8), ((rand()&7)-4));

	dropped->think = Think_Eject_Shell;
	dropped->misstime = 40;
	dropped->nextthink = level.time + 0.1;

	dropped->s.renderfx2 |= RF2_NOSHADOW;

	gi.linkentity (dropped);

	return;
}


void weapon_grenadelauncher_fire (edict_t *ent)
{
	vec3_t	offset;
	vec3_t	forward, right;
	vec3_t	start;
	int		damage = 120;
	float	radius;

	if (ent->waterlevel >= 2)
	{
		// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 33;
		}
		return;
	}

	damage *= 2;		// 240 points of damage
	radius = damage*2;	// 480 units radius
	radius *= 0.75;

	VectorSet(offset, 16, 8, ent->viewheight-8);
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	fire_grenade (ent, start, forward, damage, 750, 2.0, radius);

	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_GRENADE | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value ) )
	{
		ent->client->pers.weapon_clip[ent->client->clip_index]--;
	
		if (!(ent->client->pers.inventory[ent->client->ammo_index]) && !ent->client->pers.weapon_clip [ent->client->clip_index])
		{
			NoAmmoWeaponChange (ent);
			ent->client->ps.gunframe = 33;		
		}
	}
		
	Eject_Grenade_Shell(ent);
}

void Weapon_GrenadeLauncher_Fire (edict_t *ent)
{
	if (ent->client->ps.gunframe == 5)
			weapon_grenadelauncher_fire (ent);
	else if (ent->client->ps.gunframe == 16)
	{
		if ((ent->client->pers.inventory[ent->client->ammo_index]) && !ent->client->pers.weapon_clip [ent->client->clip_index])
		{
			ent->client->reload_weapon = true;
		}
		
		ent->client->ps.gunframe = 33;

	}


}

#define	MAX_GRENADE_ROUNDS	3

void Weapon_GrenadeLauncher (edict_t *ent)
{
	static int	pause_frames[]	= {0, 0};
	static int	fire_frames[]	= {5, 16, 0};
	int			rounds;

	if (ent->client->pers.inventory[ent->client->ammo_index] == 0 && ent->client->pers.weapon_clip[ent->client->clip_index] == 0
		&& ent->client->weaponstate != WEAPON_DROPPING) 
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
	else if (ent->client->reload_weapon)
	{
		if (ent->client->ps.gunframe >= 33 && ent->client->ps.gunframe <= 40)
		{
			if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_GRENADE_ROUNDS)
			{
				ent->client->ps.gunframe = 17;
				ent->client->weaponstate = WEAPON_RELOADING;
				
				ent->client->pers.inventory[ent->client->ammo_index] += ent->client->pers.weapon_clip[ent->client->clip_index];	

				if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_GRENADE_ROUNDS)
				{
					rounds = ent->client->pers.inventory[ent->client->ammo_index];
				}
				else
					rounds = MAX_GRENADE_ROUNDS;

				ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
				ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/grenade_launcher/reload.wav"), 1, ATTN_NORM, 0);
			}

			ent->client->reload_weapon = false;
		}
	}
	else if (!ent->client->pers.weapon_clip[ent->client->clip_index] && ent->client->pers.inventory[ent->client->ammo_index])
	{
		if (ent->client->ps.gunframe >= 33 && ent->client->ps.gunframe <= 40)
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = 17;
			ent->client->weaponstate = WEAPON_RELOADING;

			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_GRENADE_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else 
				rounds = MAX_GRENADE_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/grenade_launcher/reload.wav"), 1, ATTN_NORM, 0);
		}
	}

	Weapon_Generic (ent, 4, 32, 40, 45, pause_frames, fire_frames, Weapon_GrenadeLauncher_Fire);
	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0; //hypov8 unhide

	if (ent->client->ps.gunframe >= 17 && ent->client->ps.gunframe <= 32)
	{
		ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = 0;
	}
	else
		ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = (1<<0 | 1<<1);

	
	if (ent->client->ps.gunframe >= 9 && ent->client->ps.gunframe <= 16)
		ent->client->ps.rdflags |= RDF_NOLERP;
	else
		ent->client->ps.rdflags &= ~RDF_NOLERP;


}

// rocket launcher

void weapon_rocketlauncher_fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius;
	int		radius_damage;
	
	if (ent->waterlevel >= 2)
	{
		// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 30;
		}
		return;
	}

	damage = 100 + (int)(random() * 20.0);
	radius_damage = 120;
	damage_radius = 180;	// 256;
	if (is_quad)
	{
		damage *= 4;
		radius_damage *= 4;
	}
	
	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -4;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_rocket (ent, start, forward, damage, 900, damage_radius, radius_damage);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_ROCKET | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value ) )
	{
		ent->client->pers.weapon_clip[ent->client->clip_index]--;
		
		if (!(ent->client->pers.inventory[ent->client->ammo_index]) && !ent->client->pers.weapon_clip [ent->client->clip_index])
		{
			NoAmmoWeaponChange (ent);
			ent->client->ps.gunframe = 30;		
		}
	}
}

void Weapon_RocketLauncher_Fire (edict_t *ent)
{
	if (ent->client->ps.gunframe == 6)
		weapon_rocketlauncher_fire (ent);
	else if (ent->client->ps.gunframe == 13)
	{
		if ((ent->client->pers.inventory[ent->client->ammo_index]) && !ent->client->pers.weapon_clip [ent->client->clip_index])
		{
			ent->client->reload_weapon = true;
		}
	
		ent->client->ps.gunframe = 30;
	}

}

#define	MAX_ROCKET_ROUNDS	5

void Weapon_RocketLauncher (edict_t *ent)
{
	static int	pause_frames[]	= {0, 0};
	static int	fire_frames[]	= {6, 13, 0};
	int			rounds;

	if (ent->client->pers.inventory[ent->client->ammo_index] == 0 && ent->client->pers.weapon_clip[ent->client->clip_index] == 0
		&& ent->client->weaponstate != WEAPON_DROPPING) 
	{
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
	}
	else if (ent->client->reload_weapon)
	{
		if (ent->client->ps.gunframe >= 30 && ent->client->ps.gunframe <= 41)
		{
			if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_ROCKET_ROUNDS)
			{
				// instant reloading in rocketmode
				if (dm_realmode->value != 2)
				{
					ent->client->ps.gunframe = 14;
					ent->client->weaponstate = WEAPON_RELOADING;
				}
				
				ent->client->pers.inventory[ent->client->ammo_index] += ent->client->pers.weapon_clip[ent->client->clip_index];	

				if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_ROCKET_ROUNDS)
				{
					rounds = ent->client->pers.inventory[ent->client->ammo_index];
				}
				else 
					rounds = MAX_ROCKET_ROUNDS;

				ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
				ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
				if (dm_realmode->value != 2)
					gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/rocket_launcher/reload.wav"), 1, ATTN_NORM, 0);
			}

			ent->client->reload_weapon = false;
		}
	}
	else if (!ent->client->pers.weapon_clip[ent->client->clip_index] && ent->client->pers.inventory[ent->client->ammo_index])
	{
		if (ent->client->ps.gunframe >= 30 && ent->client->ps.gunframe <= 41)
		{
			ent->client->reload_weapon = false;
			if (dm_realmode->value != 2)
			{
				ent->client->ps.gunframe = 14;
				ent->client->weaponstate = WEAPON_RELOADING;
			}

			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_ROCKET_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else 
				rounds = MAX_ROCKET_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			if (dm_realmode->value != 2)
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/rocket_launcher/reload.wav"), 1, ATTN_NORM, 0);
		}
	}
	
	Weapon_Generic (ent, 5, 29, 41, 47, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);
	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0; //hypov8 unhide
	ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = 0; //hypov8 unhide

	//remove shell
	if (ent->client->ps.gunframe >= 14 && ent->client->ps.gunframe <= 29)
	{
		ent->client->ps.model_parts[PART_GUN_MISC].invisible_objects = 0;
	}
	else
		ent->client->ps.model_parts[PART_GUN_MISC].invisible_objects = (1<<0 | 1<<1);

	// gi.dprintf ("frame: %d\n", ent->client->ps.gunframe);
}



//GUNRACE_START
/*
======================================================================
MACHETE

0-4 raise
5-14 attack
15-15 reload
16-42 idle
29-32 lower
======================================================================
*/

#define raiseBJ1 4
#define shootBJ0 5
#define reloadBJ1 15
#define idleBJ1 42
#define lowerBJ1 47

void Machete_Hit(edict_t *ent, vec3_t vorigin, int damage)
{
	vec3_t		start;
	vec3_t		forward, right;
	int			kick = 2;
	vec3_t		offset;

	if (is_quad)
		damage *= 4;
	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorSet(offset, 32, 0, ent->viewheight);

	VectorAdd(offset, vorigin, offset);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	if (fire_blackjack(ent, start, forward, damage, kick, MOD_BLACKJACK))
		ent->client->ps.weapon_usage++;

	if (ent->client->ps.weapon_usage > 20)
	{
		ent->client->ps.rdflags &= ~RDF_SKINPLUS;
		ent->client->ps.rdflags |= RDF_SKINPLUSPLUS;
	}
	else if (ent->client->ps.weapon_usage > 10)
		ent->client->ps.rdflags |= RDF_SKINPLUS;


	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_BLACKJACK_SWING);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
}


void Weapon_Machete_Hit(edict_t *ent)
{
	int		damage = 80;

	Machete_Hit(ent, vec3_origin, damage);
	ent->client->ps.gunframe++;
}

void Weapon_Machete(edict_t *ent)
{
	static int	pause_frames[] = { 0, 0 };
	static int	fire_frames[] = { shootBJ0 + 2, shootBJ0 + 7, 0 };

	if (ent->client->ps.gunframe < raiseBJ1)
	{
		ent->client->ps.rdflags &= ~RDF_SKINPLUS;
		ent->client->ps.rdflags &= ~RDF_SKINPLUSPLUS;
	}

	Weapon_Generic(ent, raiseBJ1, reloadBJ1, idleBJ1, lowerBJ1, pause_frames, fire_frames, Weapon_Machete_Hit);

	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0;

}


// add hypov8
// pistol animation frames
#define PISTOL_RAISE_END 5

#define PISTOL_SHOOT_START 6
#define PISTOL_SHOOT_2ND 10
#define PISTOL_SHOOT_END 13

#define PISTOL_RELOAD_START 14
#define PISTOL_RELOAD_END 32

#define PISTOL_IDLE_START 33
#define PISTOL_IDLE_END 35

#define PISTOL_LOWER_START 36
#define PISTOL_LOWER_END 39

//hypo hmg last shoot frame, fix bullet count issue
//#define lowerHMG1 19

void Weapon_Pistol_Fire_Duel(edict_t *ent)
{
	int		damage;
	int		rounds;

	if (ent->waterlevel >= 2)
	{
		// gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = PISTOL_IDLE_START; // hypo was 29
		}
		return;
	}

	damage = 30; //GUNRACE_ADD

#if 0
	if (ent->client->ps.gunframe == shootGun0 + 4) //add hypov8 to allow only 1 bullet to fire
	{
		if (!(ent->client->buttons & BUTTON_ATTACK))
			ent->client->ps.gunframe = idleGun0;
		else
			ent->client->ps.gunframe++;
	}
	else
#endif
		if (ent->client->ps.gunframe == PISTOL_SHOOT_START || ent->client->ps.gunframe == PISTOL_SHOOT_2ND /*5*/) //hypov8 fire 2 shots like hmg
		{
			if (ent->client->pers.pistol_mods & WEAPON_MOD_ROF)
			{
				//GUNRACE_START
				ent->client->pers.hmg_shots--;
				if (!ent->client->pers.hmg_shots)
				{
					ent->client->pers.pistol_mods &= ~WEAPON_MOD_ROF;
					ent->client->newweapon = ent->client->pers.weapon;
					if (ent->client->pers.inventory[ent->client->ammo_index] || ent->client->pers.weapon_clip[ent->client->clip_index])
						ChangeWeapon(ent);
					else
						NoAmmoWeaponChange(ent);
				}
				//GUNRACE_END

				if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE)
				{
					damage *= 2;
				}
				ent->client->ps.gunframe += 2;
			}
			else
			{
				if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE)
				{
					damage *= 2;
				}
				ent->client->ps.gunframe++;
			}
			Pistol_Fire(ent, vec3_origin, damage);
			Eject_Pistol_Shell(ent);
		}
		else if (ent->client->ps.gunframe == PISTOL_SHOOT_END)
		{
			if (!ent->client->pers.weapon_clip[ent->client->clip_index])
			{

				if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_PISTOL_ROUNDS)
				{
					rounds = ent->client->pers.inventory[ent->client->ammo_index];
				}
				else
					rounds = MAX_PISTOL_ROUNDS;

				ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
				ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
				ent->client->ps.gunframe++;
				if (ent->client->pers.pistol_mods & WEAPON_MOD_RELOAD)
					;//gi.dprintf ("need fast reload sound\n");
				else
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/pistol/clip_out.wav"), 1, ATTN_NORM, 0);

				if (!ent->client->pers.weapon_clip[ent->client->clip_index])
				{
					if (level.time >= ent->pain_debounce_time)
					{
						gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
						ent->pain_debounce_time = level.time + 1;
					}
					NoAmmoWeaponChange(ent);
				}
			}
			else
				ent->client->ps.gunframe = PISTOL_IDLE_START; // hypo was 29
		}

}

void Weapon_Pistol_Duel(edict_t *ent)
{
	int		rounds;
	static int	pause_frames[] = { 0, 0, 0 };
	static int	fire_frames[] = { PISTOL_SHOOT_START, PISTOL_SHOOT_2ND, PISTOL_SHOOT_END, 0 };

	if (ent->client->reload_weapon) //reload pressed
	{
		if (ent->client->ps.gunframe >= PISTOL_RELOAD_END && ent->client->ps.gunframe <= PISTOL_IDLE_END) //finished reload or idle
		{
			if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_PISTOL_ROUNDS) //ammo not full
			{
				ent->client->ps.gunframe = PISTOL_RELOAD_START;
				ent->client->weaponstate = WEAPON_RELOADING;

				ent->client->pers.inventory[ent->client->ammo_index] += ent->client->pers.weapon_clip[ent->client->clip_index];

				if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_PISTOL_ROUNDS)
				{
					rounds = ent->client->pers.inventory[ent->client->ammo_index];
				}
				else
					rounds = MAX_PISTOL_ROUNDS;

				ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
				ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			}

			ent->client->reload_weapon = false; //ammo full
		}
		else
			ent->client->reload_weapon = false; //not ready to reload
	}
	else if (ent->client->pers.weapon_clip[ent->client->clip_index]<= 1 && ent->client->pers.inventory[ent->client->ammo_index]) //gun empty but have ammo
	{
		if (ent->client->ps.gunframe >= PISTOL_RELOAD_END && ent->client->ps.gunframe <= PISTOL_LOWER_START) //finished reload or idle
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = PISTOL_RELOAD_START; // hypo was 10
			ent->client->weaponstate = WEAPON_RELOADING;

			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_PISTOL_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else
				rounds = MAX_PISTOL_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
		}
	}

	if (ent->client->pers.pistol_mods & WEAPON_MOD_DAMAGE){
		ent->client->ps.gunindex = gi.modelindex("models/weapons/v_colt/magnum.mdx");
		ent->client->ps.model_parts[PART_GUN_MISC].modelindex = gi.modelindex("models/weapons/v_colt/magnum_l.mdx"); //shouldent happen!!
	}

	//            *ent, FRAME_ACTIVATE_LAST, FRAME_RELOAD_LAST, FRAME_IDLE_LAST, FRAME_DEACTIVATE_LAST,
	Weapon_Generic(ent, PISTOL_RAISE_END, PISTOL_RELOAD_END, PISTOL_IDLE_END, PISTOL_LOWER_END, pause_frames, fire_frames, Weapon_Pistol_Fire_Duel);
	ent->client->ps.model_parts[PART_GUN_R_HAND].invisible_objects = 0; //hypov8 unhide
	ent->client->ps.model_parts[PART_GUN_MISC].invisible_objects = 0; //hypov8 unhide

	if (ent->client->pers.pistol_mods & WEAPON_MOD_RELOAD)
	{
		if (ent->client->ps.gunframe == PISTOL_RELOAD_START + 11) //skipp a few frames for reload
			ent->client->ps.gunframe = PISTOL_RELOAD_START + 17;
	}


	if (ent->client->ps.gunframe == 1)
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/pistol/holster.wav"), 1, ATTN_NORM, 0);

	if (ent->client->ps.gunframe == PISTOL_RELOAD_START + 1)
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/pistol/clip_out.wav"), 1, ATTN_NORM, 0);

	if (ent->client->ps.gunframe == PISTOL_RELOAD_START + 8)
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/pistol/clip_in.wav"), 1, ATTN_NORM, 0);

	if (ent->client->ps.gunframe >= PISTOL_RELOAD_START && ent->client->ps.gunframe <= (PISTOL_RELOAD_END - 4))
	{
		ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = 0;
	}
	else
		ent->client->ps.model_parts[PART_GUN_CLIP].invisible_objects = (1 << 0 | 1 << 1);

}

void M41a_Fire(edict_t *ent)
{
	int	i;
	vec3_t		start;
	//vec3_t	fireorigin;
	vec3_t		forward, right;
	vec3_t		angles;
	int			damage = 40; //hypov8  
	int			kick = 3; //hypov8
	vec3_t		offset;

	if (ent->waterlevel >= 2)
	{
		// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 30;
		}
		return;
	}

	// First shot
	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		// ent->client->machinegun_shots = 0;
		ent->client->ps.gunframe = 30;
		return;
	}

	if (ent->client->pers.inventory[ent->client->ammo_index] == 0 && ent->client->pers.weapon_clip[ent->client->clip_index] == 0)
		return;
#if 0
	if (ent->client->ps.gunframe == 4)
		ent->client->ps.gunframe = 5; //hypo 3 frames
	else
	{
		ent->client->ps.gunframe++;
		if (ent->client->ps.gunframe == 6)
			ent->client->ps.gunframe = 4;
		return; //do nothing
	}
#else
	ent->client->ps.gunframe++;
	if (ent->client->ps.gunframe == 6)
		ent->client->ps.gunframe = 4; //hypo 3 fram

#endif

	// Tommy is too powerful in real-mode
	if (dm_realmode->value)
		damage /= 2;

	// Bounce
	for (i = 1; i<3; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.22;
		ent->client->kick_angles[i] = crandom() * 0.6;
	}
	ent->client->kick_origin[0] = crandom() * 0.22;
	// ent->client->kick_angles[0] = ent->client->machinegun_shots * -.5;

	// get start / end positions
	VectorAdd(ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);

	VectorSet(offset, 0, 0, ent->viewheight - 1);

	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_bullet(ent, start, forward, damage, kick, DEFAULT_M4BULLET_HSPREAD, DEFAULT_M4BULLET_VSPREAD, MOD_M41A); //hypov8 todo death string

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	// Disable for unlimited ammo
	if (!((int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value))
	{
		ent->client->pers.weapon_clip[ent->client->clip_index]--;

		if (!(ent->client->pers.inventory[ent->client->ammo_index]) && !ent->client->pers.weapon_clip[ent->client->clip_index])
		{
			NoAmmoWeaponChange(ent);
			ent->client->ps.gunframe = 30;
		}
		else if (!ent->client->pers.weapon_clip[ent->client->clip_index])
		{
			ent->client->reload_weapon = true;
			ent->client->ps.gunframe = 30;
			// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/machinegun/machgcock.wav"), 1, ATTN_NORM, 0);
		}
	}

	if (!deathmatch_value)	// uses up bandwidth
		Eject_Tommy_Shell(ent);
}


void Weapon_M41a(edict_t *ent)
{
	int		rounds;
	static int	pause_frames[] = { 44, 0, 0 };
	static int	fire_frames[] = { 4, 5, 6, 0 };

	if (ent->client->pers.inventory[ent->client->ammo_index] == 0 && ent->client->pers.weapon_clip[ent->client->clip_index] == 0
		&& ent->client->weaponstate != WEAPON_DROPPING)
	{
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange(ent);
	}
	else if (ent->client->reload_weapon)
	{
		if (ent->client->ps.gunframe >= 30 && ent->client->ps.gunframe <= 40)
		{
			if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_M41AGUN_ROUNDS)
			{
				ent->client->ps.gunframe = 7; //hypo was 6
				ent->client->weaponstate = WEAPON_RELOADING;

				ent->client->pers.inventory[ent->client->ammo_index] += ent->client->pers.weapon_clip[ent->client->clip_index];

				if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_M41AGUN_ROUNDS)
				{
					rounds = ent->client->pers.inventory[ent->client->ammo_index];
				}
				else
					rounds = MAX_M41AGUN_ROUNDS;

				ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
				ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/machinegun/m41acock.wav"), 1, ATTN_NORM, 0);
			}

			ent->client->reload_weapon = false;
		}
	}
	else if (!ent->client->pers.weapon_clip[ent->client->clip_index] && ent->client->pers.inventory[ent->client->ammo_index])
	{
		if (ent->client->ps.gunframe >= 30 && ent->client->ps.gunframe <= 40)
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = 7; //hypov8 was 6
			ent->client->weaponstate = WEAPON_RELOADING;

			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_M41AGUN_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else
				rounds = MAX_M41AGUN_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/machinegun/m41acock.wav"), 1, ATTN_NORM, 0);
		}
	}

	Weapon_Generic(ent, 3, 29, 40, 46, pause_frames, fire_frames, M41a_Fire);
	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0; //hypov8 unhide
	ent->client->ps.model_parts[PART_GUN_R_HAND].invisible_objects = 0; //hypov8 unhide
}

/*
======================================================================
UZI G()^T

0-4 raise
5-6 attack
7-25 reload
26-28 idle
29-32 lower
======================================================================
*/
//raise
#define raiseUzi_1 4
//shoot
#define shootUzi_0 5
#define shootUzi_1 6
//reload
#define reloadUzi_0 8 //hypov8 7 has some bullets delete?
#define reloadUzi_1 26
//idle
#define idleUzi_0 27
#define idleUzi_1 29
//put weapon away
#define lowerUzi_0 30 //hypov8 ToDo animation
#define lowerUzi_1 34

void Uzi_Fire(edict_t *ent)
{
	int	i;
	vec3_t		start;
	//vec3_t	fireorigin;
	vec3_t		forward, right;
	vec3_t		angles;
	int			damage = 14;
	int			kick = 2;
	vec3_t		offset;

	if (ent->waterlevel >= 2)
	{
		// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = idleUzi_0; //11
		}
		return;
	}

	// First shot
	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe = idleUzi_0; //idle
		return;
	}

	if (ent->client->pers.inventory[ent->client->ammo_index] <= 1 && ent->client->pers.weapon_clip[ent->client->clip_index] <= 1)
		return;

	if (ent->client->ps.gunframe == shootUzi_1)
		ent->client->ps.gunframe = shootUzi_0;
	else
		ent->client->ps.gunframe = shootUzi_1;


	// too powerful in real-mode
	if (dm_realmode->value)
		damage /= 2;

	// Bounce
	for (i = 1; i<3; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.22;
		ent->client->kick_angles[i] = crandom() * 0.6;
	}
	ent->client->kick_origin[0] = crandom() * 0.22;
	// ent->client->kick_angles[0] = ent->client->machinegun_shots * -.5;

	// get start / end positions
	VectorAdd(ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);

	VectorSet(offset, 0, 0, ent->viewheight - 1);

	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	{	//add hypov8. Crouch has better accurecy
		int spreadH = DEFAULT_BULLET_HSPREAD;
		int spreadV = DEFAULT_BULLET_VSPREAD;

		if ((ent->client->ps.pmove.pm_flags & PMF_DUCKED)){
			spreadH = 180;
			spreadV = 280;
		}

		//hypov8 2 bullets
		fire_shotgun(ent, start, forward, damage, kick, spreadH, spreadV, DEFAULT_SSHOTGUN_COUNT_GR, MOD_UZI);
	}

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_CHAINGUN2 | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	// Disable for unlimited ammo
	if (!((int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value))
	{
		ent->client->pers.weapon_clip[ent->client->clip_index]--;
		ent->client->pers.weapon_clip[ent->client->clip_index]--; //hypov8 2 bullets

		if (ent->client->pers.inventory[ent->client->ammo_index] <= 1 && ent->client->pers.weapon_clip[ent->client->clip_index] <= 1)
		{
			NoAmmoWeaponChange(ent);
			ent->client->ps.gunframe = idleUzi_0;
		}
		else if (ent->client->pers.weapon_clip[ent->client->clip_index] <= 1)
		{
			ent->client->reload_weapon = true;
			ent->client->ps.gunframe = idleUzi_0;
		}
	}
}

void Weapon_Uzi(edict_t *ent)
{
	int		rounds;
	static int	pause_frames[] = { 0, 0, 0 };//44
	static int	fire_frames[] = { shootUzi_0, shootUzi_1, 0 };//4 en 5

	if (ent->client->pers.inventory[ent->client->ammo_index] <= 1 && ent->client->pers.weapon_clip[ent->client->clip_index] <= 1
		&& ent->client->weaponstate != WEAPON_DROPPING)
	{
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange(ent);
	}
	else if (ent->client->reload_weapon)
	{
		if (ent->client->ps.gunframe >= idleUzi_0 && ent->client->ps.gunframe <= idleUzi_1)
		{
			if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_UZI_ROUNDS)
			{
				ent->client->ps.gunframe = reloadUzi_0;//6
				ent->client->weaponstate = WEAPON_RELOADING;

				ent->client->pers.inventory[ent->client->ammo_index] += ent->client->pers.weapon_clip[ent->client->clip_index];

				if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_UZI_ROUNDS)
				{
					rounds = ent->client->pers.inventory[ent->client->ammo_index];
				}
				else
					rounds = MAX_UZI_ROUNDS;

				ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
				ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/machinegun/machgcock.wav"), 1, ATTN_NORM, 0);
			}

			ent->client->reload_weapon = false;
		}
	}
	else if (ent->client->pers.weapon_clip[ent->client->clip_index] <= 1 /*&& ent->client->pers.inventory[ent->client->ammo_index] >= 2*/)
	{
		//uzi reload
		if (ent->client->ps.gunframe >= idleUzi_0 && ent->client->ps.gunframe <= idleUzi_1) //40
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = reloadUzi_0;
			ent->client->weaponstate = WEAPON_RELOADING;

			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_UZI_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else
				rounds = MAX_TOMMYGUN_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			//gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/machinegun/machgcock.wav"), 1, ATTN_NORM, 0);		
		}
	}

	Weapon_Generic(ent, raiseUzi_1, reloadUzi_1, idleUzi_1, lowerUzi_1, pause_frames, fire_frames, Uzi_Fire);

	if (ent->client->ps.gunframe == 11)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/pistol/clip_out.wav"), 1, ATTN_NORM, 0);

	if (ent->client->ps.gunframe == 18)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/pistol/clip_in.wav"), 1, ATTN_NORM, 0);

}


//G()^T SuperShotGun

/* hypov8 v_shotg2.mdx
0-4 raise
5-8 attack
9-13 cock
14-16 reload
17-55 idle
56-60 lower
*/
#define raiseSSGun1 4
//shoot
#define shootSSGun0 5 //6
#define shootSSGun1 8
//pump action
#define cockSSGun0 9
#define cockSSGun1 13
//reload
#define reloadSSGun0 14
#define reloadSSGun1 16
//idle
#define idleSSGun0 17
#define idleSSGun1 55
//put weapon away
#define lowerSSGun0 56
#define lowerSSGun1 60

//G()^T Supershotgun
void supershotgun_fire(edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage = 80; // goat 8
	int			kick = 70; //goat 50

	if (ent->waterlevel >= 2)
	{
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = idleSSGun0; //33
		}
		return;
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorSet(offset, 0, 0, ent->viewheight - 2);

	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_shotgun(ent, start, forward, damage, kick, 200, 200, DEFAULT_SSHOTGUN_COUNT_GR, MOD_SSHOTGUN);

	VectorScale(forward, -2, ent->client->kick_origin);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_SSHOTGUN | is_silenced); //hypov8 shoot and reload sounds. internal set.. MZ_SHOTGUN 
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	if (!((int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value))
	{
		ent->client->pers.weapon_clip[ent->client->clip_index]--;
		ent->client->pers.weapon_clip[ent->client->clip_index]--; //hypov8 take 2
	}

	Eject_Shotgun_Shell(ent);
}

//G()^T Supershotgun
void Weapon_SuperShotgun_Fire(edict_t *ent)
{
	if (ent->client->ps.gunframe == shootSSGun0) //6
	{
		if (ent->client->pers.weapon_clip[ent->client->clip_index] >= 2) //hypov8 2 shots
			supershotgun_fire(ent);
		else
		{
			ent->client->ps.gunframe = reloadSSGun0; //17
			ent->client->reload_weapon = true;
			ent->client->weaponstate = WEAPON_RELOADING; //WEAPON_RELOAD_CYCLE
		}
	}
	else if (ent->client->ps.gunframe == cockSSGun1) //16
	{
		if (ent->client->pers.weapon_clip[ent->client->clip_index] <2) //hypov8 2 shots
		{
			if (ent->client->pers.inventory[ent->client->ammo_index] >= 2) //hypov8 2 shots
			{
				ent->client->ps.gunframe++;
				ent->client->reload_weapon = true;
				ent->client->weaponstate = WEAPON_RELOADING;//WEAPON_RELOAD_CYCLE
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange(ent);
			}
		}
		else
		{
			ent->client->ps.gunframe = idleSSGun0;
		}
	}
}

void Weapon_SuperShotgun(edict_t *ent)
{
	static int	pause_frames[] = { 0, 0 };
	static int	fire_frames[] = { shootSSGun0, cockSSGun1, 0 }; //6 16 0
	static int  kick_frames[] = { -12, -9, -6, -4, -1 };

	if ((ent->client->ps.gunframe >= shootSSGun0 + 1 /*7*/) & (ent->client->ps.gunframe <= cockSSGun0 /*9*/))
	{
		ent->client->kick_angles[0] = kick_frames[ent->client->ps.gunframe - 6]; //hypov8 what?
	}

	if (ent->client->ps.gunframe == shootSSGun0 + 1)  // 7
		ent->client->ps.rdflags |= RDF_NOLERP;
	else
		ent->client->ps.rdflags &= ~RDF_NOLERP;

	if (ent->client->reload_weapon)
	{
		if (ent->client->buttons & BUTTON_ATTACK && ent->client->pers.weapon_clip[ent->client->clip_index] >= 2 && ent->client->weaponstate != WEAPON_FIRING)
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = idleSSGun0; //33
			ent->client->weaponstate = WEAPON_READY;
		}
		else if (ent->client->newweapon)
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = idleSSGun0; //33
			ent->client->weaponstate = WEAPON_DROPPING;
		}
		else
		{
			if (ent->client->ps.gunframe == reloadSSGun1) //29
			{
				if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_SSHOTGUN_ROUNDS)
				{
					if (ent->client->pers.inventory[ent->client->ammo_index])
					{
						ent->client->pers.inventory[ent->client->ammo_index]--;
						ent->client->pers.weapon_clip[ent->client->clip_index]++;

						if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_SSHOTGUN_ROUNDS)
							ent->client->ps.gunframe = reloadSSGun0; //21
						else
						{
							ent->client->reload_weapon = false;
							ent->client->ps.gunframe = idleSSGun1; //29
							ent->client->weaponstate = WEAPON_READY; //WEAPON_RELOADING
						}
					}
					else
					{
						ent->client->reload_weapon = false;
						ent->client->weaponstate = WEAPON_RELOADING;
					}
				}
				else
				{
					ent->client->reload_weapon = false;
					ent->client->weaponstate = WEAPON_RELOADING;
				}
			}
			else if (ent->client->ps.gunframe >= idleSSGun0 /*33*/ && ent->client->ps.gunframe <= idleSSGun1 /*40*/) //idle
			{
				if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_SSHOTGUN_ROUNDS &&
					ent->client->pers.inventory[ent->client->ammo_index])
				{
					ent->client->weaponstate = WEAPON_RELOADING;//WEAPON_RELOAD_CYCLE
					ent->client->ps.gunframe = reloadSSGun0; //17
				}
			}
		}
	}
	else if (ent->client->pers.weapon_clip[ent->client->clip_index] < 2 && ent->client->weaponstate != WEAPON_DROPPING
		&& !ent->client->buttons & BUTTON_ATTACK)
	{
		ent->client->reload_weapon = true;
		ent->client->weaponstate = WEAPON_RELOADING;//WEAPON_RELOAD_CYCLE
		ent->client->ps.gunframe = reloadSSGun0; //17

		if (!ent->client->pers.inventory[ent->client->ammo_index])
		{
			if (level.time >= ent->pain_debounce_time)
			{
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent->pain_debounce_time = level.time + 1;
			}
			NoAmmoWeaponChange(ent);
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = idleSSGun0; //33
			ent->client->weaponstate = WEAPON_READY; //BUSY
		}
	}

	if (ent->client->ps.gunframe == reloadSSGun0) //21
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/shotgun/shotgload.wav"), 1, ATTN_NORM, 0);
	}

	Weapon_Generic(ent, raiseSSGun1, reloadSSGun1, idleSSGun1, lowerSSGun1, pause_frames, fire_frames, Weapon_SuperShotgun_Fire);

}
//END OF G()^T SuperShotgun

/*
===============
M60
===============
/* hypov8 v_m60.mdx
0-4 raise
5-8 attack
9-41 reload
42-44 idle
45-50 lower
*/
#define raiseM60_1 4
//shoot
#define shootM60_0 5 //shoot start
#define shootM60_1 6 //shoot mid
#define shootM60_2 7 //shoot mid
#define shootM60_3 8 //shoot end
//reload
#define reloadM60_0 9 //hypov8 7 has some bullets delete?
#define reloadM60_1 41 //30
//idle
#define idleM60_0 42 //31
#define idleM60_1 44 //45
//put weapon away
#define lowerM60_0 45 //hypov8 ToDo animation
#define lowerM60_1 50


void Weapon_M60_fire(edict_t *ent)
{
	int	i;
	vec3_t		start;
	//vec3_t	fireorigin;
	vec3_t		forward, right;
	vec3_t		angles;
	int			damage = 80; //hypov8  
	int			kick = 16; //hypov8
	vec3_t		offset;

	if (ent->waterlevel >= 2)
	{
		// gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);	
		if (ent->client->weaponstate == WEAPON_FIRING)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = idleM60_0;
		}
		return;
	}

	// First shot
	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		// ent->client->machinegun_shots = 0;
		ent->client->ps.gunframe = idleM60_0;
		return;
	}

	if (ent->client->pers.inventory[ent->client->ammo_index] == 0 && ent->client->pers.weapon_clip[ent->client->clip_index] == 0)
		return;

	if (ent->client->ps.gunframe == shootM60_3)
		ent->client->ps.gunframe = shootM60_0; //shoot
	else
		ent->client->ps.gunframe++;
		//ent->client->ps.gunframe = shootM60_1;


	// Bounce
	for (i = 1; i<3; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.22;
		ent->client->kick_angles[i] = crandom() * 0.6;
	}
	ent->client->kick_origin[0] = crandom() * 0.22;
	// ent->client->kick_angles[0] = ent->client->machinegun_shots * -.5;

	// get start / end positions
	VectorAdd(ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);

	VectorSet(offset, 0, 0, ent->viewheight - 1);

	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	{	//add hypov8. Crouch has better accurecy
		int spreadH = DEFAULT_M60_HSPREAD;
		int spreadV = DEFAULT_M60_VSPREAD;

		if ((ent->client->ps.pmove.pm_flags & PMF_DUCKED)){
			spreadH = DEFAULT_M60_HSPREAD_CROUCH;
			spreadV = DEFAULT_M60_VSPREAD_CROUCH;
		}


		fire_bullet(ent, start, forward, damage, kick, spreadH, spreadV, MOD_M60);
	}

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_BLASTER	 /*MZ_HYPERBLASTER*/ | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	// Disable for unlimited ammo
	if (!((int)dmflags->value & DF_INFINITE_AMMO && deathmatch_value))
	{
		ent->client->pers.weapon_clip[ent->client->clip_index]--;

		if (!(ent->client->pers.inventory[ent->client->ammo_index]) && !ent->client->pers.weapon_clip[ent->client->clip_index])
		{
			NoAmmoWeaponChange(ent);
			ent->client->ps.gunframe = idleM60_0;
		}
		else if (!ent->client->pers.weapon_clip[ent->client->clip_index])
		{
			ent->client->reload_weapon = true;
			ent->client->ps.gunframe = idleM60_0;
		}
	}

}

void Weapon_M60(edict_t *ent)
{
	static int	pause_frames[] = { 0, 0 };
	static int	fire_frames[] = { shootM60_0, shootM60_1, shootM60_2, shootM60_3, 0 };
	int			rounds;

	if (ent->client->pers.inventory[ent->client->ammo_index] == 0 && ent->client->pers.weapon_clip[ent->client->clip_index] == 0
		&& ent->client->weaponstate != WEAPON_DROPPING)
	{
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange(ent);
	}
	else if (ent->client->reload_weapon)
	{
		if (ent->client->ps.gunframe >= idleM60_0  && ent->client->ps.gunframe <= idleM60_1)
		{
			if (ent->client->pers.weapon_clip[ent->client->clip_index] < MAX_M60_ROUNDS)
			{
				ent->client->ps.gunframe = reloadM60_0;
				ent->client->weaponstate = WEAPON_RELOADING;

				ent->client->pers.inventory[ent->client->ammo_index] += ent->client->pers.weapon_clip[ent->client->clip_index];

				if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_M60_ROUNDS)
				{
					rounds = ent->client->pers.inventory[ent->client->ammo_index];
				}
				else
					rounds = MAX_M60_ROUNDS;

				ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
				ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/m60/cock.wav"), 1, ATTN_NORM, 0);
			}

			ent->client->reload_weapon = false;
		}
	}
	else if (!ent->client->pers.weapon_clip[ent->client->clip_index] && ent->client->pers.inventory[ent->client->ammo_index])
	{
		if (ent->client->ps.gunframe >= idleM60_0  && ent->client->ps.gunframe <= idleM60_1)
		{
			ent->client->reload_weapon = false;
			ent->client->ps.gunframe = reloadM60_0;
			ent->client->weaponstate = WEAPON_RELOADING;

			if (ent->client->pers.inventory[ent->client->ammo_index] < MAX_M60_ROUNDS)
			{
				rounds = ent->client->pers.inventory[ent->client->ammo_index];
			}
			else
				rounds = MAX_M60_ROUNDS;

			ent->client->pers.weapon_clip[ent->client->clip_index] = rounds;
			ent->client->pers.inventory[ent->client->ammo_index] -= rounds;
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/m60/cock.wav"), 1, ATTN_NORM, 0);
		}
	}

	Weapon_Generic(ent, raiseM60_1, reloadM60_1, idleM60_1, lowerM60_1, pause_frames, fire_frames, Weapon_M60_fire);
	ent->client->ps.model_parts[PART_GUN_L_HAND].invisible_objects = 0; //hypov8 unhide
	ent->client->ps.model_parts[PART_GUN_FINGERS].invisible_objects = 0; //hypov8 unhide
}


//hypov8 todo add new weps
//GUNRACE_END