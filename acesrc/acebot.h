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
//  acebot.h - Main header file for ACEBOT
// 
// 
///////////////////////////////////////////////////////////////////////

#ifndef _ACEBOT_H
#define _ACEBOT_H

#define BOT_JUMP_VEL (200*2) //340
#define BOT_FORWARD_VEL (160*2) //340 //hypov8 kp default
#define BOT_SIDE_VEL (160*2) //cl_anglespeedkey->value)	//hypov8 kp default 1.5

// Only 100 allowed for now (probably never be enough edicts for 'em
#define MAX_BOTS 100

// Platform states
#define	STATE_TOP			0
#define	STATE_BOTTOM		1
#define STATE_UP			2
#define STATE_DOWN			3

// Maximum nodes
#define MAX_BOTNODES 1000

// Link types
#define INVALID -1

// Node types
#define BOTNODE_MOVE 0
#define BOTNODE_LADDER 1
#define BOTNODE_PLATFORM 2
#define BOTNODE_TELEPORTER 3
#define BOTNODE_ITEM 4
#define BOTNODE_WATER 5
#define BOTNODE_GRAPPLE 6
#define BOTNODE_JUMP 7
#define BOTNODE_TRIGPUSH 8 //hypov8 add for trigger_push todo:
#define BOTNODE_ALL 99 // For selecting all nodes

// Density setting for nodes
#define BOTNODE_DENSITY			128
#define BOTNODE_DENSITY_HALVE	64
#define BOTNODE_DENSITY_THIRD	42
#define BOTNODE_DENSITY_STAIR	160

// Bot state types
#define BOTSTATE_STAND 0
#define BOTSTATE_MOVE 1
#define BOTSTATE_ATTACK 2
#define BOTSTATE_WANDER 3
#define BOTSTATE_FLEE 4

#define MOVE_LEFT 0
#define MOVE_RIGHT 1
#define MOVE_FORWARD 2
#define MOVE_BACK 3

// KingPin Item defines 
#define ITEMLIST_NULL				0
	
#define ITEMLIST_ARMORHELMET		1
#define ITEMLIST_ARMORJACKET		2
#define ITEMLIST_ARMORLEGS			3
#define ITEMLIST_ARMORHELMETHEAVY	4
#define ITEMLIST_ARMORJACKETHEAVY	5
#define ITEMLIST_ARMORLEGSHEAVY		6

#define ITEMLIST_BLACKJACK          7
#define ITEMLIST_CROWBAR			8
#define ITEMLIST_PISTOL				9
#define ITEMLIST_SPISTOL			10
#define ITEMLIST_SHOTGUN			11
#define ITEMLIST_TOMMYGUN			12
#define ITEMLIST_HEAVYMACHINEGUN	13
#define ITEMLIST_GRENADELAUNCHER	14
#define ITEMLIST_BAZOOKA			15
#define ITEMLIST_FLAMETHROWER		16
#define ITEMLIST_SHOTGUN_E			17
#define ITEMLIST_HEAVYMACHINEGUN_E	18
#define ITEMLIST_BAZOOKA_E			19
#define ITEMLIST_FLAMETHROWER_E		20
#define ITEMLIST_GRENADELAUNCHER_E	21
#define ITEMLIST_PISTOL_E			22
#define ITEMLIST_TOMMYGUN_E			23

#define ITEMLIST_GRENADES			24

#define ITEMLIST_SHELLS				25
#define ITEMLIST_BULLETS			26
#define ITEMLIST_ROCKETS			27
#define ITEMLIST_AMMO308			28
#define ITEMLIST_CYLINDER			29
#define ITEMLIST_FLAMETANK			30

#define ITEMLIST_COIL				31
#define ITEMLIST_LIZZYHEAD			32
#define ITEMLIST_CASHROLL			33
#define ITEMLIST_CASHBAGLARGE		34
#define ITEMLIST_CASHBAGSMALL		35
#define ITEMLIST_BATTERY			36
#define ITEMLIST_JETPACK			37

#define ITEMLIST_HEALTH_SMALL		38
#define ITEMLIST_HEALTH_LARGE		39
#define ITEMLIST_FLASHLIGHT			40
#define ITEMLIST_WATCH				41
#define ITEMLIST_WHISKEY			42
#define ITEMLIST_PACK				43
#define ITEMLIST_ADRENALINE			44
#define ITEMLIST_KEYFUSE			45
#define ITEMLIST_SAFEDOCS			46
#define ITEMLIST_VALVE				47
#define ITEMLIST_OILCAN				48
#define ITEMLIST_KEY1				49
#define ITEMLIST_KEY2				50
#define ITEMLIST_KEY3				51
#define ITEMLIST_KEY4				52
#define ITEMLIST_KEY5				53
#define ITEMLIST_KEY6				54
#define ITEMLIST_KEY7				55
#define ITEMLIST_KEY8				56
#define ITEMLIST_KEY9				57
#define ITEMLIST_KEY10				58

#define ITEMLIST_PISTOLMODS			59
#define ITEMLIST_PISTOLMODS2		60
#define ITEMLIST_PISTOLMODS3		61
#define ITEMLIST_PISTOLMODS4		62
#define ITEMLIST_HMGMODS			63
#define ITEMLIST_TELEPORTER			54

#define ITEMLIST_BOT				65
#define ITEMLIST_PLAYER				66

typedef struct gitem_s gitem_t; //needed for ->acebot.

// Node structure
typedef struct botnode_s
{
	vec3_t origin; // Using Id's representation
	int type;   // type of node

} botnode_t;

typedef struct item_table_s
{
	int item;
	float weight;
	edict_t *ent;
	int node;

} item_table_t;

#if 1
typedef struct //bot->acebot.is_bot
{
	qboolean is_bot;
	qboolean is_jumping;

	// For movement
	vec3_t move_vector;
	float next_move_time;
	float wander_timeout;
	float suicide_timeout;
	vec3_t botOldOrigin; //add hypov8
	float botMoveDirVel; //add hypov8 velocity compare
	vec3_t botDeathAngles; //hypov8 store angles for dead body
	int bot_isPushed; //add hypov8 trig push. dont move
	int bot_targetPlayerNode; //add hypov8. dont check if item exists
	int	lastDamageTimer; //last time bot took damage. make bot attack quicker
	int	bot_last_strafeTime; //frame since strafed. make strafe go for longer
	int	bot_last_strafeDir;

	//hypov8 aim recalculate on shoot
	vec3_t bot_enemyOrigin; //store enamy origin untill we shoot with filre_lead
	float bot_accuracy; //store accuracy untill we shoot with filre_lead

	//auto route
	int bot_pm_playerJumpTime; //add hypov8 store last jump time for auto rout
	int bot_pm_jumpPadMove; //add hypov8. connect nodes if in air

	// For node code
	int current_node; // current node
	int goal_node; // current goal node
	int next_node; // the node that will take us one step closer to our goal
	int node_timeout;

	//PM (Player Movement)
	int pm_last_node;		//used in auto rout
	int tries;

	int state;

	//hypo new bot skill func
	int			new_target;			//if new target. dont shoot straight away
	int			old_target;			//old player target. shoot if more than xx seconds
	int			old_targetTimer;	//dont keep old targets in memory for to long? will ignore skill on 2nd sight
	float		botNewTargetTime;	//timer to allow bot to start attacking
	int			lastDamagePlayerIndex; //store last attacker num

} acebot_t;
#endif

//extern struct edict_s;
extern int num_players;
extern edict_t *players[MAX_CLIENTS];		// pointers to all players in the game

//////////////
// extern decs
extern botnode_t nodes[MAX_BOTNODES]; 
extern item_table_t item_table[MAX_EDICTS];
extern qboolean debug_mode;
extern qboolean debug_mode_origin_ents; //add hypov8
extern int numnodes;
extern int num_items;

////////////////////////////
// id Function Protos I need
void     LookAtKiller (edict_t *self, edict_t *inflictor, edict_t *attacker);
void     ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker);
void     TossClientWeapon (edict_t *self);
void     ClientThink (edict_t *ent, usercmd_t *ucmd);
void     SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles);
void     ClientUserinfoChanged (edict_t *ent, char *userinfo);
void     CopyToBodyQue (edict_t *ent);
//qboolean ClientConnect (edict_t *ent, char *userinfo);
void     Use_Plat (edict_t *ent, edict_t *other, edict_t *activator);

//////////////
// acebot_ai.c protos
void     ACEAI_Think (edict_t *self);
void     ACEAI_PickLongRangeGoal(edict_t *self);
void     ACEAI_PickShortRangeGoal(edict_t *self);
qboolean ACEAI_FindEnemy(edict_t *self);
void     ACEAI_ChooseWeapon(edict_t *self);

////////////////
// acebot_cmds.c protos
qboolean ACECM_Commands(edict_t *ent);
void	 ACECM_LevelBegin(void); //hypov8 add
void     ACECM_LevelEnd(void);
void	 ACECM_BotDebug(qboolean changeState); //add hypov8
void	 ACECM_BotAdd(char *name, char *skin, char *team); //add hypov8. should be moved to ace spawn
int		 ACECM_ReturnBotSkillWeb(void); //add hypov8

/////////////////
// acebot_items.c protos
void     ACEIT_PlayerAdded(edict_t *ent);
void     ACEIT_PlayerRemoved(edict_t *ent);
qboolean ACEIT_IsVisible(edict_t *self, vec3_t goal);
qboolean ACEIT_IsReachable(edict_t *self,vec3_t goal);
qboolean ACEIT_ChangeWeapon(edict_t *ent, gitem_t *item);
qboolean ACEIT_CanUseArmor (gitem_t *item, edict_t *other);
float	 ACEIT_ItemNeed(edict_t *self, int item);
int		 ACEIT_ClassnameToIndex(char *classname);
void     ACEIT_BuildItemNodeTable (qboolean rebuild);
qboolean ACEIT_CheckIfItemExists(edict_t *self); //add hypov8

////////////////////
// acebot_movement.c protos
qboolean ACEMV_SpecialMove(edict_t *self,usercmd_t *ucmd);
void     ACEMV_Move(edict_t *self, usercmd_t *ucmd);
void     ACEMV_Attack (edict_t *self, usercmd_t *ucmd);
void     ACEMV_Wander (edict_t *self, usercmd_t *ucmd);
void	 ACEMV_JumpPadUpdate(edict_t *bot); //add hypov8
void	 ACEMV_Attack_CalcRandDir(edict_t *self, vec3_t aimdir); //aim at enamy but shoot off target

/////////////////
// acebot_nodes.c protos
int      ACEND_FindCost(short from, short to);
int      ACEND_FindClosestReachableNode(edict_t *self, int range, int type);
void     ACEND_SetGoal(edict_t *self, int goal_node);
qboolean ACEND_FollowPath(edict_t *self);
void     ACEND_PathMap(edict_t *self);
void     ACEND_InitNodes(void);
void     ACEND_ShowNode(int node, int isTmpNode);
void     ACEND_DrawPath();
void     ACEND_ShowPath(edict_t *self, int goal_node);
int      ACEND_AddNode(edict_t *self, int type);
void     ACEND_UpdateNodeEdge(int from, int to, qboolean stopJumpNodes, qboolean stopTeleNodes, qboolean checkSight, qboolean isTrigPush);
void     ACEND_RemoveNodeEdge(edict_t *self, int from, int to);
void     ACEND_ResolveAllPaths();
void     ACEND_SaveNodes();
void     ACEND_LoadNodes();
void	 ACEND_DebugNodesLocal(void); //add hypov8
void	 ACEND_PathToTeleporter(edict_t *player); //add hypov8
void	 ACEND_PathToTeleporterDest(edict_t *player); //add hypov8
void	 ACEND_PathToTrigPush(edict_t *player); //add hypov8
void	 ACEND_PathToTrigPushDest(edict_t *player); //add hypov8
short	 ACEND_FindClosestNode(edict_t *self, int range, short type); //add hypov8

/////////////////
// acebot_spawn.c protos
void	 ACESP_SaveBots();
void	 ACESP_LoadBots();
void     ACESP_HoldSpawn(edict_t *self);
void     ACESP_PutClientInServer (edict_t *bot, int team);
void     ACESP_Respawn (edict_t *self);
edict_t *ACESP_FindFreeClient (void);
void     ACESP_SetName(edict_t *bot, char *name, char *skin);
void     ACESP_SpawnBot (char *name, char *skin, char *team, char *userinfo);
void     ACESP_ReAddBots();
void     ACESP_RemoveBot(char *name, qboolean saveBotFile);
void	 safe_cprintf (edict_t *ent, int printlevel, char *fmt, ...);
void     safe_centerprintf (edict_t *ent, char *fmt, ...);
void     safe_bprintf (int printlevel, char *fmt, ...);
void     safe_ConsolePrintf(int printlevel, char *fmt, ...); //add hypov8
void     debug_printf (char *fmt, ...);
void	 ACECM_LevelEnd(void);

void	ACESP_FreeBots(void); //fix CNCT issue

//GUNRACE_ADD
///////////
//gunrace.c
void	gr_ResetPlayerBeginDM(gclient_t *client); //add hypov8
//GUNRACE_END

#endif