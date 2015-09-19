cmake_minimum_required(VERSION 2.8.5)
if(NOT CMAKE_VERSION VERSION_LESS "3.0")
    cmake_policy(SET CMP0048 OLD)
endif()
if(NOT CMAKE_VERSION VERSION_LESS "3.1")
    cmake_policy(SET CMP0054 OLD)
endif()
if(NOT CMAKE_VERSION VERSION_LESS "3.3")
    cmake_policy(SET CMP0063 NEW)
    set(HONOR_VISIBILITY TRUE)
else()
    set(HONOR_VISIBILITY FALSE)
endif()

#
# Project Definition
#

project(epoxy C)
set(EPOXY_TARGET_NAME epoxy)
set(TARGET_VER 1.3.1_neph-git)
set(TARGET_ABI_VER 0)
set(TARGET_OUTPUT_NAME "${EPOXY_TARGET_NAME}")

#
# CMake Modules
#

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake/modules")
include(CheckCCompilerFlag)
include(CheckIncludeFile)
include(CMakeDependentOption)
include(GNUInstallDirs)
find_package(PythonInterp)
find_package(EGL)
find_package(OpenGL)
find_package(X11)

if (NOT PYTHONINTERP_FOUND)
    message (FATAL_ERROR "Cannot find Python interpreter. This is required for header generation.")
endif()

#
# Include Files
#

include_directories(
	"${CMAKE_CURRENT_BINARY_DIR}/include"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src")
if(EGL_INCLUDE_DIR)
	include_directories("${EGL_INCLUDE_DIR}")
	list(APPEND CMAKE_REQUIRED_INCLUDES "${EGL_INCLUDE_DIR}")
endif()
if(OPENGL_INCLUDE_DIR)
	include_directories("${OPENGL_INCLUDE_DIR}")
	list(APPEND CMAKE_REQUIRED_INCLUDES "${OPENGL_INCLUDE_DIR}")
endif()
if(X11_INCLUDE_DIR)
	include_directories("${X11_INCLUDE_DIR}")
	list(APPEND CMAKE_REQUIRED_INCLUDES "${X11_INCLUDE_DIR}")
endif()
check_include_file("KHR/khrplatform.h" HAVE_KHRPLATFORM_H)

#
# Project Options
#

option(EPOXY_BUILD_SHARED "Build Shared Library" ON)
option(EPOXY_BUILD_STATIC "Build Static Library" ON)
cmake_dependent_option(EPOXY_BUILD_TESTS "Build Epoxy Tests" ON "NOT CMAKE_TOOLCHAIN_FILE" OFF)
cmake_dependent_option(EPOXY_SUPPORT_EGL "Include EGL Support" ON "EGL_INCLUDE_DIR" OFF)
cmake_dependent_option(EPOXY_SUPPORT_GLX "Include GLX Support" ON "OPENGL_INCLUDE_DIR AND X11_INCLUDE_DIR" OFF)
cmake_dependent_option(EPOXY_SUPPORT_WGL "Include WGL Support" ON "WIN32 OR MSYS" OFF)
cmake_dependent_option(EPOXY_INSTALL_KHRPLATFORM "Install Epoxy KHR/khrplatform.h" ON "NOT HAVE_KHRPLATFORM_H" OFF)

set(EPOXY_HEADERS
	"${PROJECT_SOURCE_DIR}/include/epoxy/gl.h"
	"${PROJECT_SOURCE_DIR}/src/dispatch_common.h"
	"${CMAKE_CURRENT_BINARY_DIR}/include/epoxy/gl_generated.h")

set(EPOXY_SOURCE
	"${PROJECT_SOURCE_DIR}/src/dispatch_common.c"
	"${CMAKE_CURRENT_BINARY_DIR}/src/gl_generated_dispatch.c")

if(NOT HAVE_KHRPLATFORM_H)
	include_directories("${PROJECT_SOURCE_DIR}/registry")
	list(APPEND EPOXY_HEADERS
		"${PROJECT_SOURCE_DIR}/registry/KHR/khrplatform.h")
endif()

if(EPOXY_SUPPORT_EGL)
	add_definitions(-DBUILD_EGL)
	list(APPEND EPOXY_HEADERS
		"${PROJECT_SOURCE_DIR}/include/epoxy/egl.h"
		"${CMAKE_CURRENT_BINARY_DIR}/include/epoxy/egl_generated.h")
	list(APPEND EPOXY_SOURCE
		"${PROJECT_SOURCE_DIR}/src/dispatch_egl.c"
		"${CMAKE_CURRENT_BINARY_DIR}/src/egl_generated_dispatch.c")
endif()

if(EPOXY_SUPPORT_GLX)
	add_definitions(-DBUILD_GLX)
	list(APPEND EPOXY_HEADERS
		"${PROJECT_SOURCE_DIR}/include/epoxy/glx.h"
		"${CMAKE_CURRENT_BINARY_DIR}/include/epoxy/glx_generated.h")
	list(APPEND EPOXY_SOURCE
		"${PROJECT_SOURCE_DIR}/src/dispatch_glx.c"
		"${CMAKE_CURRENT_BINARY_DIR}/src/glx_generated_dispatch.c")
endif()

if(EPOXY_SUPPORT_WGL)
	add_definitions(-DBUILD_WGL)
	list(APPEND EPOXY_HEADERS
		"${PROJECT_SOURCE_DIR}/include/epoxy/wgl.h"
		"${CMAKE_CURRENT_BINARY_DIR}/include/epoxy/wgl_generated.h")
	list(APPEND EPOXY_SOURCE
		"${PROJECT_SOURCE_DIR}/src/dispatch_wgl.c"
		"${CMAKE_CURRENT_BINARY_DIR}/src/wgl_generated_dispatch.c")
endif()

#
# Require C99 Standard
#

if(NOT MSVC)
	if(NOT CMAKE_VERSION VERSION_LESS "3.1")
		set(CMAKE_C_STANDARD_REQUIRED TRUE)
		set(CMAKE_C_STANDARD "99")
	else()
		check_c_compiler_flag("-std=gnu99" _COMPILER_HAS_STD_GNU99)
		if(_COMPILER_HAS_STD_GNU99)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
		else()
			check_c_compiler_flag("-std=c99" _COMPILER_HAS_STD_C99)
			if(_COMPILER_HAS_STD_C99)
				set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
			endif()
		endif()
	endif()
endif()

#
# MSVC Runtime DLL
#

cmake_dependent_option(EPOXY_USE_MSVC_RUNTIME "Use MSVC Runtime Library DLL" ON MSVC OFF)
if(MSVC AND NOT EPOXY_USE_MSVC_RUNTIME)
	foreach(flag CMAKE_C_FLAGS
			CMAKE_C_FLAGS_DEBUG
			CMAKE_C_FLAGS_RELEASE
			CMAKE_C_FLAGS_MINSIZEREL
			CMAKE_C_FLAGS_RELWITHDEBINFO)
		string(REGEX REPLACE "/MDd" "/MTd" ${flag} "${${flag}}")
		string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
	endforeach()
endif()

#
# Compiler Flags
#

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
else()
	check_c_compiler_flag("-fno-strict-aliasing" _COMPILER_HAS_NO_STRICT_ALIASING)
	check_c_compiler_flag("-fvisibility=hidden" _COMPILER_HAS_VISIBILITY_HIDDEN)
	check_c_compiler_flag("-Wall" _COMPILER_HAS_WARN_ALL)
	check_c_compiler_flag("-Wextra" _COMPILER_HAS_WARN_EXTRA)
	check_c_compiler_flag("-Wno-unused-parameter" _COMPILER_HAS_NO_WARN_UNUSED_PARAMETER)
	if(_COMPILER_HAS_NO_STRICT_ALIASING)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
	endif()
	if(_COMPILER_HAS_WARN_ALL)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
	endif()
	if(_COMPILER_HAS_WARN_EXTRA)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
	endif()
	if(_COMPILER_HAS_NO_WARN_UNUSED_PARAMETER)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")
	endif()
endif()

#
# Linker Flags
#

if(NOT MSVC)
	set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})

	set(CMAKE_REQUIRED_FLAGS "-Wl,--no-undefined")
	check_c_compiler_flag("" _LINKER_HAS_NO_UNDEFINED)
	if (_LINKER_HAS_NO_UNDEFINED)
		set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined ${CMAKE_SHARED_LINKER_FLAGS}")
	endif()

	set(CMAKE_REQUIRED_FLAGS "-Wl,-Bsymbolic")
	check_c_compiler_flag("" _LINKER_HAS_BSYMBOLIC)
	if (_LINKER_HAS_BSYMBOLIC)
		set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic ${CMAKE_SHARED_LINKER_FLAGS}")
	endif()

	if (MINGW)
		set(CMAKE_REQUIRED_FLAGS "-Wl,--nxcompat")
		check_c_compiler_flag("" _LINKER_HAS_DEP)
		if (_LINKER_HAS_DEP)
			set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--nxcompat ${CMAKE_SHARED_LINKER_FLAGS}")
		endif()
		set(CMAKE_REQUIRED_FLAGS "-Wl,--dynamicbase")
		check_c_compiler_flag("" _LINKER_HAS_ASLR)
		if (_LINKER_HAS_ASLR)
			set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--dynamicbase ${CMAKE_SHARED_LINKER_FLAGS}")
		endif()
		set(CMAKE_REQUIRED_FLAGS "-Wl,--high-entropy-va")
		check_c_compiler_flag("" _LINKER_HAS_64ASLR)
		if (_LINKER_HAS_64ASLR)
			set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--high-entropy-va ${CMAKE_SHARED_LINKER_FLAGS}")
		endif()
	endif()
	
	set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endif()

#
# Python-Generated Files
#

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/epoxy")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/src")

set(EPOXY_GENERATED_APIS gl egl wgl glx)
set(EPOXY_GENERATED_FILES "")
foreach(EPOXY_GENERATED_API ${EPOXY_GENERATED_APIS})
	set(EPOXY_GENERATED_API_FILES "${CMAKE_CURRENT_BINARY_DIR}/include/epoxy/${EPOXY_GENERATED_API}_generated.h" "${CMAKE_CURRENT_BINARY_DIR}/src/${EPOXY_GENERATED_API}_generated_dispatch.c")
	list(APPEND EPOXY_GENERATED_FILES ${EPOXY_GENERATED_API_FILES})
	add_custom_command (
		OUTPUT ${EPOXY_GENERATED_API_FILES}
		COMMAND ${PYTHON_EXECUTABLE}
			"${PROJECT_SOURCE_DIR}/src/gen_dispatch.py" --dir "${CMAKE_CURRENT_BINARY_DIR}"
			"${PROJECT_SOURCE_DIR}/registry/${EPOXY_GENERATED_API}.xml"
		MAIN_DEPENDENCY
			"${PROJECT_SOURCE_DIR}/registry/${EPOXY_GENERATED_API}.xml"
		DEPENDS
			"${PROJECT_SOURCE_DIR}/src/gen_dispatch.py")
endforeach()
add_custom_target (epoxy_generated ALL SOURCES ${EPOXY_GENERATED_FILES})

#
# Build Targets
#

set(EPOXY_TARGET_BUILT "")
set(EPOXY_TARGET_TYPES "")

if(EPOXY_BUILD_SHARED)
	list(APPEND EPOXY_TARGET_TYPES "shared")
	add_library("${EPOXY_TARGET_NAME}_shared" SHARED ${EPOXY_SOURCE} ${EPOXY_HEADERS})
	if(WIN32 OR ANDROID OR MSYS)
		set_target_properties("${EPOXY_TARGET_NAME}_shared" PROPERTIES
			OUTPUT_NAME "${TARGET_OUTPUT_NAME}${TARGET_ABI_VER}")
	else()
		set_target_properties("${EPOXY_TARGET_NAME}_shared" PROPERTIES
			OUTPUT_NAME "${TARGET_OUTPUT_NAME}")
	endif()
	if(NOT ANDROID)
		set_target_properties("${EPOXY_TARGET_NAME}_shared" PROPERTIES
			VERSION "${TARGET_VER}"
			SOVERSION "${TARGET_ABI_VER}")
	endif()
	set_target_properties("${EPOXY_TARGET_NAME}_shared" PROPERTIES
		COMPILE_DEFINITIONS "EPOXY_DLL")
endif()

if(EPOXY_BUILD_STATIC)
	list(APPEND EPOXY_TARGET_TYPES "static")
	add_library("${EPOXY_TARGET_NAME}_static" STATIC ${EPOXY_SOURCE} ${EPOXY_HEADERS})
	if(MSVC)
		set_target_properties("${EPOXY_TARGET_NAME}_static" PROPERTIES
			OUTPUT_NAME "${TARGET_OUTPUT_NAME}${TARGET_ABI_VER}_static")
	else()
		set_target_properties("${EPOXY_TARGET_NAME}_static" PROPERTIES
			OUTPUT_NAME "${TARGET_OUTPUT_NAME}${TARGET_ABI_VER}")
	endif()
endif()

foreach(EPOXY_TARGET_TYPE ${EPOXY_TARGET_TYPES})
	list(APPEND EPOXY_TARGET_BUILT "${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}")
	if(NOT MSYS)
		target_link_libraries("${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}" ${CMAKE_DL_LIBS})
	endif()
	if(HONOR_VISILIBITY)
		set_target_properties("${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}" PROPERTIES
			C_VISIBILITY_PRESET hidden)
	elseif(_COMPILER_HAS_VISIBILITY_HIDDEN)
		set_target_properties("${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}" PROPERTIES
			COMPILE_FLAGS "-fvisibility=hidden")
	endif()
	set_target_properties("${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}" PROPERTIES
		POSITION_INDEPENDENT_CODE ON)
endforeach()

#
# Tests
#

if(EPOXY_BUILD_TESTS)
	enable_testing()
	foreach(EPOXY_TARGET_TYPE ${EPOXY_TARGET_TYPES})
		add_executable(test_${EPOXY_TARGET_TYPE}_headerguards "test/headerguards.c")
		target_link_libraries(test_${EPOXY_TARGET_TYPE}_headerguards ${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE})
		add_test(NAME "test_${EPOXY_TARGET_TYPE}_headerguards" COMMAND "test_${EPOXY_TARGET_TYPE}_headerguards")

		add_executable(test_${EPOXY_TARGET_TYPE}_miscdefines "test/miscdefines.c")
		target_link_libraries(test_${EPOXY_TARGET_TYPE}_miscdefines ${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE})
		add_test(NAME "test_${EPOXY_TARGET_TYPE}_miscdefines" COMMAND "test_${EPOXY_TARGET_TYPE}_miscdefines")

		if(EPOXY_SUPPORT_GLX AND X11_FOUND)
			set(DL_TEST_COMMON "test/dlwrap.c" "test/dlwrap.h")
			set(GLX_TEST_COMMON "test/glx_common.c" "test/glx_common.h")

			if(NOT APPLE)
				add_executable(test_glx_${EPOXY_TARGET_TYPE}_alias_prefer_same ${DL_TEST_COMMON} ${GLX_TEST_COMMON} "test/glx_alias_prefer_same_name.c")
				set_target_properties(test_glx_${EPOXY_TARGET_TYPE}_alias_prefer_same PROPERTIES LINK_FLAGS "-rdynamic")
				target_link_libraries(test_glx_${EPOXY_TARGET_TYPE}_alias_prefer_same "${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}" ${CMAKE_DL_LIBS} ${X11_LIBRARIES})
				add_test(NAME "test_glx_${EPOXY_TARGET_TYPE}_alias_prefer_same" COMMAND "test_glx_${EPOXY_TARGET_TYPE}_alias_prefer_same")
			endif()

			if(OPENGL_FOUND)
				add_executable(test_glx_${EPOXY_TARGET_TYPE}_begin_end ${GLX_TEST_COMMON} "test/glx_beginend.c")
				target_link_libraries(test_glx_${EPOXY_TARGET_TYPE}_begin_end "${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}" ${OPENGL_gl_LIBRARY} ${X11_LIBRARIES})
				add_test(NAME "test_glx_${EPOXY_TARGET_TYPE}_begin_end" COMMAND "test_glx_${EPOXY_TARGET_TYPE}_begin_end")
			endif()

			add_executable(test_glx_${EPOXY_TARGET_TYPE}_public_api_compat ${GLX_TEST_COMMON} "test/glx_public_api.c")
			target_link_libraries(test_glx_${EPOXY_TARGET_TYPE}_public_api_compat "${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}" ${X11_LIBRARIES})
			add_test(NAME "test_glx_${EPOXY_TARGET_TYPE}_public_api_compat" COMMAND "test_glx_${EPOXY_TARGET_TYPE}_public_api_compat")
			
			add_executable(test_glx_${EPOXY_TARGET_TYPE}_public_api_core ${GLX_TEST_COMMON} "test/glx_public_api_core.c")
			target_link_libraries(test_glx_${EPOXY_TARGET_TYPE}_public_api_core "${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}" ${X11_LIBRARIES})
			add_test(NAME "test_glx_${EPOXY_TARGET_TYPE}_public_api_core" COMMAND "test_glx_${EPOXY_TARGET_TYPE}_public_api_core")
		endif()
		
		if(EPOXY_SUPPORT_WGL)
			set(WGL_TEST_COMMON "test/wgl_common.c" "test/wgl_common.h")

			add_executable(test_wgl_${EPOXY_TARGET_TYPE}_core_and_exts ${WGL_TEST_COMMON} "test/wgl_core_and_exts.c")
			target_link_libraries(test_wgl_${EPOXY_TARGET_TYPE}_core_and_exts "${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}")
			add_test(NAME "test_wgl_${EPOXY_TARGET_TYPE}_core_and_exts" COMMAND "test_wgl_${EPOXY_TARGET_TYPE}_core_and_exts")
			
			add_executable(test_wgl_${EPOXY_TARGET_TYPE}_context_funcptrs ${WGL_TEST_COMMON} "test/wgl_per_context_funcptrs.c")
			target_link_libraries(test_wgl_${EPOXY_TARGET_TYPE}_context_funcptrs "${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}")
			add_test(NAME "test_wgl_${EPOXY_TARGET_TYPE}_context_funcptrs" COMMAND "test_wgl_${EPOXY_TARGET_TYPE}_context_funcptrs")

			add_executable(test_wgl_${EPOXY_TARGET_TYPE}_usefontbitmaps_ascii ${WGL_TEST_COMMON} "test/wgl_usefontbitmaps.c")
			target_link_libraries(test_wgl_${EPOXY_TARGET_TYPE}_usefontbitmaps_ascii "${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}")
			add_test(NAME "test_wgl_${EPOXY_TARGET_TYPE}_usefontbitmaps_ascii" COMMAND "test_wgl_${EPOXY_TARGET_TYPE}_usefontbitmaps_ascii")

			add_executable(test_wgl_${EPOXY_TARGET_TYPE}_usefontbitmaps_wide ${WGL_TEST_COMMON} "test/wgl_usefontbitmaps.c")
			target_link_libraries(test_wgl_${EPOXY_TARGET_TYPE}_usefontbitmaps_wide "${EPOXY_TARGET_NAME}_${EPOXY_TARGET_TYPE}")
			add_test(NAME "test_wgl_${EPOXY_TARGET_TYPE}_usefontbitmaps_wide" COMMAND "test_wgl_${EPOXY_TARGET_TYPE}_usefontbitmaps_wide")

			if("${EPOXY_TARGET_TYPE}" STREQUAL "shared")
				set_target_properties(test_wgl_${EPOXY_TARGET_TYPE}_core_and_exts PROPERTIES
						COMPILE_DEFINITIONS "EPOXY_DLL")
				set_target_properties(test_wgl_${EPOXY_TARGET_TYPE}_context_funcptrs PROPERTIES
						COMPILE_DEFINITIONS "EPOXY_DLL")
				set_target_properties(test_wgl_${EPOXY_TARGET_TYPE}_usefontbitmaps_ascii PROPERTIES
						COMPILE_DEFINITIONS "EPOXY_DLL")
				set_target_properties(test_wgl_${EPOXY_TARGET_TYPE}_usefontbitmaps_wide PROPERTIES
						COMPILE_DEFINITIONS "_UNICODE;UNICODE;EPOXY_DLL")
			else()
				set_target_properties(test_wgl_${EPOXY_TARGET_TYPE}_usefontbitmaps_wide PROPERTIES
						COMPILE_DEFINITIONS "_UNICODE;UNICODE")
			endif()
		endif()
	endforeach()
endif()

#
# Install
#

install(TARGETS ${EPOXY_TARGET_BUILT}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/epoxy" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/epoxy" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(EPOXY_INSTALL_KHRPLATFORM)
	install(DIRECTORY "${PROJECT_SOURCE_DIR}/registry/KHR" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
if(MSVC)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Debug/${TARGET_OUTPUT_NAME}${TARGET_ABI_VER}.pdb" DESTINATION ${CMAKE_INSTALL_BINDIR} CONFIGURATIONS Debug)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/${TARGET_OUTPUT_NAME}${TARGET_ABI_VER}.pdb" DESTINATION ${CMAKE_INSTALL_BINDIR} CONFIGURATIONS RelWithDebInfo)
endif()
