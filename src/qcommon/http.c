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
#include "http.h"
#include <sys/stat.h> // reqSS


static size_t read_callback(char* ptr, size_t size, size_t nmemb, void* stream)
{
	curl_off_t nread;

	size_t retcode = fread(ptr, size, nmemb, stream);

	nread = (curl_off_t)retcode;

	fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
		" bytes from file\n", nread);
	return retcode;
}
/*
===============
reqSS
===============
*/
void* CL_HTTP_SSUpload(void* args) {
	SS_info_t* SS_info = (SS_info_t*)args;
	CURL* curl;
	CURLcode res;
	struct curl_slist* headerlist = NULL;

	FILE* fd;

	fd = fopen(SS_info->filepath, "rb");

	if (!fd)
	{
		Com_DPrintf("HTTP[fu]: cannot o/r\n");
		return NULL;
	}

	curl = curl_easy_init();

	headerlist = curl_slist_append(headerlist, SS_info->hookid);
	headerlist = curl_slist_append(headerlist, SS_info->hooktoken);
	headerlist = curl_slist_append(headerlist, SS_info->name);
    headerlist = curl_slist_append(headerlist, SS_info->guid);
	headerlist = curl_slist_append(headerlist, SS_info->waittime);
	headerlist = curl_slist_append(headerlist, SS_info->datetime);
	headerlist = curl_slist_append(headerlist, SS_info->protocol);


	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
		curl_easy_setopt(curl, CURLOPT_URL, va("http://%s/%s.jpg", SS_info->address, SS_info->filename));
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl, CURLOPT_READDATA, fd);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			Com_DPrintf("HTTP[res] failed: %s\n", curl_easy_strerror(res));
		}

		curl_slist_free_all(headerlist);
		curl_easy_cleanup(curl);
	}

	fclose(fd);
	remove(SS_info->filepath);
	return NULL;
}

/*
===============
HTTP_InitString

Allocates memory for web request.
===============
*/
void HTTP_InitString(struct HTTP_Reply_t* s) {
	s->len = 0;
	s->ptr = malloc(s->len + 1);

	if (s->ptr == NULL) {
		Com_DPrintf("HTTP[i_s]: malloc() failed\n");
		return;
	}
	s->ptr[0] = '\0';
}

/*
===============
HTTP_ParseReply

Parses web reply. 
===============
*/
size_t HTTP_ParseReply(void* ptr, size_t size, size_t nmemb, struct HTTP_Reply_t* s) {
	size_t new_len = s->len + size * nmemb;
	char* tmp;

	tmp = realloc(s->ptr, new_len + 1);
	if (tmp == NULL) {
		Com_DPrintf("HTTP[write]: realloc() failed\n");
		return 0;
	}

	s->ptr = tmp;
	memcpy(s->ptr + s->len, ptr, size * nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size * nmemb;
}


/*
===============
getCurrentPath

Because we're not going through Game we need to sort stuff ourself.
===============
*/
char* getCurrentPath(char* file) {
	char* path = Cvar_VariableString("fs_game");

	return (strlen(path) < 2 ? va("%s/%s", BASEGAME, file) : va("%s/%s", path, file));
}

/*
===============
HTTP_Get

Sends a request and returns a response.
===============
*/
void* HTTP_Get(void* args) {
	HTTP_Inquiry_t* inquiry = (HTTP_Inquiry_t*)args;
	CURL* handle;
	CURLcode res;
	
	handle = curl_easy_init();
	if (handle) {
		struct HTTP_Reply_t s;
		HTTP_InitString(&s);
		struct curl_slist* headers = NULL;

		headers = curl_slist_append(headers, va("Mod: %s", GAMEVERSION));

		curl_easy_setopt(handle, CURLOPT_URL,inquiry->url);
		curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
		curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
		curl_easy_setopt(handle, CURLOPT_FORBID_REUSE, 1L);
		curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5);
		curl_easy_setopt(handle, CURLOPT_POSTFIELDS, inquiry->param);
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, HTTP_ParseReply);
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, &s);
		curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1);

		res = curl_easy_perform(handle);
		if (res != CURLE_OK) {
			Com_DPrintf("HTTP_Get[res] failed: %s\n", curl_easy_strerror(res));
			inquiry->callback(NULL, inquiry->userinfo);
		}
		else {
			Com_Printf("Response: %s\n", s.ptr);
			inquiry->callback(s.ptr, inquiry->userinfo);
			
		}
		free(s.ptr);
		curl_slist_free_all(headers);
	}
	curl_easy_cleanup(handle);
	free(inquiry);
	return 0;
}



