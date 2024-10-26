#-----------------------------------------------------------------
# Build Renderer
#-----------------------------------------------------------------

add_library(renderer STATIC ${RENDERER_FILES} ${RENDERER_COMMON})

target_link_libraries(renderer renderer_gl1_libraries renderer_libraries)
target_include_directories(renderer PRIVATE src/renderer)

if(NOT MSVC)
	target_link_libraries(renderer m)
endif()

target_link_libraries(client_libraries INTERFACE renderer)

#-----------------------------------------------------------------
# Build JPEG
#-----------------------------------------------------------------

find_package(JPEGTURBO)
if(JPEGTURBO_FOUND)
	target_link_libraries(renderer_libraries INTERFACE ${JPEG_LIBRARIES})
	target_include_directories(renderer_libraries INTERFACE ${JPEG_INCLUDE_DIR})
	# Check for libjpeg-turbo v1.3
	include(CheckFunctionExists)
	set(CMAKE_REQUIRED_INCLUDES ${JPEG_INCLUDE_DIR})
	set(CMAKE_REQUIRED_LIBRARIES ${JPEG_LIBRARY})
	# FIXME: function is checked, but HAVE_JPEG_MEM_SRC is empty. Why?
	check_function_exists("jpeg_mem_src" HAVE_JPEG_MEM_SRC)
endif()
