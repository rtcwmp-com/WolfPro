#-----------------------------------------------------------------
# Build Client
#-----------------------------------------------------------------

set(WOLF_OUTPUT_DIR "")


function(setup_client wolfmp_target IS_VULKAN)

if(IS_VULKAN)
	set(CLIENT_SRC ${CLIENT_SRC_VK})
	LIST(APPEND WOLF_COMPILE_DEF "RTCW_VULKAN")
else()
	set(CLIENT_SRC ${CLIENT_SRC_GL})
endif()

#todo add debug macro in debug build
if(WIN32)
	set(CMAKE_C_FLAGS_DEBUG "/DEBUG /Zi")
	set(CMAKE_C_FLAGS_RELEASE "/Zi")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DEBUG /OPT:REF /OPT:ICF")
	add_executable(${wolfmp_target} WIN32 ${COMMON_SRC} ${CLIENT_SRC})
	if(CMAKE_BUILD_TYPE MATCHES "Debug")
			set_property(TARGET ${wolfmp_target} PROPERTY
             MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
	else()
			 set_property(TARGET ${wolfmp_target} PROPERTY
             MSVC_RUNTIME_LIBRARY "MultiThreaded")
	endif()
	set_property(TARGET ${wolfmp_target} PROPERTY VS_STARTUP_PROJECT INSTALL)
	
else()
	add_executable(${wolfmp_target} ${COMMON_SRC} ${CLIENT_SRC})
endif()


if(IS_VULKAN)
	target_link_libraries(${wolfmp_target}
		client_libraries_vk
		engine_libraries
		os_libraries
	)
	target_link_libraries(${wolfmp_target} cimgui)
else()
	target_link_libraries(${wolfmp_target}
	client_libraries_gl
	engine_libraries
	os_libraries
	)
endif()

message(STATUS CMAKE_BUILD_TYPE)


message(STATUS "Wolfmp Compile defs: " ${WOLF_COMPILE_DEF})
set_target_properties(${wolfmp_target} PROPERTIES
	COMPILE_DEFINITIONS "${WOLF_COMPILE_DEF}"
	RUNTIME_OUTPUT_DIRECTORY "${WOLF_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${WOLF_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${WOLF_OUTPUT_DIR}"
)

if((UNIX) AND NOT APPLE AND NOT ANDROID)
	set_target_properties(${wolfmp_target} PROPERTIES SUFFIX "${BIN_SUFFIX}")
endif()

target_compile_definitions(${wolfmp_target} PRIVATE WOLF_CLIENT=1)

if(MSVC AND NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/wolfmp.vcxproj.user)
	configure_file(${PROJECT_SOURCE_DIR}/cmake/vs2013.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/wolfmp.vcxproj.user @ONLY)
endif()

install(TARGETS ${wolfmp_target}
	BUNDLE  DESTINATION "${INSTALL_DEFAULT_BASEDIR}"
	RUNTIME DESTINATION "${INSTALL_DEFAULT_BASEDIR}"
)

install(FILES $<TARGET_PDB_FILE:${wolfmp_target}> DESTINATION ${INSTALL_DEFAULT_BASEDIR} OPTIONAL)
install(FILES $<TARGET_PDB_FILE:wolfded> DESTINATION ${INSTALL_DEFAULT_BASEDIR} OPTIONAL)
install(FILES $<TARGET_PDB_FILE:qagame> DESTINATION ${INSTALL_DEFAULT_BASEDIR}/${MODNAME} OPTIONAL)
install(FILES $<TARGET_PDB_FILE:ui> DESTINATION ${INSTALL_DEFAULT_BASEDIR}/${MODNAME} OPTIONAL)
install(FILES $<TARGET_PDB_FILE:cgame> DESTINATION ${INSTALL_DEFAULT_BASEDIR}/${MODNAME} OPTIONAL)
endfunction()

setup_client(wolfmp_gl FALSE)
setup_client(wolfmp_vk TRUE)