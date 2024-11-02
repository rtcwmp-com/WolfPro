#-----------------------------------------------------------------
# Platform
#-----------------------------------------------------------------

# Used to store real system processor when we overwrite CMAKE_SYSTEM_PROCESSOR for cross-compile builds
set(WOLF_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})

# has to be set to "", otherwise CMake will pass -rdynamic resulting in a client crash
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

# How many architectures are we building
set(WOLF_ARCH_COUNT 1)

add_library(os_libraries INTERFACE)

# Color diagnostics for build systems other than make
if(UNIX)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=always")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
	endif()
endif()

if(UNIX)
	# optimization/debug flags
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ffast-math")
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ggdb")
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__extern_always_inline=inline")
	endif()
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")

	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffast-math")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

	if(ENABLE_ASAN)
		include (CheckCCompilerFlag)
		include (CheckCXXCompilerFlag)
		set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
		CHECK_C_COMPILER_FLAG("-fsanitize=address" HAVE_FLAG_SANITIZE_ADDRESS_C)
		CHECK_CXX_COMPILER_FLAG("-fsanitize=address" HAVE_FLAG_SANITIZE_ADDRESS_CXX)

		if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
			# Clang requires an external symbolizer program.
			FIND_PROGRAM(LLVM_SYMBOLIZER
					NAMES llvm-symbolizer
					llvm-symbolizer-3.8
					llvm-symbolizer-3.7
					llvm-symbolizer-3.6)

			if(NOT LLVM_SYMBOLIZER)
				message(WARNING "AddressSanitizer failed to locate an llvm-symbolizer program. Stack traces may lack symbols.")
			endif()
		endif()

		if(HAVE_FLAG_SANITIZE_ADDRESS_C AND HAVE_FLAG_SANITIZE_ADDRESS_CXX)
			message(STATUS "Enabling AddressSanitizer for this configuration")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
		else()
			message(FATAL_ERROR "AddressSanitizer enabled but compiler doesn't support it - cannot continue.")
		endif()
	endif()

	
	target_link_libraries(os_libraries INTERFACE  m ${CMAKE_DL_LIBS} rt pthread)
	set(LIB_SUFFIX ".mp.")
	

elseif(WIN32)
	target_compile_definitions(shared_libraries INTERFACE WINVER=0x601)

	if(WOLF_WIN64)
		target_compile_definitions(shared_libraries INTERFACE C_ONLY)
	endif()

	target_link_libraries(os_libraries INTERFACE wsock32 ws2_32 psapi winmm)

	if(BUNDLED_SDL)
		# Libraries for Win32 native and MinGW required by static SDL2 build
		target_link_libraries(os_libraries INTERFACE user32 gdi32 imm32 ole32 oleaut32 version uuid hid setupapi)
	endif()
	set(LIB_SUFFIX "_mp_")
	if(MSVC)

		message(STATUS "MSVC version: ${MSVC_VERSION}")

		if(ENABLE_ASAN)
			# Dont know the exact version but its somewhere there
			if(MSVC_VERSION LESS 1925)
				message(FATAL_ERROR "AddressSanitizer enabled but compiler doesn't support it - cannot continue.")
			endif()

			message(STATUS "Enabling AddressSanitizer for this configuration")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
		endif()

		# is using cl.exe the __FILE__ macro will not contain the full path to the file by default.
		# enable the full path with /FC (we use our own WOLF_FILENAME to cut the path to project relative path)
		target_compile_options(shared_libraries INTERFACE /FC)

		if(FORCE_STATIC_VCRT)
			set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /EHsc /O2")
			set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /EHa /W3")

			set(CompilerFlags
				CMAKE_CXX_FLAGS
				CMAKE_CXX_FLAGS_DEBUG
				CMAKE_CXX_FLAGS_RELEASE
				CMAKE_C_FLAGS
				CMAKE_C_FLAGS_DEBUG
				CMAKE_C_FLAGS_RELEASE
			)

			foreach(CompilerFlag ${CompilerFlags})
				string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
			endforeach()

			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT.lib /NODEFAULTLIB:MSVCRTD.lib")
			set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT.lib /NODEFAULTLIB:MSVCRTD.lib")
		endif()

		# Should we always use this?
		# add_definitions(-DC_ONLY)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS) # Do not show CRT warnings
	endif(MSVC)

	if(MINGW)

		# This is not yet supported, but most likely will happen in the future.
		if(ENABLE_ASAN)
			include (CheckCCompilerFlag)
			include (CheckCXXCompilerFlag)
			set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
			CHECK_C_COMPILER_FLAG("-fsanitize=address" HAVE_FLAG_SANITIZE_ADDRESS_C)
			CHECK_CXX_COMPILER_FLAG("-fsanitize=address" HAVE_FLAG_SANITIZE_ADDRESS_CXX)

			if(HAVE_FLAG_SANITIZE_ADDRESS_C AND HAVE_FLAG_SANITIZE_ADDRESS_CXX)
				message(STATUS "Enabling AddressSanitizer for this configuration")
				add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
			else()
				message(FATAL_ERROR "AddressSanitizer enabled but compiler doesn't support it - cannot continue.")
			endif()
		endif()

		if(NOT DEBUG_BUILD)
			set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -static-libgcc")
			set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -static-libgcc -static-libstdc++")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
			set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -s")
			set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static-libgcc -liconv -s")
			set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++ -liconv -s")
			add_definitions(-D_WIN32_IE=0x0501)
		endif()

	endif()
endif()

# Get the system architecture
# Not that this is NOT APPLE because in macos we bundle everything into a single file
if(NOT APPLE)
	if(WOLF_X86 AND WOLF_32BITS)
		if(WIN32)
			set(ARCH "x86")
		else()
			set(ARCH "i386")
		endif()
		set(BIN_DESCRIBE "(x86 32-bit)")
	elseif(WOLF_X86 AND WOLF_64BITS)
		if(WIN32)
			set(ARCH "x64")
		else()
			set(ARCH "x86_64")
		endif()
		set(BIN_DESCRIBE "(x86 64-bit)")
	elseif(WOLF_ARM AND WOLF_64BITS)
		if(NOT ANDROID)
			set(ARCH "aarch64")
		else()
			set(ARCH "arm64-v8a")
		endif()
	else()
		set(ARCH "${CMAKE_SYSTEM_PROCESSOR}")
		message(STATUS "Warning: processor architecture not recognised (${CMAKE_SYSTEM_PROCESSOR})")
	endif()

	if (ENABLE_MULTI_BUILD)
		set(BIN_SUFFIX ".${ARCH}")
	endif()

	# default to a sane value
	if(NOT DEFINED BIN_DESCRIBE OR "${BIN_DESCRIBE}" STREQUAL "")
		set(BIN_DESCRIBE "(${ARCH})")
	endif()
endif()

if(UNIX AND WOLF_64BITS)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# summary
message(STATUS "System: ${CMAKE_SYSTEM} (${WOLF_SYSTEM_PROCESSOR})")
message(STATUS "Lib arch: ${CMAKE_LIBRARY_ARCHITECTURE}")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "Install path:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Compiler flags:")
message(STATUS "- C             ${CMAKE_C_FLAGS}")
message(STATUS "- C++           ${CMAKE_CXX_FLAGS}")
message(STATUS "- PIC           ${CMAKE_POSITION_INDEPENDENT_CODE}")
message(STATUS "Linker flags:")
message(STATUS "- Executable    ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "- Module        ${CMAKE_MODULE_LINKER_FLAGS}")
message(STATUS "- Shared        ${CMAKE_SHARED_LINKER_FLAGS}")
