#-----------------------------------------------------------------
# Build Client
#-----------------------------------------------------------------

set(WOLF_OUTPUT_DIR "")

if(WIN32)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /DEBUG /Zi")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
	add_executable(wolfmp WIN32 ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SHARED_SRC} ${PLATFORM_CLIENT_SRC})
else()
	add_executable(wolfmp ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SHARED_SRC} ${PLATFORM_CLIENT_SRC})
endif()

target_link_libraries(wolfmp
	client_libraries
	engine_libraries
	os_libraries
)

if(FEATURE_WINDOWS_CONSOLE AND WIN32)
	set(WOLF_COMPILE_DEF "USE_ICON;USE_WINDOWS_CONSOLE;BOTLIB;WIN32")
	message("FEATURE_WINDOWS_CONSOLE AND WIN32")
else()
	if(CMAKE_CROSSCOMPILE)
		set(CMAKE_RC_FLAGS "/I ${CMAKE_SOURCE_DIR}/deps/xwin/sdk/include/um /I ${CMAKE_SOURCE_DIR}/deps/xwin/sdk/include/shared")
		message("CMAKE_CROSSCOMPILE")
	endif()
	set(WOLF_COMPILE_DEF "USE_ICON;BOTLIB;__i386__")
endif()

set_target_properties(wolfmp PROPERTIES
	COMPILE_DEFINITIONS "${WOLF_COMPILE_DEF}"
	RUNTIME_OUTPUT_DIRECTORY "${WOLF_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${WOLF_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${WOLF_OUTPUT_DIR}"
)

if((UNIX) AND NOT APPLE AND NOT ANDROID)
	set_target_properties(wolfmp PROPERTIES SUFFIX "${BIN_SUFFIX}")
endif()

target_compile_definitions(wolfmp PRIVATE WOLF_CLIENT=1)

if(MSVC AND NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/wolfmp.vcxproj.user)
	configure_file(${PROJECT_SOURCE_DIR}/cmake/vs2013.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/wolfmp.vcxproj.user @ONLY)
endif()

install(TARGETS wolfmp
	BUNDLE  DESTINATION "${INSTALL_DEFAULT_BINDIR}"
	RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}"
)
