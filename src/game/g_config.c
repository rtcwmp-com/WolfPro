/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "g_local.h"
#include "../../MAIN/ui_mp/menudef.h" // For vote options



// *** G_PrintConfigs ***
void G_PrintConfigs(gentity_t* ent) {
	char configNames[8192];
	char filename[MAX_QPATH];
	int  numconfigs = 0, i = 0, namelen = 0;
	char* configPointer;
	char gameConfig[MAX_QPATH];

	G_Printf("Starting to read configs\n");
	trap_Cvar_VariableStringBuffer("sv_GameConfig", gameConfig, sizeof(gameConfig));

	numconfigs = trap_FS_GetFileList("configs", ".config", configNames, sizeof(configNames));
	configPointer = configNames;
	for (i = 0; i < numconfigs; i++, configPointer += namelen + 1) {
		namelen = strlen(configPointer);
		Q_strncpyz(filename, Q_StrReplace(configPointer, ".config", ""), sizeof(filename));

		if (!Q_stricmp(filename, gameConfig)) {
			G_EntPrintf(ent, "^7Config: ^3%s ^7[active]", filename);
		} else {
			G_EntPrintf(ent, "^7Config: ^3%s", filename);
		}
	}
	G_Printf("Config list done.\n");
}

// *** Checks if config file is in paths (used before initiating a vote for configs) ***
qboolean G_isValidConfig(gentity_t* ent, const char* configname) {
	char filename[MAX_QPATH];

	if (configname[0]) {
		Q_strncpyz(filename, configname, sizeof(filename));
	}
	else {
		G_EntPrintf(ent, "^7No config set.");
		return qfalse;
	}

	if (!trap_FS_FileExists(va("configs/%s.config", filename))) {
		G_EntPrintf(ent, "^3Warning: No config with filename '%s' found\n", filename);
		return qfalse;
	}
	return qtrue;
}

// ***  Force settings to predefined state. ***
qboolean G_ConfigSet(const char* configName) {
	char gameConfig[MAX_QPATH];

	G_Printf("Will try to load %s config ..\n", configName);
	if (!trap_FS_FileExists(va("configs/%s.config", configName))) {
		G_Printf("^3Warning: No config with filename [%s] found\n", configName);
		return qfalse;
	}

	trap_Cvar_VariableStringBuffer("sv_GameConfig", gameConfig, sizeof(gameConfig));
	trap_Cvar_Set("sv_GameConfig", configName);
	//trap_Cvar_Restrictions_Load();

	G_Printf(">> %s settings loaded.\n", Info_ValueForKey(gameConfig, "sv_GameConfig")); // Use info so we're sure it has been set.
	if (g_gamestate.integer == GS_WARMUP_COUNTDOWN) {
		level.lastRestartTime = level.time;
	}
	trap_SendConsoleCommand(EXEC_APPEND, va("map_restart 0 %i\n", GS_WARMUP));

	return qtrue;
}