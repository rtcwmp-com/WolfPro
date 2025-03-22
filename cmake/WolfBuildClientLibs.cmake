#-----------------------------------------------------------------
# Build Renderer
#-----------------------------------------------------------------

add_library(renderer STATIC ${RENDERER_FILES} ${RENDERER_COMMON})

target_link_libraries(renderer renderer_gl1_libraries renderer_libraries)
target_include_directories(renderer PRIVATE src/renderer)

if(NOT MSVC)
	target_link_libraries(renderer m)
endif()

target_link_libraries(client_libraries_gl INTERFACE renderer)


#-----------------------------------------------------------------
# Build Vulkan Renderer
#-----------------------------------------------------------------

add_library(renderer_vk STATIC ${RENDERER_VK_FILES} ${RENDERER_COMMON})
add_library(vk_vma_alloc STATIC ${RENDERER_VK_VMA_FILES})
target_include_directories(vk_vma_alloc PRIVATE ${Vulkan_INCLUDE_DIR})
add_library(cimgui STATIC ${RENDERER_CIMGUI_FILES})
target_include_directories(cimgui PRIVATE src/cimgui)

target_link_libraries(renderer_vk renderer_vk_libraries renderer_libraries vk_vma_alloc)
target_include_directories(renderer_vk PRIVATE src/renderer_vk)

set_target_properties(renderer_vk PROPERTIES
	COMPILE_DEFINITIONS "${WOLF_COMPILE_DEF}"
	RUNTIME_OUTPUT_DIRECTORY "${WOLF_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${WOLF_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${WOLF_OUTPUT_DIR}"
)
set_target_properties(vk_vma_alloc PROPERTIES
	COMPILE_DEFINITIONS "${WOLF_COMPILE_DEF}"
	RUNTIME_OUTPUT_DIRECTORY "${WOLF_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${WOLF_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${WOLF_OUTPUT_DIR}"
)
message(STATUS "Compile defs: " ${WOLF_COMPILE_DEF})
set_target_properties(cimgui PROPERTIES
	COMPILE_DEFINITIONS "${WOLF_COMPILE_DEF}"
	RUNTIME_OUTPUT_DIRECTORY "${WOLF_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${WOLF_OUTPUT_DIR}"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${WOLF_OUTPUT_DIR}"
)


if(NOT MSVC)
	target_link_libraries(renderer_vk m)
endif()

target_link_libraries(client_libraries_vk INTERFACE renderer_vk)


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
