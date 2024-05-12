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
# Build Splines
#-----------------------------------------------------------------

add_library(splines STATIC ${SPLINES_FILES})
target_link_libraries(client_libraries INTERFACE splines)
target_include_directories(splines PRIVATE src/splines)

#-----------------------------------------------------------------
# Build JPEG
#-----------------------------------------------------------------

add_library(jpeg6 STATIC ${JPEG_FILES})
target_link_libraries(client_libraries INTERFACE jpeg6)
target_include_directories(jpeg6 PRIVATE src/jpeg-6)