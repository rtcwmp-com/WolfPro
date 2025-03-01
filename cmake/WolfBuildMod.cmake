#-----------------------------------------------------------------
# Build mod pack
#-----------------------------------------------------------------

# find libm where it exists and link game modules against it
include(CheckLibraryExists)
check_library_exists(m pow "" LIBM)
if(UNIX)
	if(LIBM)
		target_link_libraries(cgame_libraries INTERFACE m)
		target_link_libraries(ui_libraries INTERFACE m)
	endif()
endif(UNIX)

#
# cgame
#
if(BUILD_CLIENT_MOD)
	add_library(cgame MODULE ${CGAME_SRC})
	target_link_libraries(cgame cgame_libraries mod_libraries)

	set_target_properties(cgame
		PROPERTIES
		PREFIX ""
		C_STANDARD 90
		OUTPUT_NAME "cgame${LIB_SUFFIX}${ARCH}"
		LIBRARY_OUTPUT_DIRECTORY "${MODNAME}"
		LIBRARY_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
		LIBRARY_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
	)
	target_compile_definitions(cgame PRIVATE CGAMEDLL=1 MODLIB=1)
	if(MSVC)
		target_link_options(cgame PRIVATE /DEF:cgame.def /DEBUG)
	elseif(MINGW)
		target_sources(cgame PRIVATE src/cgame/cgame.def)
	endif()
endif()

#
# ui
#
if(BUILD_CLIENT_MOD)
	add_library(ui MODULE ${UI_SRC})
	target_link_libraries(ui ui_libraries mod_libraries)

	set_target_properties(ui
		PROPERTIES
		PREFIX ""
		C_STANDARD 90
		OUTPUT_NAME "ui${LIB_SUFFIX}${ARCH}"
		LIBRARY_OUTPUT_DIRECTORY "${MODNAME}"
		LIBRARY_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
		LIBRARY_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
	)
	target_compile_definitions(ui PRIVATE UIDLL=1 MODLIB=1)
	if(MSVC)
		target_link_options(ui PRIVATE /DEF:ui.def /DEBUG)
	elseif(MINGW)
		target_sources(ui PRIVATE src/ui/ui.def)
	endif()
endif()

#
# qagame
#
if(BUILD_SERVER_MOD)
	add_library(qagame MODULE ${QAGAME_SRC})
	target_link_libraries(qagame qagame_libraries mod_libraries)

	set_target_properties(qagame
		PROPERTIES
		PREFIX ""
		C_STANDARD 90
		OUTPUT_NAME "qagame${LIB_SUFFIX}${ARCH}"
		LIBRARY_OUTPUT_DIRECTORY "${MODNAME}"
		LIBRARY_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
		LIBRARY_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY_DEBUG "${MODNAME}"
		RUNTIME_OUTPUT_DIRECTORY_RELEASE "${MODNAME}"
	)
	target_compile_definitions(qagame PRIVATE GAMEDLL=1 MODLIB=1)
	if(MSVC)
		target_link_options(qagame PRIVATE /DEF:qagame.def /DEBUG)
	elseif(MINGW)
		target_sources(qagame PRIVATE src/qagame/qagame.def)
	endif()
	
endif()

# install bins of cgame, ui and qgame
if(BUILD_SERVER_MOD)
	install(TARGETS qagame
		RUNTIME DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
		LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
		ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
	)
endif()

if(NOT BUILD_MOD_PK3 AND BUILD_CLIENT_MOD)
	install(TARGETS cgame ui
		RUNTIME DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
		LIBRARY DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
		ARCHIVE DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
	)
endif()

#
# mod pk3
# Full cross-compile build needs this OFF to update the pk3 with the 2nd build's files
# 
if(BUILD_MOD_PK3)
	# main
	file(GLOB MAIN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/MAIN/*")
	foreach(FILE ${MAIN_FILES})
		file(RELATIVE_PATH REL "${CMAKE_CURRENT_SOURCE_DIR}/MAIN" ${FILE})
		list(APPEND MAIN_FILES_LIST ${REL})
	endforeach()

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_bin.pk3
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/MAIN ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}
		COMMAND ${CMAKE_COMMAND} -E tar c ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_bin.pk3 --format=zip $<TARGET_FILE_NAME:ui> $<TARGET_FILE_NAME:cgame> $<TARGET_FILE_NAME:qagame> ${MAIN_FILES_LIST}
		DEPENDS cgame ui qagame
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/
		VERBATIM
	)

    add_custom_target(mod_pk3 ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_bin.pk3)

	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}/${MODNAME}_bin.pk3
		DESTINATION "${INSTALL_DEFAULT_MODDIR}/${MODNAME}"
	)
endif()