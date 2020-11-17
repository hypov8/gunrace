//////////////////////////////////////
// defines/settings for gunrace		//
//									//
//////////////////////////////////////
//max wep count
#define	GR_WEPS	13 //match grWeps_t grWeps //hypo todo: is also in g_weapon.c
//hypov8 todo add new weps

typedef struct grWeps_s
{
	char		*grWepName;		// weapon name  
	char		grWepNameMenu[8];	// weapon name in Scoreboard 
	char		*grWepNameHUD;	// weapon name in hud 
	char		*grAmmoName;	// weapon ammo                    
	int         grAmmo;		// Fill up ammo      
} grWeps_t;

grWeps_t grWeps[GR_WEPS + 1];
int		killcount[GR_WEPS];

void	gr_CheckWepState(void);//add hypov8
void	gr_RespawnSetWeps(gclient_t * client); //add hypov8
void	gr_ResetPlayerBeginDM(gclient_t *client); //add hypov8
void	gr_SetKillsToNext(gclient_t *client); //add hypov8
void	gr_ScoreBoard(void); //add hypov8
void	gr_setScoreboardGunrace(gclient_t *client); //add hypov8
void	SetupGunrace(void); //add hypov8

void EndDMLevel(void); //add hypov8

void check_version(edict_t *ent); //add hypov8
qboolean gr_isLastWep(gclient_t * client); //add hypov8
