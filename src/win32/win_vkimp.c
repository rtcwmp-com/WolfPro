
#include <assert.h>
#include "../renderer_vk/tr_local_gal.h"
#include "../qcommon/qcommon.h"
#include "resource.h"
#include "glw_win.h"
#include "win_local.h"
#include "../client/client.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "../renderer_vk/tr_vulkan.h"


vkwstate_t vkw_state;

static BOOL CALLBACK WIN_MonitorEnumCallback( HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData )
{
	if ( lprcMonitor ) {
		vk_wv.monitorRects[vk_wv.monitorCount] = *lprcMonitor;
		vk_wv.hMonitors[vk_wv.monitorCount] = hMonitor;
		vk_wv.monitorCount++;
	}

	if ( vk_wv.monitorCount >= MAX_MONITOR_COUNT )
		return FALSE;
	
	return TRUE;
}

void WIN_InitMonitorList()
{
    vk_wv.monitor = 0;
	vk_wv.primaryMonitor = 0;
	vk_wv.monitorCount = 0;

	EnumDisplayMonitors( NULL, NULL, &WIN_MonitorEnumCallback, 0 );

	const POINT zero = { 0, 0 };
	const HMONITOR hMonitor = MonitorFromPoint( zero, MONITOR_DEFAULTTOPRIMARY );
	for ( int i = 0; i < vk_wv.monitorCount; i++ ) {
		if ( hMonitor ==  vk_wv.hMonitors[i] ) {
			vk_wv.primaryMonitor = i;
			vk_wv.monitor = i;
			break;
		}
	}
}

void WIN_UpdateMonitorIndexFromCvar()
{
	// r_monitor is the 1-based monitor index, 0 means primary monitor
	// use Cvar_Get to enforce the latched change, if any
	const int monitor = Cvar_Get( "r_monitor", "0", CVAR_ARCHIVE | CVAR_LATCH )->integer;
	if ( monitor <= 0 || monitor > vk_wv.monitorCount ) {
		vk_wv.monitor = vk_wv.primaryMonitor;
		return;
	}

	vk_wv.monitor = Com_ClampInt( 0, vk_wv.monitorCount - 1, monitor - 1 );
}

static const char* VKW_GetCurrentDisplayDeviceName()
{
	static char deviceName[CCHDEVICENAME + 1];

	const HMONITOR hMonitor = vk_wv.hMonitors[vk_wv.monitor];
	if ( hMonitor == NULL )
		return NULL;

	MONITORINFOEXA info;
	ZeroMemory( &info, sizeof(info) );
	info.cbSize = sizeof(info);
	if ( GetMonitorInfoA(hMonitor, &info) == 0 )
		return NULL;

	Q_strncpyz( deviceName, info.szDevice, sizeof(deviceName) );
	
	return deviceName;
}


static void VKW_UpdateMonitorRect( const char* deviceName )
{
	if ( deviceName == NULL )
		return;

	DEVMODEA dm;
	ZeroMemory( &dm, sizeof(dm) );
	dm.dmSize = sizeof(dm);
	if ( EnumDisplaySettingsExA(deviceName, ENUM_CURRENT_SETTINGS, &dm, 0) == 0 )
		return;

	if ( dm.dmPelsWidth == 0 || dm.dmPelsHeight == 0 )
		return;

	// Normally, we should check dm.dmFields for the following flags:
	// DM_POSITION DM_PELSWIDTH DM_PELSHEIGHT
	// EnumDisplaySettingsExA doesn't always set up the flags properly.

	RECT rect = vk_wv.monitorRects[vk_wv.monitor];
	rect.left = dm.dmPosition.x;
	rect.top = dm.dmPosition.y;
	rect.right = dm.dmPosition.x + dm.dmPelsWidth;
	rect.bottom = dm.dmPosition.y + dm.dmPelsHeight;
}


static qbool VKW_SetDisplaySettings( DEVMODE dm )
{
	const char* deviceName = VKW_GetCurrentDisplayDeviceName();
	const int ec = ChangeDisplaySettingsExA( deviceName, &dm, NULL, CDS_FULLSCREEN, NULL );
	if ( ec == DISP_CHANGE_SUCCESSFUL )
	{
		vkw_state.cdsDevMode = dm;
		vkw_state.cdsDevModeValid = qtrue;
		VKW_UpdateMonitorRect( deviceName );
		return qtrue;
	}

	vkw_state.cdsDevModeValid = qfalse;

	ri.Printf( PRINT_ALL, "...CDS: %ix%i (C%i) failed: ", (int)dm.dmPelsWidth, (int)dm.dmPelsHeight, (int)dm.dmBitsPerPel );

#define CDS_ERROR(x) case x: ri.Printf( PRINT_ALL, #x"\n" ); break;
	switch (ec) {
		default:
			ri.Printf( PRINT_ALL, "unknown error %d\n", ec );
			break;
		CDS_ERROR( DISP_CHANGE_RESTART );
		CDS_ERROR( DISP_CHANGE_BADPARAM );
		CDS_ERROR( DISP_CHANGE_BADFLAGS );
		CDS_ERROR( DISP_CHANGE_FAILED );
		CDS_ERROR( DISP_CHANGE_BADMODE );
		CDS_ERROR( DISP_CHANGE_NOTUPDATED );
	}
//#undef CDS_ERROR

	return qfalse;
}


static void VKW_ResetDisplaySettings( qbool invalidate )
{
	const char* deviceName = VKW_GetCurrentDisplayDeviceName();
	ChangeDisplaySettingsEx( deviceName, NULL, NULL, 0, NULL );
	VKW_UpdateMonitorRect( deviceName );
	if ( invalidate )
		vkw_state.cdsDevModeValid = qfalse;
}


static qbool VKW_CreateWindow()
{
	static qbool s_classRegistered = qfalse;

	if ( !s_classRegistered )
	{
		WNDCLASS wc;
		memset( &wc, 0, sizeof( wc ) );

		wc.style         = CS_OWNDC;
		wc.lpfnWndProc   = MainWndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = vk_wv.hInstance;
		wc.hIcon         = LoadIcon( vk_wv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
		wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName  = 0;
		wc.lpszClassName = CLIENT_WINDOW_TITLE"-vk";

		if ( !RegisterClass( &wc ) )
			ri.Error( ERR_FATAL, "VKW_CreateWindow: could not register window class" );

		s_classRegistered = qtrue;
		ri.Printf( PRINT_DEVELOPER, "...registered window class\n" );
	}

	//
	// create the HWND if one does not already exist
	//
	if ( !vk_wv.hWnd )
	{
		vk_wv.inputInitialized = qfalse;

		RECT r;
		r.left = 0;
		r.top = 0;
		r.right  = glInfo.winWidth;
		r.bottom = glInfo.winHeight;

		int style = WS_VISIBLE | WS_CLIPCHILDREN;
		int exstyle = 0;

		if ( glInfo.winFullscreen )
		{
			style |= WS_POPUP;
			exstyle |= WS_EX_TOPMOST;
		}
		else
		{
			style |= WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
			AdjustWindowRect( &r, style, FALSE );
		}

		const int w = r.right - r.left;
		const int h = r.bottom - r.top;

		const RECT monRect = vk_wv.monitorRects[vk_wv.monitor];

		int dx = 0;
		int dy = 0;

		if ( !glInfo.winFullscreen )
		{
			dx = ri.Cvar_Get( "vid_xpos", "0", 0 )->integer;
			dy = ri.Cvar_Get( "vid_ypos", "0", 0 )->integer;
			dx = Com_ClampInt( 0, max( 0, monRect.right - monRect.left - w ), dx );
			dy = Com_ClampInt( 0, max( 0, monRect.bottom - monRect.top - h ), dy );
		}

		const int x = monRect.left + dx;
		const int y = monRect.top + dy;

		vk_wv.duringCreateWindow = qtrue;
		vk_wv.hWnd = CreateWindowEx( exstyle, CLIENT_WINDOW_TITLE"-vk", " " CLIENT_WINDOW_TITLE"-vk", style,
				x, y, w, h, NULL, NULL, vk_wv.hInstance, NULL );
		vk_wv.duringCreateWindow = qfalse;

		if ( !vk_wv.hWnd )
			ri.Error( ERR_FATAL, "VKW_CreateWindow() - Couldn't create window" );

		ShowWindow( vk_wv.hWnd, SW_SHOW );
		UpdateWindow( vk_wv.hWnd );
		ri.Printf( PRINT_DEVELOPER, "...created window@%d,%d (%dx%d)\n", x, y, w, h );
	}
	else
	{
		ri.Printf( PRINT_DEVELOPER, "...window already present, CreateWindowEx skipped\n" );
	}

	glConfig.colorBits = 32;
	glConfig.depthBits = 24;
	glConfig.stencilBits = 8;

	SetForegroundWindow( vk_wv.hWnd );
	SetFocus( vk_wv.hWnd );

	return qtrue;
}

// @TODO: use it somewhere like before??? :p
static qbool VKW_SetMode()
{
	WIN_InitMonitorList();
	WIN_UpdateMonitorIndexFromCvar();

	const RECT monRect = vk_wv.monitorRects[vk_wv.monitor];
	const int desktopWidth = (int)(monRect.right - monRect.left);
	const int desktopHeight = (int)(monRect.bottom - monRect.top);
	re.ConfigureVideoMode( desktopWidth, desktopHeight );

	DEVMODE dm;
	ZeroMemory( &dm, sizeof( dm ) );
	dm.dmSize = sizeof( dm );

	if (glInfo.vidFullscreen != vkw_state.cdsDevModeValid) {
		if (glInfo.vidFullscreen) {
			dm.dmPelsWidth  = glConfig.vidWidth;
			dm.dmPelsHeight = glConfig.vidHeight;
			dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

			if ( r_displayRefresh->integer ) {
				dm.dmDisplayFrequency = r_displayRefresh->integer;
				dm.dmFields |= DM_DISPLAYFREQUENCY;
			}

			dm.dmBitsPerPel = 32;
			dm.dmFields |= DM_BITSPERPEL;

			dm.dmPosition.x = monRect.left;
			dm.dmPosition.y = monRect.top;
			dm.dmFields |= DM_POSITION;

			glInfo.vidFullscreen = VKW_SetDisplaySettings( dm );
		}
		else
		{
			VKW_ResetDisplaySettings( qtrue );
		}
	}

	if (!VKW_CreateWindow())
		return qfalse;

	if (EnumDisplaySettingsA( VKW_GetCurrentDisplayDeviceName(), ENUM_CURRENT_SETTINGS, &dm ))
		glInfo.displayFrequency = dm.dmDisplayFrequency;

	return qtrue;
}


uint64_t Sys_Vulkan_Init( void* vkInstance )
{
	if(!VKW_SetMode())
	{
	    Com_Error(ERR_FATAL, "VKW_SetMode failed\n");
	}

	VkSurfaceKHR surface;

	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = vk_wv.hWnd;
	createInfo.hinstance = GetModuleHandle(NULL);
	if(vkCreateWin32SurfaceKHR((VkInstance)vkInstance, &createInfo, NULL, &surface) != VK_SUCCESS)
	{
		ri.Error(ERR_FATAL, "vkCreateWin32SurfaceKHR failed\n");
	}

	return (uint64_t)surface;
}

