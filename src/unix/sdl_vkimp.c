/*
===========================================================================
Parts taken from CNQ3:
Copyright (C) 2017-2019 Gian 'myT' Schellenbaum

This file is part of RtcwPro.

RtcwPro is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

RtcwPro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RtcwPro. If not, see <https://www.gnu.org/licenses/>.
===========================================================================
*/

/*
** GLW_IMP.C
**
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
** GLimp_SetGamma
**
*/

#include <dlfcn.h>

#include "../renderer_common/tr_local.h"
#include "linux_local.h"

#include "unix_glw.h"

#include "../renderer_vk/volk.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include "sdl_local.h"

#define WINDOW_CLASS_NAME   "Return to Castle Wolfenstein"

glImp_t glimp;
glwstate_t glw_state;


static qbool sdl_IsMonitorListValid()
{
	const int count = glimp.monitorCount;
	const int curr = glimp.monitor;

	return
		count >= 1 && count <= MAX_MONITOR_COUNT &&
		curr >= 0 && curr < count;
}

static int sdl_CompareMonitors( const void* aPtr, const void* bPtr )
{
	const SDL_Rect* const a = &((const monitor_t*)aPtr)->rect;
	const SDL_Rect* const b = &((const monitor_t*)bPtr)->rect;
	const int dy = a->y - b->y;
	if (dy != 0)
		return dy;

	return a->x - b->x;
}

static void sdl_CreateMonitorList()
{
	glimp.monitorCount = 0;

	const int count = SDL_GetNumVideoDisplays();
	if (count <= 0)
		return;

	int gi = 0;
	for (int si = 0; si < count; ++si) {
		if (gi >= MAX_MONITOR_COUNT)
			break;
		if (SDL_GetDisplayBounds(si, &glimp.monitors[gi].rect) == 0) {
			glimp.monitors[gi].sdlIndex = si;
			++gi;
		}
	}
	glimp.monitorCount = gi;

	if (sdl_IsMonitorListValid())
		qsort(glimp.monitors, (size_t)glimp.monitorCount, sizeof(glimp.monitors[0]), &sdl_CompareMonitors);
	else
		glimp.monitorCount = 0;
}

// call this before creating the window
static void sdl_UpdateMonitorIndexFromCvar()
{
	if (glimp.monitorCount <= 0 || glimp.monitorCount >= MAX_MONITOR_COUNT)
		return;

	const int monitor = Cvar_Get("r_monitor", "0", CVAR_ARCHIVE | CVAR_LATCH)->integer;
	if (monitor < 0 || monitor >= glimp.monitorCount) {
		glimp.monitor = 0;
		return;
	}
	glimp.monitor = monitor;
}

static void sdl_PrintMonitorList()
{
	const int count = glimp.monitorCount;
	if (count <= 0) {
		Com_Printf("No monitor detected.\n");
		return;
	}

	Com_Printf("Monitors detected (left is r_monitor ^7value):\n");
	for (int i = 0; i < count; ++i) {
		const SDL_Rect rect = glimp.monitors[i].rect;
		Com_Printf( "%d ^7%dx%d at %d,%d\n", i, rect.w, rect.h, rect.x, rect.y);
	}
}


static void sdl_MonitorList_f()
{
	sdl_CreateMonitorList();
	sdl_UpdateMonitorIndexFromCvar();
	sdl_PrintMonitorList();
}


// call this after the window has been moved
void sdl_UpdateMonitorIndexFromWindow()
{
	if (glimp.monitorCount <= 0)
		return;

	// try to find the glimp index and update data accordingly
	const int sdlIndex = SDL_GetWindowDisplayIndex(glimp.window);
	for (int i = 0; i < glimp.monitorCount; ++i) {
		if (glimp.monitors[i].sdlIndex == sdlIndex) {
			glimp.monitor = i;
			Cvar_Set("r_monitor", va("%d", i));
			break;
		}
	}
}


void sdl_Window( const SDL_WindowEvent* event )
{
	// events of interest:
	//SDL_WINDOWEVENT_SHOWN
	//SDL_WINDOWEVENT_HIDDEN
	//SDL_WINDOWEVENT_RESIZED
	//SDL_WINDOWEVENT_SIZE_CHANGED // should prevent this from happening except on creation?
	//SDL_WINDOWEVENT_MINIMIZED
	//SDL_WINDOWEVENT_MAXIMIZED
	//SDL_WINDOWEVENT_RESTORED
	//SDL_WINDOWEVENT_ENTER // mouse focus gained
	//SDL_WINDOWEVENT_LEAVE // mouse focus lost
	//SDL_WINDOWEVENT_FOCUS_GAINED // kb focus gained
	//SDL_WINDOWEVENT_FOCUS_LOST // kb focus lost
	//SDL_WINDOWEVENT_CLOSE
	//SDL_WINDOWEVENT_MOVED

	switch (event->event) {
		case SDL_WINDOWEVENT_MAXIMIZED:
		case SDL_WINDOWEVENT_RESTORED:
		case SDL_WINDOWEVENT_RESIZED:
		case SDL_WINDOWEVENT_SIZE_CHANGED:
		case SDL_WINDOWEVENT_MOVED:
			// if this turns out to be too expensive, track movement and
			// only call when movement stops
			sdl_UpdateMonitorIndexFromWindow();
			break;

		default:
			break;
	}

}


static void sdl_GetSafeDesktopRect( SDL_Rect* rect )
{
	if (!sdl_IsMonitorListValid()) {
		rect->x = 0;
		rect->y = 0;
		rect->w = 1280;
		rect->h = 720;
	}

	*rect = glimp.monitors[glimp.monitor].rect;
}

typedef enum
{
	RSERR_OK,
	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,
	RSERR_UNKNOWN
} rserr_t;



/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
void GLimp_Shutdown( void ) {
	if (glimp.glContext != NULL) {
		glimp.glContext = NULL;
	}

	if (glimp.window != NULL) {
		SDL_DestroyWindow(glimp.window);
		glimp.window = NULL;
	}
}


/*****************************************************************************/
/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] ) {
}

/*
** GLimp_LogComment
*/
void GLimp_LogComment( char *comment ) {
	if ( glw_state.log_fp ) {
		fprintf( glw_state.log_fp, "%s", comment );
	}
}


// @TODO: use it somewhere like before??? :p
static qbool VKW_SetMode(void)
{

	sdl_CreateMonitorList();
	sdl_UpdateMonitorIndexFromCvar();
	sdl_PrintMonitorList();

	SDL_Rect deskropRect;
	sdl_GetSafeDesktopRect(&deskropRect);
	RE_ConfigureVideoMode(deskropRect.w, deskropRect.h);

	Uint32 windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN;
	if (glInfo.winFullscreen) {
			windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}


	// load and initialize the specific OpenGL driver
	//
	// if ( !SDL_Vulkan_LoadLibrary( NULL ) ) {
	// 	volkInitializeCustom(SDL_Vulkan_GetVkGetInstanceProcAddr());
	// }else{
	// 	SDL_GetError();
	// 	ri.Error( ERR_FATAL, "SDL_Vulkan_LoadLibrary() - could not load Vulkan subsystem\n" );
	// }

	glimp.window = SDL_CreateWindow(WINDOW_CLASS_NAME, deskropRect.x, deskropRect.y, glConfig.vidWidth, glConfig.vidHeight, windowFlags);
	if(!SDL_Vulkan_GetVkGetInstanceProcAddr()){
		return qfalse;
	}
	return qtrue;
}


uint64_t Sys_Vulkan_Init( void* vkInstance )
{
	

	VkSurfaceKHR surface;

	if(!SDL_Vulkan_CreateSurface(glimp.window, (VkInstance)vkInstance, &surface)){
		Com_Printf("failed to create surface, SDL Error: %s, %x", SDL_GetError(), vkInstance);
	}

	return (uint64_t)surface;
}

void Sys_Vulkan_Shutdown(void)
{
	if (glimp.window != NULL) {
		ri.Printf( PRINT_ALL, "...destroying window\n" );
		SDL_DestroyWindow(glimp.window);
		glimp.window = NULL;
	}

}

qboolean Sys_Vulkan_GetRequiredExtensions(char **pNames, int *pCount){
	if(glimp.window == NULL){
		if(!VKW_SetMode())
		{
			Com_Error(ERR_FATAL, "VKW_SetMode failed\n");
		}
	}
	
	return (qboolean)SDL_Vulkan_GetInstanceExtensions(glimp.window, pCount, pNames);
}