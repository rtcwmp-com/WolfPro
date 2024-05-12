#-----------------------------------------------------------------
# Build Server
#-----------------------------------------------------------------

add_executable(wolfded WIN32 ${COMMON_SRC} ${SERVER_SRC} ${PLATFORM_SHARED_SRC} ${PLATFORM_SERVER_SRC})
target_link_libraries(wolfded
	server_libraries
	engine_libraries
	os_libraries
)

set_target_properties(wolfded
	PROPERTIES COMPILE_DEFINITIONS "DEDICATED;BOTLIB"
	RUNTIME_OUTPUT_DIRECTORY ""
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ""
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ""
)

if((UNIX) AND NOT APPLE AND NOT ANDROID)
	set_target_properties(wolfded PROPERTIES SUFFIX "${BIN_SUFFIX}")
endif()

if(MSVC AND NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/wolfded.vcxproj.user)
	configure_file(${PROJECT_SOURCE_DIR}/cmake/vs2013.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/wolfded.vcxproj.user @ONLY)
endif()

install(TARGETS wolfded RUNTIME DESTINATION "${INSTALL_DEFAULT_BINDIR}")
