#-----------------------------------------------------------------
# Build Client
#-----------------------------------------------------------------

set(WOLF_OUTPUT_DIR "")

#todo add debug macro in debug build
if(WIN32)
	set(CMAKE_C_FLAGS_DEBUG "/DEBUG /Zi")
	set(CMAKE_C_FLAGS_RELEASE "/Zi")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DEBUG /OPT:REF /OPT:ICF")
	add_executable(wolfmp WIN32 ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SHARED_SRC} ${PLATFORM_CLIENT_SRC})
	if(CMAKE_BUILD_TYPE MATCHES "Debug")
			set_property(TARGET wolfmp PROPERTY
             MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
	else()
			 set_property(TARGET wolfmp PROPERTY
             MSVC_RUNTIME_LIBRARY "MultiThreaded")
	endif()
	set_property(TARGET wolfmp PROPERTY VS_STARTUP_PROJECT INSTALL)
	
else()
	add_executable(wolfmp ${COMMON_SRC} ${CLIENT_SRC} ${PLATFORM_SHARED_SRC} ${PLATFORM_CLIENT_SRC})
endif()

target_link_libraries(wolfmp
	client_libraries
	engine_libraries
	os_libraries
)
if(FEATURE_RENDERER_VULKAN)
	target_link_libraries(wolfmp cimgui)
endif()

message(STATUS CMAKE_BUILD_TYPE)


message(STATUS "Wolfmp Compile defs: " ${WOLF_COMPILE_DEF})
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
	BUNDLE  DESTINATION "${INSTALL_DEFAULT_BASEDIR}"
	RUNTIME DESTINATION "${INSTALL_DEFAULT_BASEDIR}"
)

install(FILES $<TARGET_PDB_FILE:wolfmp> DESTINATION ${INSTALL_DEFAULT_BASEDIR} OPTIONAL)
install(FILES $<TARGET_PDB_FILE:wolfded> DESTINATION ${INSTALL_DEFAULT_BASEDIR} OPTIONAL)
install(FILES $<TARGET_PDB_FILE:qagame> DESTINATION ${INSTALL_DEFAULT_BASEDIR}/${MODNAME} OPTIONAL)
install(FILES $<TARGET_PDB_FILE:ui> DESTINATION ${INSTALL_DEFAULT_BASEDIR}/${MODNAME} OPTIONAL)
install(FILES $<TARGET_PDB_FILE:cgame> DESTINATION ${INSTALL_DEFAULT_BASEDIR}/${MODNAME} OPTIONAL)