#-----------------------------------------------------------------
# Setup Features
#-----------------------------------------------------------------

# Helper to src folder
set(SRC "${PROJECT_SOURCE_DIR}/src")

if(FEATURE_WINDOWS_CONSOLE AND WIN32)
	set(WOLF_COMPILE_DEF "USE_ICON;USE_WINDOWS_CONSOLE;BOTLIB;WIN32")
	message("FEATURE_WINDOWS_CONSOLE AND WIN32")
elseif(CMAKE_CROSSCOMPILE)
	set(CMAKE_RC_FLAGS "/I ${CMAKE_SOURCE_DIR}/deps/xwin/sdk/include/um /I ${CMAKE_SOURCE_DIR}/deps/xwin/sdk/include/shared")
	message("CMAKE_CROSSCOMPILE")
	set(WOLF_COMPILE_DEF "USE_ICON;BOTLIB;__i386__")
else()
	set(WOLF_COMPILE_DEF "USE_ICON;BOTLIB;__i386__")
	if(FEATURE_RENDERER_VULKAN)
		LIST(APPEND WOLF_COMPILE_DEF "RTCW_VULKAN")
	endif()
	if(CMAKE_BUILD_TYPE MATCHES "Debug")
		message(STATUS "Using DEBUG")
		LIST(APPEND WOLF_COMPILE_DEF "_DEBUG;DEBUG")
	else()
		message(STATUS "Using NDEBUG")
		LIST(APPEND WOLF_COMPILE_DEF "NDEBUG")
	endif()
		
endif()

#-----------------------------------------------------------------
# Client features
#-----------------------------------------------------------------
if(BUILD_CLIENT)
	if(FEATURE_RENDERER_GL1)
		# ghost target to link all opengl renderer libraries
		add_library(opengl_renderer_libs INTERFACE)

		if(CLIENT_GLVND)
			message(STATUS "Using GLVND instead of legacy GL library")
			set(OpenGL_GL_PREFERENCE GLVND)
		else()
			message(STATUS "Using legacy OpenGL instead of GLVND")
			set(OpenGL_GL_PREFERENCE LEGACY)
		endif ()
		find_package(OpenGL REQUIRED COMPONENTS OpenGL)
		target_link_libraries(opengl_renderer_libs INTERFACE OpenGL::GL)
		target_include_directories(opengl_renderer_libs INTERFACE ${OPENGL_INCLUDE_DIR})

		target_link_libraries(renderer_gl1_libraries INTERFACE opengl_renderer_libs)
	endif()

	if(FEATURE_RENDERER_VULKAN)
		# ghost target to link all opengl renderer libraries
		add_library(vk_renderer_libs INTERFACE)

		message(STATUS "Using Vulkan instead of legacy GL library")

		find_package(Vulkan REQUIRED)
		target_link_libraries(vk_renderer_libs INTERFACE ${Vulkan_LIBRARIES})
		target_include_directories(vk_renderer_libs INTERFACE ${Vulkan_INCLUDE_DIR})
	
		target_link_libraries(renderer_vk_libraries INTERFACE vk_renderer_libs)
	endif()

	if(UNIX)
		find_package(SDL2 REQUIRED)
		target_link_libraries(client_libraries INTERFACE  SDL2::SDL2)
	endif()

endif()


#-----------------------------------------------------------------
# Mod features
#-----------------------------------------------------------------
if(BUILD_MOD)

endif(BUILD_MOD)

#-----------------------------------------------------------------
# Server/Common features
#-----------------------------------------------------------------
if(BUILD_CLIENT OR BUILD_SERVER)

endif()

