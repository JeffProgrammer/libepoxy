cmake_minimum_required(VERSION 2.8)
project(epoxy)

#
# Project Options
#

option(BUILD_SHARED_LIBS "Build Shared Libraries" ON)
option(EPOXY_BUILD_TESTS "Build Epoxy Test Programs" ON)
option(EPOXY_USE_DLLIMPORT "Use __declspec(dllimport) For Shared Library" ON)

if(UNIX AND NOT MSYS)
	option(EPOXY_BUILD_GLX "Build Epoxy GLX support" ON)
	option(EPOXY_BUILD_EGL "Build Epoxy EGL support" ON)
else()
	option(EPOXY_BUILD_EGL "Build Epoxy EGL support" OFF)
endif()
if (MSVC)
    option(USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC runtime library DLL" ON)
endif()

#
# COMPILER FLAGS
#

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    if (NOT USE_MSVC_RUNTIME_LIBRARY_DLL)
        foreach (flag CMAKE_C_FLAGS
                        CMAKE_C_FLAGS_DEBUG
                        CMAKE_C_FLAGS_RELEASE
                        CMAKE_C_FLAGS_MINSIZEREL
                        CMAKE_C_FLAGS_RELWITHDEBINFO)
            if (${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
            endif()
            if (${flag} MATCHES "/MDd")
                string(REGEX REPLACE "/MDd" "/MTd" ${flag} "${${flag}}")
            endif()
        endforeach()
    endif()
else()
    include(CheckCCompilerFlag)

    add_definitions(-Wall)

    set(CMAKE_REQUIRED_FLAGS "-Wno-int-conversion")
    check_c_compiler_flag("" _EPOXY_HAS_NOINTCONVERSION)
    if(_EPOXY_HAS_NOINTCONVERSION)
        add_definitions(-Wno-int-conversion)
    endif()

	set(CMAKE_REQUIRED_FLAGS "-fvisibility=hidden")
	check_c_compiler_flag("" _EPOXY_HAS_VISIBLITYHIDDEN)
	if (_EPOXY_HAS_VISIBLITYHIDDEN)
		add_definitions(-fvisibility=hidden)
	endif()

	set(CMAKE_REQUIRED_FLAGS "-no-undefined")
	check_c_compiler_flag("" _EPOXY_HAS_NOUNDEFINED)
	if (_EPOXY_HAS_NOUNDEFINED)
		set(CMAKE_SHARED_LINKER_FLAGS "-no-undefined ${CMAKE_SHARED_LINKER_FLAGS}")
	endif()

	set(CMAKE_REQUIRED_FLAGS "-Bsymbolic")
	check_c_compiler_flag("" _EPOXY_HAS_BSYMBOLIC)
	if (_EPOXY_HAS_BSYMBOLIC)
		set(CMAKE_SHARED_LINKER_FLAGS "-Bsymbolic ${CMAKE_SHARED_LINKER_FLAGS}")
	endif()

	if (MINGW)
		set(CMAKE_REQUIRED_FLAGS "-Wl,--nxcompat")
		check_c_compiler_flag("" _EPOXY_HAS_DEP)
		if (_EPOXY_HAS_DEP)
			set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--nxcompat ${CMAKE_SHARED_LINKER_FLAGS}")
		endif()
		set(CMAKE_REQUIRED_FLAGS "-Wl,--dynamicbase")
		check_c_compiler_flag("" _EPOXY_HAS_ASLR)
		if (_EPOXY_HAS_ASLR)
			set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--dynamicbase ${CMAKE_SHARED_LINKER_FLAGS}")
		endif()
		set(CMAKE_REQUIRED_FLAGS "-Wl,--high-entropy-va")
		check_c_compiler_flag("" _EPOXY_HAS_64ASLR)
		if (_EPOXY_HAS_64ASLR)
			set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--high-entropy-va ${CMAKE_SHARED_LINKER_FLAGS}")
		endif()
	endif()
endif()

#
# Build Configuration
#

if (WIN32)
	set(_EPOXY_WGL 1)
else()
	set(_EPOXY_WGL 0)
endif()
if(EPOXY_USE_EGL)
	set(_EPOXY_EGL 1)
else()
	set(_EPOXY_EGL 0)
endif()
if(EPOXY_USE_GLX)
	set(_EPOXY_GLX 1)
else()
	set(_EPOXY_GLX 0)
endif()

include(CheckIncludeFiles)
check_include_files("KHR/khrplatform.h" HAVE_KHRPLATFORM_H)

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/include/epoxy")
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src")
configure_file (
	"${PROJECT_SOURCE_DIR}/include/epoxy_config.h.in"
	"${PROJECT_BINARY_DIR}/include/epoxy_config.h"
)
include_directories("${PROJECT_BINARY_DIR}/include" "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/src")

#
# Generated Files
#

add_custom_command (
	OUTPUT
		${PROJECT_BINARY_DIR}/include/epoxy/egl_generated.h
		${PROJECT_BINARY_DIR}/src/egl_generated_dispatch.c
	COMMAND python ${PROJECT_SOURCE_DIR}/src/gen_dispatch.py --dir ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/registry/egl.xml
)
add_custom_command (
	OUTPUT
		${PROJECT_BINARY_DIR}/include/epoxy/gl_generated.h
		${PROJECT_BINARY_DIR}/src/gl_generated_dispatch.c
	COMMAND python ${PROJECT_SOURCE_DIR}/src/gen_dispatch.py --dir ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/registry/gl.xml
)
add_custom_command (
	OUTPUT
		${PROJECT_BINARY_DIR}/include/epoxy/glx_generated.h
		${PROJECT_BINARY_DIR}/src/glx_generated_dispatch.c
	COMMAND python ${PROJECT_SOURCE_DIR}/src/gen_dispatch.py --dir ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/registry/glx.xml
)
add_custom_command (
	OUTPUT
		${PROJECT_BINARY_DIR}/include/epoxy/wgl_generated.h
		${PROJECT_BINARY_DIR}/src/wgl_generated_dispatch.c
	COMMAND python ${PROJECT_SOURCE_DIR}/src/gen_dispatch.py --dir ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/registry/wgl.xml
)

set(EGL_HEADERS
	"${PROJECT_SOURCE_DIR}/include/epoxy/egl.h"
	"${PROJECT_BINARY_DIR}/include/epoxy/egl_generated.h" )
set(GL_HEADERS
	"${PROJECT_SOURCE_DIR}/src/dispatch_common.h"
	"${PROJECT_SOURCE_DIR}/include/epoxy/gl.h"
	"${PROJECT_BINARY_DIR}/include/epoxy/gl_generated.h" )
set(GLX_HEADERS
	"${PROJECT_SOURCE_DIR}/include/epoxy/glx.h"
	"${PROJECT_BINARY_DIR}/include/epoxy/glx_generated.h" )
set(WGL_HEADERS
	"${PROJECT_SOURCE_DIR}/include/epoxy/wgl.h"
	"${PROJECT_BINARY_DIR}/include/epoxy/wgl_generated.h" )
set(EGL_SOURCE
	"${PROJECT_SOURCE_DIR}/src/dispatch_egl.c"
	"${PROJECT_BINARY_DIR}/src/egl_generated_dispatch.c" )
set(GL_SOURCE
	"${PROJECT_SOURCE_DIR}/src/dispatch_common.c"
	"${PROJECT_BINARY_DIR}/src/gl_generated_dispatch.c" )
set(GLX_SOURCE
	"${PROJECT_SOURCE_DIR}/src/dispatch_glx.c"
	"${PROJECT_BINARY_DIR}/src/glx_generated_dispatch.c" )
set(WGL_SOURCE
	"${PROJECT_SOURCE_DIR}/src/dispatch_wgl.c"
	"${PROJECT_BINARY_DIR}/src/wgl_generated_dispatch.c" )
	
#
# Build Subsystems
#

if(_EPOXY_EGL)
	list(APPEND GL_SOURCE "${EGL_SOURCE}")
	list(APPEND GL_HEADERS "${EGL_HEADERS}")
	set(EGL_TESTS
		"${PROJECT_SOURCE_DIR}/test/egl_common.c"
		"${PROJECT_SOURCE_DIR}/test/egl_common.h")
endif()
if(_EPOXY_GLX)
	list(APPEND GL_SOURCE "${GLX_SOURCE}")
	list(APPEND GL_HEADERS "${GLX_HEADERS}")
	set(GLX_TESTS
		"${PROJECT_SOURCE_DIR}/test/glx_common.c"
		"${PROJECT_SOURCE_DIR}/test/glx_common.h")
endif()
if(_EPOXY_WGL)
	list(APPEND GL_SOURCE "${WGL_SOURCE}")
	list(APPEND GL_HEADERS "${WGL_HEADERS}")
	set(WGL_TESTS
		"${PROJECT_SOURCE_DIR}/test/wgl_common.c"
		"${PROJECT_SOURCE_DIR}/test/wgl_common.h")
endif()

#
# Library Target
#

set(EPOXY_LIB_NAME epoxy1)
set(EPOXY_LIB_TYPE STATIC)
if (BUILD_SHARED_LIBS)
	set(EPOXY_LIB_TYPE SHARED)
	if (UNIX)
		set(EPOXY_LIB_NAME epoxy)
	endif()
endif()

add_library(epoxy ${EPOXY_LIB_TYPE} ${GL_SOURCE} ${GL_HEADERS})
set_target_properties(epoxy PROPERTIES
                      OUTPUT_NAME "${EPOXY_LIB_NAME}"
                      VERSION ${EPOXY_VERSION}
                      SOVERSION ${EPOXY_VERSION_MAJOR}
                      POSITION_INDEPENDENT_CODE ON)

#
# Test Executables
#

if(EPOXY_BUILD_TESTS)
	enable_testing()
	if(_EPOXY_WGL)
		add_executable(test_wgl_core_and_exts ${WGL_TESTS} "${PROJECT_SOURCE_DIR}/test/wgl_core_and_exts.c")
		target_link_libraries(test_wgl_core_and_exts epoxy)
		add_test(test_wgl_core_and_exts test_wgl_core_and_exts)
		add_executable(test_wgl_per_context_funcptrs ${WGL_TESTS} "${PROJECT_SOURCE_DIR}/test/wgl_per_context_funcptrs.c")
		target_link_libraries(test_wgl_per_context_funcptrs epoxy)
		add_test(test_wgl_per_context_funcptrs test_wgl_per_context_funcptrs)
		add_executable(test_wgl_usefontbitmaps ${WGL_TESTS} "${PROJECT_SOURCE_DIR}/test/wgl_usefontbitmaps.c")
		target_link_libraries(test_wgl_usefontbitmaps epoxy)
		add_test(test_wgl_usefontbitmaps test_wgl_usefontbitmaps)
	endif()
endif()

#
# Install
#

install(TARGETS epoxy
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION ${LIBDIR}
	ARCHIVE DESTINATION ${LIBDIR})

install(DIRECTORY "${PROJECT_BINARY_DIR}/include/epoxy" DESTINATION include)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/epoxy" DESTINATION include)
