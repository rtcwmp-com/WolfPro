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
#ifndef _S_HTTP
#define _S_HTTP

#include <curl/curl.h>
#include <curl/easy.h>
#include "threads.h"
#ifdef DEDICATED
#include "../server/server.h"
#else
#include "../client/client.h"
#endif


/*
===============
HTTP_Reply

Structure for replies.
===============
*/
struct HTTP_Reply_t {
	char*	ptr;
	size_t	len;
};

/*
============
HTTP_Inquiry_t

Structure for issuing inquiries and invoking callbacks.
============
*/
typedef struct {
	char*	url;
	char*	param;
	char	userinfo[MAX_INFO_STRING];

	void (*callback)(char* fmt, ...);
} HTTP_Inquiry_t;



/*
============
Lazy way for submitting SS..
eventually use struct above
============
*/
typedef struct {
	char* address;
	char* hookid;
	char* hooktoken;
	char* waittime;
	char* datetime;
	char* name;
	char* guid;
	char* protocol;
	char* filepath;
	char* filename;
	void (*callback)(char* fmt, ...);
} SS_info_t;


/*
============
http_stats_t

Structure for API stats
============
*/
typedef struct {
    char* url;
	char* filename;
    char* matchid;
	void (*callback)(char* fmt, ...);
} http_stats_t;


/*
============
HTTP_APIInquiry_t

Structure for issuing inquiries and invoking callbacks.
============
*/
typedef struct {
	char* url;
	char* param;
	char* jsonText;
	int clientNumber;
	size_t size;
	char* response;

	void (*callback)(char* fmt, ...);
} HTTP_APIInquiry_t;

void* HTTP_Get(void* args);
size_t HTTP_ParseReply(void* ptr, size_t size, size_t nmemb, struct HTTP_Reply_t* s);
char* getCurrentPath(char* file);
void* CL_HTTP_SSUpload(void* args);

#endif // ~_S_HTTP
