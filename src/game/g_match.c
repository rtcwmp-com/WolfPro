#include "g_local.h"

char *aTeams[TEAM_NUM_TEAMS] = { "FFA", "^1Axis^7", "^4Allies^7", "Spectators" };
team_info teamInfo[TEAM_NUM_TEAMS];

void G_loadMatchGame(void) {
	int  randomValues[MAX_REINFSEEDS];
	char reinfSeeds[MAX_STRING_CHARS];

	// Set up the random reinforcement seeds for both teams and send to clients
	int blueOffset = rand() % MAX_REINFSEEDS;
	int redOffset  = rand() % MAX_REINFSEEDS;
	Q_strncpyz(reinfSeeds, va("%d %d", (blueOffset << REINF_BLUEDELT) + (rand() % (1 << REINF_BLUEDELT)),
	                             (redOffset << REINF_REDDELT)  + (rand() % (1 << REINF_REDDELT))),
	           MAX_STRING_CHARS);

	for (int i = 0; i < MAX_REINFSEEDS; i++){
		randomValues[i] = (rand() % g_spawnOffset.integer) * BG_ReinfSeeds[i];
		Q_strcat(reinfSeeds, MAX_STRING_CHARS, va(" %d", randomValues[i]));
	}

	level.blueReinfOffset = 1000 * randomValues[blueOffset] / BG_ReinfSeeds[blueOffset];
	level.redReinfOffset  = 1000 * randomValues[redOffset] / BG_ReinfSeeds[redOffset];

	trap_SetConfigstring(CS_REINFSEEDS, reinfSeeds);
    // write first respawn time
    // if (g_gameStatslog.integer && g_gamestate.integer == GS_PLAYING) {
    //     gentity_t *dummy = g_entities;

    //     G_writeGeneralEvent(dummy,dummy,"",teamFirstSpawn);
    // }
}

/*
=================
Match Info

Basically just some info prints..
=================
*/
// Gracefully taken from s4ndmod :p
char* GetLevelTime(void) {
	int Objseconds, Objmins, Objtens;

	Objseconds = (((g_timelimit.value * 60 * 1000) - ((level.time - level.startTime))) / 1000); // martin - this line was a bitch :-)
																								// nate	  - I know, that's why I took it. :p
	Objmins = Objseconds / 60;
	Objseconds -= Objmins * 60;
	Objtens = Objseconds / 10;
	Objseconds -= Objtens * 10;

	if (Objseconds < 0) { Objseconds = 0; }
	if (Objtens < 0) { Objtens = 0; }
	if (Objmins < 0) { Objmins = 0; }

	return va("%i:%i%i", Objmins, Objtens, Objseconds);
}

// Prints stuff
void G_matchPrintInfo(char *msg, qboolean printTime) {
	if (printTime)
		AP(va("print \"[%s] ^3%s \n\"", GetLevelTime(), msg));
	else
		AP(va("print \"*** ^3INFO: ^7%s \n\"", msg));
}

/*
=================
Countdown

Causes some troubles on client side so done it here.
=================
*/
void CountDown(void) {

	if (level.cnStarted == qfalse) {
		return;
	}

	// Prepare to fight takes 2 seconds..
	if(level.cnNum == 0) {
		level.cnPush = level.time + 2000;
	// Just enough to fix the bug and skip to action..
	} else if (level.cnNum == 6) {
		level.cnPush = level.time + 200;
	// Otherwise, 1 second.
	} else {
		level.cnPush = level.time + 1000;  
	} 
	
	// We're done.. restart the game
	if (level.cnNum == 7) {
		level.warmupTime += 10000;
		trap_Cvar_Set( "g_restarted", "1" );
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		level.restarted = qtrue;			
		
		return;
	}
		
	level.cnNum++; 
}

/*
=================
G_delayPrint

Deals with pause related functionality
=================
*/
void G_delayPrint(gentity_t *dpent) {
	int think_next = 0;
	qboolean fFree = qtrue;

	switch (dpent->spawnflags){
	case DP_PAUSEINFO:
		if (level.paused > PAUSE_UNPAUSING) {
			int cSeconds = match_timeoutlength.integer * 1000 - (level.time - dpent->timestamp);

			if (cSeconds > 1000) {
				think_next = level.time + 1000;
				fFree = qfalse;

				if (cSeconds > 30000) {
					AP(va("popin \"Timeouts Available: [^1Axis^7] %d - [^4Allies^7] %d\n\"y",
						teamInfo[TEAM_RED].timeouts, teamInfo[TEAM_BLUE].timeouts));
				}
			}
			else {
				level.paused = PAUSE_UNPAUSING;
				G_spawnPrintf(DP_UNPAUSING, level.time + 7.2, NULL);
			}
		}
		break;
	case DP_UNPAUSING:
		if (level.paused == PAUSE_UNPAUSING) {
			int cSeconds = 10 * 1000 - (level.time - dpent->timestamp);

			if (cSeconds > 1000) {
				think_next = level.time + 1000;
				fFree      = qfalse;
			}
			else {
				level.paused = PAUSE_NONE;
				AP("print \"^1FIGHT!\n\"");
				//AAPS("sound/match/fight.wav");
				trap_SetConfigstring(CS_PAUSED, "0");
				trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime + level.timeDelta));
			}
		}
		break;
	default:
		break;
	}

	dpent->nextthink = think_next;
	if (fFree) {
		dpent->think = 0;
		G_FreeEntity(dpent);
	}
}

static char *pszDPInfo[] =
{
	"DPRINTF_PAUSEINFO",
	"DPRINTF_UNPAUSING",
	"DPRINTF_CONNECTINFO",
	"DPRINTF_MVSPAWN",
	"DPRINTF_UNK1",
	"DPRINTF_UNK2",
	"DPRINTF_UNK3",
	"DPRINTF_UNK4",
	"DPRINTF_UNK5"
};

/**
 * @brief G_spawnPrintf
 * @param[in] print_type
 * @param[in] print_time
 * @param[in] owner
 */
void G_spawnPrintf(int print_type, int print_time, gentity_t *owner) {
	gentity_t* ent = G_Spawn();

	ent->classname  = pszDPInfo[print_type];
	ent->clipmask   = 0;
	ent->parent     = owner;
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eFlags  |= EF_NODRAW;
	ent->s.eType    = ET_ITEM;

	ent->spawnflags = print_type;       // Tunnel in DP enum
	ent->timestamp  = level.time;       // Time entity was created

	ent->nextthink = print_time;
	ent->think     = G_delayPrint;

	// Set it here so client can do it's own magic..
	if (print_type == DP_PAUSEINFO)
		trap_SetConfigstring(CS_PAUSED, va("%d", match_timeoutlength.integer));
	else if (print_type == DP_UNPAUSING)
		trap_SetConfigstring(CS_PAUSED, "10000");
}

/*
=================
G_handlePause

Central function for (un)pausing the game.
=================
*/
void G_handlePause(qboolean dPause, int time) {
	if (dPause) {
		level.paused = 100 + time;
		G_spawnPrintf(DP_PAUSEINFO, level.time + 15000, NULL);
	}
	else {
		level.paused = PAUSE_UNPAUSING;
		G_spawnPrintf(DP_UNPAUSING, level.time + 10, NULL);
	}
}