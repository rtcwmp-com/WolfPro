#include "g_local.h"




// Puts a player on a team.
void G_refPlayerPut_cmd(int team_id) {
	char arg[MAX_TOKEN_CHARS];
	gentity_t *player;

	// Works for teamplayish matches
	if (g_gametype.integer < GT_WOLF) {
		G_Printf("\"put[allies|axis]\" only for team-based games!");
		return;
	}

    int argc = trap_Argc();
    if(trap_Argc() < 2){
        G_Printf("Usage: put[allies|axis|spec] <player ID>");
    }

	// Find the player to place.
	trap_Argv(2, arg, sizeof(arg));
	int pid = Q_atoi(arg);

	player = g_entities + pid;

	// Can only move to other teams.
	if (player->client->sess.sessionTeam == team_id) {
		G_Printf("\"%s\" is already on team %s!\n", player->client->pers.netname, aTeams[team_id]);
		return;
	}

	player->client->pers.ready = qfalse;

	if (team_id == TEAM_RED) {
		SetTeam(player, "red");
	} else if (team_id == TEAM_BLUE){
		SetTeam(player, "blue"); 
	} else {
        SetTeam(player, "spectator");
    }

}



qboolean G_refCommandCheck() {
    char cmd[MAX_TOKEN_CHARS];
    trap_Argv(1, cmd, sizeof(cmd));
    if (!Q_stricmp(cmd, "putallies")) {
		G_refPlayerPut_cmd(TEAM_BLUE);
	} else if (!Q_stricmp(cmd, "putaxis")) {
		G_refPlayerPut_cmd(TEAM_RED);
	} else if (!Q_stricmp(cmd, "putspec")) {
		G_refPlayerPut_cmd(TEAM_SPECTATOR);
	} else { return(qfalse); }

	return(qtrue);
}

