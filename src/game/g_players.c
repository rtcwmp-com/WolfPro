#include "g_local.h"

/*
===================
READY / NOTREADY

Sets a player's "ready" status.

Tardo - rewrote this because the parameter handling to the function is different in rtcw.
===================
*/
void G_readyHandle( gentity_t* ent, qboolean ready ) {
	ent->client->pers.ready = ready;
}

void G_ready_cmd( gentity_t *ent, qboolean state ) {
	char *status[2] = { "^zNOT READY^7", "^3READY^7" };

	if (!g_tournament.integer) {
		return;
	}

	if ( g_gamestate.integer == GS_PLAYING || g_gamestate.integer == GS_INTERMISSION ) {
		CP( "@print \"Match is already in progress!\n\"" );
		return;
	}

	if ( !state && g_gamestate.integer == GS_WARMUP_COUNTDOWN ) {
		CP( "print \"Countdown started..^znotready^7 ignored!\n\"" );
		return;
	}

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		CP( va("print \"Specs cannot use %s ^7command!\n\"", status[state] ));
		return;
	}

	//if (level.readyTeam[ent->client->sess.sessionTeam] == qtrue && !state) { // Doesn't cope with unreadyteam but it's out anyway..
	//	CP(va("print \"%s ^7ignored. Your team has issued ^3TEAM READY ^7command..\n\"", status[state]));
	//	return;
	//}

	// Move them to correct ready state
	if ( ent->client->pers.ready == state ) {
		CP( va( "print \"You are already %s!\n\"", status[state] ) );
	} else {
		ent->client->pers.ready = state;
		if ( !level.intermissiontime ) {
			if (state) {
				ent->client->pers.ready = qtrue;
				//ent->client->ps.powerups[PW_READY] = INT_MAX;
				//player_ready_status[ent->client->ps.clientNum].isReady = 1;
			}
			else {
				ent->client->pers.ready = qfalse;
				//ent->client->ps.powerups[PW_READY] = 0;
				//player_ready_status[ent->client->ps.clientNum].isReady = 0;
			}

			// Doesn't rly matter..score tab will show slow ones..
			AP( va( "cp \"\n%s \n^3is %s!\n\"", ent->client->pers.netname, status[state] ) );
		}
	}
}

/*
===================
TEAM-READY / NOTREADY
===================
*/
void pCmd_teamReady(gentity_t *ent, qboolean ready) {
	char *status[2] = { "NOT READY", "READY" };
	int i, p = { 0 };
	int team = ent->client->sess.sessionTeam;
	gentity_t *cl;

	if (!g_tournament.integer) {
		return;
	}
	if (team_nocontrols.integer) {
		CP("print \"Team commands are not enabled on this server.\n\"");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		CP("print \"Specs cannot use ^3TEAM ^7commands.\n\"");
		return;
	}
	if (!ready && g_gamestate.integer == GS_WARMUP_COUNTDOWN) {
		CP("print \"Countdown started, ^3notready^7 ignored.\n\"");
		return;
	}

	for (i = 0; i < level.numConnectedClients; i++) {
		cl = g_entities + level.sortedClients[i];

		if (!cl->inuse) {
			continue;
		}

		if (cl->client->sess.sessionTeam != team) {
			continue;
		}

		if ((cl->client->pers.ready != ready) && !level.intermissiontime) {
			cl->client->pers.ready = ready;
			++p;
		}
	}

	if (!p) {
		CP(va("print \"Your team is already ^3%s^7!\n\"", status[ready]));
	}
	else {
		AP(va("cp \"%s ^7team is %s%s!\n\"", aTeams[team], (ready ? "^3" : "^z"), status[ready]));
		G_matchPrintInfo(va("%s ^7team is %s%s! ^7(%s)\n", aTeams[team], (ready ? "^3" : "^z"), status[ready], ent->client->pers.netname), qfalse);
		level.readyTeam[team] = ready;
	}
}

/*
===================
Pause/Unpause
===================
*/
void pCmd_pauseHandle(gentity_t *ent, qboolean dPause) {
    int team = ent->client->sess.sessionTeam;
	char* status[2] = {"^3UN", "^3"};
    char tName[MAX_NETNAME];

	if (team_nocontrols.integer) {
		CP("print \"Team commands are not enabled on this server.\n\"");
		return;
	}

	if (team == TEAM_FREE || team == TEAM_SPECTATOR) {
		CP("print \"^jError: ^7Pause cannot be issued by a spectator!\n\"");
		return;
	}

	if ( g_gamestate.integer != GS_PLAYING ) {
		CP("print \"^jError: ^7Pause feature can only be issued during a match!\n\"");
		return;
	}

	if (level.numPlayingClients == 0) {
		CP("print \"^jError: ^7You cannot use pause feature with no playing clients..\n\"");
		return;
	}

	if ((!level.alliedPlayers || !level.axisPlayers) && dPause) {
		CP("print \"^jError: ^7Pause can only be used when both teams have players!\n\"");
		return;
	}

	if ((PAUSE_UNPAUSING >= level.paused && !dPause) || (PAUSE_NONE != level.paused && dPause)) {
		CP(va("print \"^jError: ^7The match is already %sPAUSED^7!\n\"", status[dPause]));
		return;
	}

	DecolorString(aTeams[team], tName);
	if (dPause) {
		if (!teamInfo[team].timeouts) {
			CP("print \"^jError: ^7Your team has no more timeouts remaining!\n\"");
			//CPS(ent, "sound/misc/denied.wav");
			return;
		}

		teamInfo[team].timeouts--;
		level.paused = team + 128;
		G_spawnPrintf(DP_PAUSEINFO, level.time + 15000, NULL);

		AP(va("chat \"^zconsole: ^7%s has ^3Paused ^7the match!\n\"", tName));
		AP(va("cp \"[%s^7] %d Timeouts Remaining\n\"3", aTeams[team], teamInfo[team].timeouts));
		AP(va("@print \"^z>> ^7%s ^zPaused the match.\n\"", ent->client->pers.netname));
		//AAPS("sound/match/klaxon1.wav");

	} 
	else if (team + 128 != level.paused) {
		CP("print \"^jError: ^7Your team did not call the Pause^j.\n\"");
		return;	
	}
	else {
		//AAPS("sound/match/prepare.wav");
		level.paused = PAUSE_UNPAUSING;
		G_spawnPrintf(DP_UNPAUSING, level.time + 10, NULL);
		AP(va("chat \"^zconsole: ^7%s has ^3Unpaused ^7a match!\n\"", tName));
		AP(va("print \"^z>> ^7%s ^zUnpaused the match.\n\"", ent->client->pers.netname));
		// lock the teams after unpausing
		teamInfo[TEAM_RED].team_lock = qtrue;
		teamInfo[TEAM_BLUE].team_lock = qtrue;
	}

    // if (g_gameStatslog.integer) {
    //     G_writeGeneralEvent (ent , ent, " ", (dPause) ? eventUnpause : eventPause);  // might want to distinguish between player and admin here?
    // }
	return;
}


/*
===================
OSP's stats
===================
*/
void G_scores_cmd( gentity_t *ent ) {
	G_printMatchInfo( ent , qfalse);
}

void G_scoresDump_cmd( gentity_t *ent ) {
	G_printMatchInfo( ent , qtrue);
}
// Shows a player's stats to the requesting client.
void G_weaponStats_cmd( gentity_t *ent ) {
	G_statsPrint( ent, 0 );
}

/******************* Client commands *******************/
qboolean playerCmds (gentity_t *ent, char *cmd ) {
if(!Q_stricmp(cmd, "readyteam"))			{ pCmd_teamReady(ent, qtrue);	return qtrue;}
	else if(!Q_stricmp(cmd, "ready"))				{ G_ready_cmd( ent, qtrue ); return qtrue;}
	else if(!Q_stricmp(cmd, "unready") ||
			!Q_stricmp(cmd, "notready"))			{ G_ready_cmd( ent, qfalse ); return qtrue;}
	else if(!Q_stricmp(cmd, "pause"))				{ pCmd_pauseHandle( ent, qtrue); return qtrue;}
	else if(!Q_stricmp(cmd, "unpause"))				{ pCmd_pauseHandle( ent, qfalse); return qtrue;}
	else if(!Q_stricmp(cmd, "wstats"))				{ G_statsPrint( ent, 1 );	return qtrue;}
	else if(!Q_stricmp(cmd, "cstats"))				{ G_clientStatsPrint( ent, 1, qtrue );	return qtrue;}
	else if(!Q_stricmp(cmd, "stats"))				{ G_clientStatsPrint( ent, 1, qfalse );	return qtrue;}
	else if(!Q_stricmp(cmd, "gamestats"))			{ G_gameStatsPrint(ent);	return qtrue; }
	else if(!Q_stricmp(cmd, "sgstats"))				{ G_statsPrint( ent, 2 );	return qtrue;}
	else if(!Q_stricmp(cmd, "stshots"))				{ G_weaponStatsLeaders_cmd( ent, qtrue, qtrue );	return qtrue;}
	else if(!Q_stricmp(cmd, "scores"))				{ G_scores_cmd(ent);	return qtrue;}
	else if(!Q_stricmp(cmd, "scoresdump"))			{ G_scoresDump_cmd(ent);	return qtrue;}
	else if(!Q_stricmp(cmd, "statsall"))			{ G_statsall_cmd( ent, 0, qfalse );	return qtrue;}
	else if(!Q_stricmp(cmd, "bottomshots"))			{ G_weaponRankings_cmd( ent, qtrue, qfalse );	return qtrue;}
	else if(!Q_stricmp(cmd, "topshots"))			{ G_weaponRankings_cmd( ent, qtrue, qtrue );	return qtrue;}
	else if(!Q_stricmp(cmd, "weaponstats"))			{ G_weaponStats_cmd( ent );	return qtrue;}
	else
		return qfalse;
}