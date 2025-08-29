#include "client.h"

cvar_rest_t* cvar_rest_vars;

#define FILE_HASH_SIZE      1024
#define MAX_CVARS   2048
static cvar_rest_t* restHashTable[FILE_HASH_SIZE];
cvar_rest_t cvar_rest_indexes[MAX_CVARS];
int cvar_rest_numIndexes;
/*
============
Cvar_RestrictedList_f

Prints the list of all the restricted/controlled cvars on a server.
============
*/
void Cvar_RestrictedList_f(void) {
	cvar_rest_t* var;
	int i, j = 0;
#ifndef DEDICATED
	int v = 0;
#endif

//#ifndef _DEBUG
//	if (!com_dedicated->integer && !clientIsConnected) {
//		Com_Printf("Restricted list is not available in offline mode.\n");
//		return;
//	}
//#endif // !_DEBUG

#ifdef DEDICATED
	Com_Printf("Active restricted cvars:\n^7-----------------------------------------\n");
#else
	Com_Printf("^5Active restricted cvars:\n^7-----------------------------------------\n");
#endif

	i = 0;
	for (var = cvar_rest_vars; var; var = var->next, i++) {

		if (!var) {
			continue;
		}

		if (var->type == SVC_NONE) {
			continue;
		}

#ifdef DEDICATED
		Com_Printf("%-32s %s %s %s\n", 
			var->name,
			Cvar_Restriction_Flags[var->type].longDesc, 
			var->sVal1, 
			(var->sVal2 == NULL ? NULL : var->sVal2)
		);
#else
#if _DEBUG
		Com_Printf("%s%-32s %5s %s %s %s ^z[%s]\n", 
			(var->flagged ? "^n" : "^5"), 
			var->name, 
			(var->flagged?"[NO]":"[OK]"),
			Cvar_Restriction_Flags[var->type].longDesc, 
			var->sVal1, 
			(var->sVal2 == NULL ? NULL : var->sVal2),
			Cvar_VariableString(var->name)
		);
#else
		Com_Printf("%s%-32s %5s %s %s %s\n",
			(var->flagged?"^n":"^5"),
			var->name,
			(var->flagged?"[NO]":"[OK]"),
			Cvar_Restriction_Flags[var->type].longDesc,
			var->sVal1,
			(var->sVal2 == NULL ? NULL : var->sVal2)
		);
#endif
#endif
		j++;
#ifndef DEDICATED
		if (var->flagged) {
			v++;
		}
#endif
	}

#ifdef DEDICATED
	if (i != j) {
		Com_Printf("\nIgnored cvars due wrong values:\n^7-----------------------------------------\n");

		for (var = cvar_rest_vars; var; var = var->next) {

			if (var->type != SVC_NONE) {
				continue;
			}
			Com_Printf("%-32s %s %s %s\n", var->name, Cvar_Restriction_Flags[var->type].longDesc, var->sVal1, (var->sVal2 == NULL ? NULL : var->sVal2));
		}
	}
#endif

	if (j < 1) {
		Com_Printf("<none set>\n");
	}
	else {
		Com_Printf("-----------------------------------------\n");
#ifdef DEDICATED
		Com_Printf("Active %i restricted cvars [Total %d]\n\n", j, i);
#else
		Com_Printf("^5Total: %i restricted cvar%s ^n[Violations: %i]\n\n", j, (i == 1?"":"s"), v);
#endif
	}
}

/*
============
Cvar_Rest_Reset_Single

Wipes the cvar completely ..
============
*/
void Cvar_Rest_Reset_Single(cvar_rest_t* var) {
	if (var->sVal1 && var->sVal1 != NULL) {
		Z_Free(var->sVal1);
	}
	if (var->sVal2 && var->sVal2 != NULL) {
		Z_Free(var->sVal2);
	}

	memset(var, 0, sizeof(cvar_rest_t));
}

/*
============
Cvar_Rest_Reset

Wipes the list completely ..
============
*/
void Cvar_Rest_Reset(void) {
	cvar_rest_t* var;
	cvar_rest_t** prev;

	prev = &cvar_rest_vars;
	while (1) {
		var = *prev;
		if (!var) {
			break;
		}

		*prev = var->next;
		Cvar_Rest_Reset_Single(var);
	}
}

/*
============
Cvar_GetRestrictedList

Builds restricted list that is send to a client
============
*/
char* Cvar_GetRestrictedList(void) {
	cvar_rest_t* var;
	char* out = "";

	for (var = cvar_rest_vars; var; var = var->next) {
		if (var->type == SVC_NONE) {
			continue;
		}

		if (Q_stricmp(var->name, "")) {
			out = va("%s%s\\%d\\%s\\%s|", out, var->name, var->type, var->sVal1, (!Q_stricmp(var->sVal2, "") ? "@" : var->sVal2));
		}
	}
	return out;
}

/*
============
var_RestValueIsValid

Check if value can be applied
============
*/
qboolean Cvar_RestValueIsValid(cvar_rest_t* var, const char* value) {

	if (var && strlen(value) > 0) {
		float fValue = 0.0;

		if (var->type == SVC_NONE) {
			return qtrue;
		}

		if (Q_IsNumeric(value)) {
			fValue = atof(value);
		}

		switch (var->type) {
			case SVC_EQUAL:
				if (Q_IsNumeric(value)) {
					if (var->fVal1 != fValue) {
						return qfalse;
					}
					return qtrue;
				}
				return Q_stricmp(var->sVal1, value);
			break;
			case SVC_NOTEQUAL:
				if (Q_IsNumeric(value)) {
					if (var->fVal1 == fValue) {
						return qfalse;
					}
					return qtrue;
				}
				return !Q_stricmp(var->sVal1, value);
			break;
			case SVC_GREATER:
				return (fValue > var->fVal1);
			break;
			case SVC_GREATEREQUAL:
				return (fValue >= var->fVal1);
			break;
			case SVC_LOWER:
				return (fValue < var->fVal1);
			break;
			case SVC_LOWEREQUAL:
				return (fValue <= var->fVal1);
			break;
			case SVC_INSIDE:
				return (fValue >= var->fVal1 && fValue <= var->fVal2);
			break;
			case SVC_OUTSIDE:
				return !(fValue >= var->fVal1 && fValue <= var->fVal2);
			break;
			case SVC_INCLUDE:
				return (strstr(value, var->sVal1) ? qtrue : qfalse);
			break;
			case SVC_EXCLUDE:
				return (!strstr(value, var->sVal1) ? qtrue : qfalse);
			break;
			case SVC_WITHBITS:
				return !(var->iVal1 & atoi(value));
			break;
			case SVC_WITHOUTBITS:
				return (var->iVal1 & atoi(value));
			break;
		}
	}
	return qtrue;
}

/*
============
Cvar_ValidateRest

Validates cvars and returns violations count
============
*/
int Cvar_ValidateRest(void) {
	cvar_t* var;
	cvar_rest_t* cv;
	int i = 0, violations = 0;

	for (cv = cvar_rest_vars; cv; cv = cv->next, i++) {
		cv->flagged = qfalse;

		var = Cvar_FindVar(cv->name);
		if (!var) {
			if (cv->type == SVC_INCLUDE || cv->type == SVC_WITHBITS) {
				violations++;

				cv->flagged = qtrue;
			}
		}
		else if (!Cvar_RestValueIsValid(cv, var->string)) {
			violations++;

			cv->flagged = qtrue;
		}
	}
	return (i > 0 ? violations : -1);
}

/*
============
Cvar_ValidateString
============
*/
static qboolean Cvar_ValidateString( const char *s ) {
	if ( !s ) {
		return qfalse;
	}
	if ( strchr( s, '\\' ) ) {
		return qfalse;
	}
	if ( strchr( s, '\"' ) ) {
		return qfalse;
	}
	if ( strchr( s, ';' ) ) {
		return qfalse;
	}
	return qtrue;
}

/*
============
Cvar_Rest_ValidateString
============
*/
static qboolean Cvar_Rest_ValidateString(const char* s) {
	if (!s) {
		return qfalse;
	}
	if (strchr(s, '/') && strchr(s + 1, '/')) {
		return qfalse;
	}
	return Cvar_ValidateString(s);
}

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname ) {
	int i;
	long hash;
	char letter;

	if ( !fname ) {
		Com_Error( ERR_DROP, "null name in generateHashValue" );       //gjd
	}
	hash = 0;
	i = 0;
	while ( fname[i] != '\0' ) {
		letter = tolower( fname[i] );
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash &= ( FILE_HASH_SIZE - 1 );
	return hash;
}

/*
============
CvarRest_FindVar
============
*/
cvar_rest_t* Cvar_Rest_FindVar(const char* var_name) {
	cvar_rest_t* var;
	long hash;

	hash = generateHashValue(var_name);

	for (var = restHashTable[hash]; var; var = var->hashNext) {
		if (!Q_stricmp(var_name, var->name)) {
			return var;
		}
	}

	return NULL;
}


/*
============
Cvar_Rest_Get

If the variable already exists, the value will not be set 
============
*/
cvar_rest_t* Cvar_Rest_Get(const char* var_name, int type, const char* value1, const char* value2) {
	cvar_rest_t* var;
	long hash;

	if (!var_name) {
		Com_Error(ERR_FATAL, "Cvar_Get: NULL parameter");
	}

	if (!Cvar_Rest_ValidateString(var_name)) {
		Com_Printf("invalid cvar name string: %s\n", var_name);
		var_name = "BADNAME";
		return NULL;
	}

	var = Cvar_Rest_FindVar(var_name);
	if (var) {
		return var;
	}

	//
	// allocate a new cvar
	//
	if (cvar_rest_numIndexes >= MAX_CVARS) {
		Com_Error(ERR_FATAL, "MAX_REST_CVARS");
		return var;
	}

	var = &cvar_rest_indexes[cvar_rest_numIndexes];
	cvar_rest_numIndexes++;
	var->name = CopyString(var_name);
	var->type = type;
	var->sVal1 = CopyString(value1);
	var->sVal2 = (value2 == NULL ? NULL : CopyString(value2));
	var->fVal1 = atof(var->sVal1);
	var->fVal2 = (value2 == NULL ? 0.00 : atof(var->sVal2));
	var->iVal1 = atoi(var->sVal1);
	var->iVal2 = (value2 == NULL ? 0 : atoi(var->sVal2));

	// link the variable in
	var->next = cvar_rest_vars;
	cvar_rest_vars = var;

	hash = generateHashValue(var_name);
	var->hashNext = restHashTable[hash];
	restHashTable[hash] = var;

	return var;
}

/*
============
Cvar_SetRestricted

Sets restricted cvar flags.
============
*/
cvar_rest_t* Cvar_SetRestricted(const char* var_name, unsigned int type, const char* value1, const char* value2) {
	cvar_rest_t* var;

	Com_DPrintf("Cvar_SetRestricted: %s %d %s %s\n", var_name, type, value1, value2);

	if (!Cvar_Rest_ValidateString(var_name)) {
		Com_Printf("invalid cvar name string: %s\n", var_name);
		return NULL;
	}

	var = Cvar_Rest_FindVar(var_name);
	if (!var) {
		return Cvar_Rest_Get(var_name, type, value1, value2);
	}

	if (!value1) {
		return var;
	}

	Z_Free(var->sVal1);
	Z_Free(var->sVal2);

	var->type = type;
	var->sVal1 = CopyString(value1);
	var->sVal2 = (value2 == NULL ? NULL : CopyString(value2));
	var->fVal1 = atof(var->sVal1);
	var->fVal2 = (value2 == NULL ? 0.00 : atof(var->sVal2));
	var->iVal1 = atoi(var->sVal1);
	var->iVal2 = (value2 == NULL ? 0 : atoi(var->sVal2));

	return var;
}


/*
=================
Cvar_RestBuildList

Builds actual data
=================
*/
void Cvar_RestBuildList(char* data) {
	Cvar_Rest_Reset();

	if (data) {
		char* next = NULL;
		char* first = strtok_r(data, "|", &next);

		do {
			char* posn = NULL;

			Cmd_TokenizeLine(first, "\\", posn);
			if (Cmd_Argv(0)) {
				Cvar_SetRestricted(Cmd_Argv(0), atoi(Cmd_Argv(1)), Cmd_Argv(2), !Q_stricmp(Cmd_Argv(3), "@") ? "" : Cmd_Argv(3));
			}
		} while ((first = strtok_r(NULL, "|", &next)) != NULL);
	}
	Com_DPrintf("\nRestriction list has been updated.\n");
}

/*
============
Cvar_RestFlagged

Checks if cvar is flagged
============
*/
qboolean Cvar_RestFlagged(const char* var_name) {
	cvar_rest_t* var;

	var = Cvar_Rest_FindVar(var_name);
	if (var) {
		return var->flagged;
	}
	return qfalse;
}

/*
============
Cvar_RestAcceptedValues

Returns which values are accepted for a specific cvar
============
*/
char* Cvar_RestAcceptedValues(const char* var_name) {
	cvar_rest_t* var;
	char* message = "";

	var = Cvar_Rest_FindVar(var_name);
	if (!var || var->type == SVC_NONE) {
		return va("%s is not a restricted variable.", var_name);
	}

	switch (var->type) {
		case SVC_EQUAL:
			message = va("MUST %s %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1);
		break;
		case SVC_NOTEQUAL:
			message = va("MUST %s %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1);
		break;
		case SVC_GREATER:
			message = va("HAS TO BE %s THAN %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1);
		break;
		case SVC_GREATEREQUAL:
			message = va("HAS TO BE %s THAN %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1);
		break;
		case SVC_LOWER:
			message = va("HAS TO BE %s THAN %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1);
		break;
		case SVC_LOWEREQUAL:
			message = va("HAS TO BE %s THAN %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1);
		break;
		case SVC_INSIDE:
			message = va("HAS TO BE %s %s AND %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1, var->sVal2);
		break;
		case SVC_OUTSIDE:
			message = va("HAS TO BE %s %s AND %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1, var->sVal2);
		break;
		case SVC_INCLUDE:
			message = va("MUST %s %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1);
		break;
		case SVC_EXCLUDE:
			message = va("MUST %s %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1);
		break;
		case SVC_WITHBITS:
			message = va("MUST BE %s %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1);
		break;
		case SVC_WITHOUTBITS:
			message = va("MUST %s %s", Cvar_Restriction_Flags[var->type].longDesc, var->sVal1);
		break;
	}
	return message;
}

/*
============
RestrictedTypeToInt

Remaps string to int
============
*/
unsigned int RestrictedTypeToInt(char* str) {
	int i;

	for (i = 0; i < SVC_MAX; i++) {
		if (!Q_stricmp(str, Cvar_Restriction_Flags[i].operatorFlag))
			return Cvar_Restriction_Flags[i].type;
	}
	return SVC_NONE;
}


#ifndef DEDICATED
/*
====================
CL_GetRestStatus
====================
*/
void CL_SetRestStatus(void) {
	Cvar_ValidateRest();
	cl.handle.warnedTime = cls.realtime + RKVALD_TIME_PING;
	cl.handle.doPrint = qtrue;
}

/*
====================
CL_CheckRestStatus
====================
*/
void CL_CheckRestStatus(void) {

	if (cl.handle.doPrint) {
		if (cls.realtime > cl.handle.warnedTime) {
			int violations = Cvar_ValidateRest();

			if (violations > 0) {
				Com_Printf("^5CVAR >>>>>\n");
				Com_Printf("^5CVAR >>>>> You have %d setting%s violating server rules.\n", violations, (violations > 1 ? "s" : ""));
				Com_Printf("^5CVAR >>>>> Please use /violations and correct them.\n");
				Com_Printf("^5CVAR >>>>>\n");
			}
			cl.handle.warnedTime = cls.realtime + (violations < 1 ? RKVALD_TIME_PING_L : RKVALD_TIME_PING_S);
			CL_AddReliableCommand(va("%s %s", CTL_RKVALD, violations < 1 ? RKVALD_OK : RKVALD_NOT_OK));
		}
	}
}
#endif