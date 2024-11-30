#include "g_local.h"

char *aTeams[TEAM_NUM_TEAMS] = { "FFA", "^1Axis^7", "^4Allies^7", "Spectators" };
team_info teamInfo[TEAM_NUM_TEAMS];

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