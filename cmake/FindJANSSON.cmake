# - Find jansson
set(JANSSON_NAMES jansson)
find_path(JANSSON_INCLUDE_DIR jansson.h
	${PROJECT_SOURCE_DIR}/deps/jansson/build/include/
	${PROJECT_SOURCE_DIR}/deps/jansson/build-win/include/
	/usr/include
	/usr/local/include
	/sw/include
	/opt/local/include
	DOC "The directory where jansson.h resides"
)

if(CMAKE_CROSSCOMPILING)
find_library(JANSSON_LIBRARY
	NAMES ${JANSSON_NAMES} libjansson
	PATHS
    ${PROJECT_SOURCE_DIR}/deps/bin
    ${PROJECT_SOURCE_DIR}/deps/jansson/bin
    ${PROJECT_SOURCE_DIR}/deps/jansson/build/lib/Release
	${PROJECT_SOURCE_DIR}/deps/jansson/build/lib
	${PROJECT_SOURCE_DIR}/deps/jansson/build-win
	/usr/lib64
	/usr/lib
	/usr/local/lib64
	/usr/local/lib
	/sw/lib
	/opt/local/lib
	DOC "jansson library"
)
else()
find_library(JANSSON_LIBRARY
	NAMES ${JANSSON_NAMES} libjansson
	PATHS
    ${PROJECT_SOURCE_DIR}/deps/bin
    ${PROJECT_SOURCE_DIR}/deps/jansson/bin
    ${PROJECT_SOURCE_DIR}/deps/jansson/build/lib/Release
	${PROJECT_SOURCE_DIR}/deps/jansson/build/lib
	${PROJECT_SOURCE_DIR}/deps/jansson/build-win
	/usr/lib64
	/usr/lib
	/usr/local/lib64
	/usr/local/lib
	/sw/lib
	/opt/local/lib
	DOC "jansson library"
)
endif()
# Determine curl version
if(JANSSON_INCLUDE_DIR AND EXISTS "${JANSSON_INCLUDE_DIR}/jansson.h")
	file(STRINGS "${JANSSON_INCLUDE_DIR}/jansson.h" jansson_version_str REGEX "^#define JANSSON_VERSION[ ]+[0-9].[0-9].[0-9]")
	string(REGEX REPLACE "^#define JANSSON_VERSION[ ]+([^\"]*).*" "\\1" JANSSON_VERSION_STRING "${jansson_version_str}")
	unset(jansson_version_str)
endif()

# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JANSSON
	REQUIRED_VARS JANSSON_LIBRARY JANSSON_INCLUDE_DIR
	VERSION_VAR JANSSON_VERSION_STRING)

if(JANSSON_FOUND)
	set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
endif()