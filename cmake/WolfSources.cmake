#-----------------------------------------------------------------
# Sources
#-----------------------------------------------------------------

FILE(GLOB COMMON_SRC
	"src/qcommon/*.c"
	"src/qcommon/*.h"
)

FILE(GLOB QCOMMON
	"src/game/q_shared.c"
	"src/game/q_shared.h"
	"src/game/q_math.c"
)

if(UNIX)
	FILE(GLOB COMMON_SRC_REMOVE
		"src/qcommon/vm_x86.c"
	)
	FILE(GLOB SDL_SRC
		"src/unix/sdl_*.c"
	)
endif()

LIST(REMOVE_ITEM COMMON_SRC ${COMMON_SRC_REMOVE})

# Platform specific code for server and client
if(UNIX)
	LIST(APPEND PLATFORM_SHARED_SRC "src/unix/unix_main.c")
	LIST(APPEND PLATFORM_SHARED_SRC "src/unix/unix_net.c")
	LIST(APPEND PLATFORM_SHARED_SRC "src/unix/unix_shared.c")
	LIST(APPEND PLATFORM_SHARED_SRC "src/unix/linux_common.c")
	LIST(APPEND PLATFORM_SHARED_SRC "src/unix/linux_signals.c")
	LIST(APPEND PLATFORM_SHARED_SRC "src/unix/snapvector.nasm")
	LIST(APPEND PLATFORM_CLIENT_SRC ${SDL_SRC})
	LIST(APPEND PLATFORM_CLIENT_SRC "src/unix/linux_qgl.c")
elseif(WIN32)
	LIST(APPEND PLATFORM_SHARED_SRC "src/win32/win_syscon.c")
	LIST(APPEND PLATFORM_SHARED_SRC "src/win32/win_shared.c")
	LIST(APPEND PLATFORM_SHARED_SRC "src/win32/win_main.c")
	LIST(APPEND PLATFORM_SHARED_SRC "src/win32/win_net.c")
	LIST(APPEND PLATFORM_SHARED_SRC "src/win32/win_wndproc.c")
	LIST(APPEND PLATFORM_CLIENT_SRC "src/win32/win_snd.c")
	LIST(APPEND PLATFORM_CLIENT_SRC "src/win32/win_qgl.c")
	LIST(APPEND PLATFORM_CLIENT_SRC "src/win32/win_input.c")
	LIST(APPEND PLATFORM_CLIENT_SRC "src/win32/win_glimp.c")
	LIST(APPEND PLATFORM_CLIENT_SRC "src/win32/win_vkimp.c")
	LIST(APPEND PLATFORM_CLIENT_SRC "src/win32/win_gamma.c")
	LIST(APPEND PLATFORM_SHARED_SRC "src/win32/winquake.rc")
endif()

#if(FEATURE_RENDERER_VULKAN)
#	LIST(REMOVE_ITEM PLATFORM_CLIENT_SRC
#		"src/win32/win_glimp.c"
#		"src/win32/win_qgl.c"
#		"src/win32/win_gamma.c"
#	)
#endif()
if(FEATURE_RENDERER_GL1)
	LIST(REMOVE_ITEM PLATFORM_CLIENT_SRC
		"src/win32/win_vkimp.c"
	)
endif()

FILE(GLOB SERVER_SRC
	"src/server/*.c"
	"src/server/*.h"
	"src/null/*.c"
	"src/null/*.h"
	"src/botlib/be*.c"
	"src/botlib/be*.h"
	"src/botlib/l_*.c"
	"src/botlib/l_*.h"
)

LIST(APPEND SERVER_SRC ${QCOMMON})

FILE(GLOB CLIENT_SRC
	"src/server/*.c"
	"src/server/*.h"
	"src/client/*.c"
	"src/client/*.h"
	"src/botlib/be*.c"
	"src/botlib/be*.h"
	"src/botlib/l_*.c"
	"src/botlib/l_*.h"
)

LIST(APPEND CLIENT_SRC ${QCOMMON})

# These files are shared with the CGAME from the UI library
FILE(GLOB UI_SHARED
	"src/ui/ui_shared.c"
	"src/ui/ui_parse.c"
	"src/ui/ui_script.c"
	"src/ui/ui_menu.c"
	"src/ui/ui_menuitem.c"
)

FILE(GLOB CGAME_SRC
	"src/cgame/*.c"
	"src/cgame/*.h"
	"src/game/q_math.c"
	"src/game/q_shared.c"
	"src/game/bg_*.c"
	"src/cgame/cgame.def"
)

LIST(APPEND CGAME_SRC ${UI_SHARED})

FILE(GLOB QAGAME_SRC
	"src/game/*.c"
	"src/game/*.h"
	"src/game/q_math.c"
	"src/game/q_shared.c"
	"src/botai/*.c"
	"src/botai/*.h"
	"src/game/game.def"
)


FILE(GLOB UI_SRC
	"src/ui/*.c"
	"src/ui/*.h"
    "src/ui/lib/*.c"
    "src/ui/lib/*.h"
	"src/game/q_math.c"
	"src/game/q_shared.c"
	"src/game/bg_classes.c"
	"src/game/bg_misc.c"
	"src/ui/ui.def"
)

FILE(GLOB CLIENT_FILES
	"src/client/*.c"
)

FILE(GLOB SERVER_FILES
	"src/server/*.c"
)

FILE(GLOB SYSTEM_FILES
	"src/sys/sys_main.c"
	"src/sys/con_log.c"
)

FILE(GLOB BOTLIB_FILES
	"src/botlib/be*.c"
	"src/botlib/l_*.c"
)

FILE(GLOB SPLINES_FILES
	"src/splines/*.cpp"
	"src/splines/*.h"
)

FILE(GLOB JPEG_FILES
	"src/jpeg-6/*.c"
	"src/jpeg-6/*.h"
)

FILE(GLOB RENDERER_COMMON
	"src/game/q_shared.h"
	"src/renderer_common/*.h"
)

FILE(GLOB RENDERER_FILES
	"src/renderer_gl/*.c"
	"src/renderer_gl/*.h"
)

FILE(GLOB RENDERER_VK_FILES
	"src/renderer_vk/*.c"
	"src/renderer_vk/*.h"
)
LIST(REMOVE_ITEM RENDERER_COMMON "src/renderer_vk/qgl*")


FILE(GLOB RENDERER_VK_VMA_FILES
	"src/renderer_vk/vk_vma_alloc.cpp"
)

