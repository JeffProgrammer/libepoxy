cmake_minimum_required(VERSION 2.8)
project(epoxy)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake/modules")

#
# Project Options
#

option(BUILD_SHARED_LIBS "Build Shared Libraries" ON)
option(EPOXY_BUILD_TESTS "Build Epoxy Test Programs" ON)

if(UNIX AND NOT MSYS)
	option(EPOXY_BUILD_GLX "Build Epoxy GLX Support" ON)
else()
	option(EPOXY_BUILD_GLX "Build Epoxy GLX Support" OFF)
endif()

if(UNIX AND NOT APPLE)
	option(EPOXY_BUILD_EGL "Build Epoxy EGL Support" ON)
else()
	option(EPOXY_BUILD_EGL "Build Epoxy EGL Support" OFF)
endif()

if (MSVC)
    option(USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC Runtime Library DLL" ON)
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

	set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})

	check_c_compiler_flag("-std=gnu11" _EPOXY_HAS_GNU11)
	if (_EPOXY_HAS_GNU11)
		add_definitions(-std=gnu11)
	else()
		check_c_compiler_flag("-std=gnu99" _EPOXY_HAS_GNU99)
		if (_EPOXY_HAS_GNU99)
			add_definitions(-std=gnu99)
		endif()
	endif()

	check_c_compiler_flag("-Wno-int-conversion" _EPOXY_HAS_NOINTCONVERSION)
	if(_EPOXY_HAS_NOINTCONVERSION)
		add_definitions(-Wno-int-conversion)
	endif()

	set(CMAKE_REQUIRED_FLAGS "-fvisibility=hidden")
	check_c_compiler_flag("" _EPOXY_HAS_VISIBLITYHIDDEN)
	if (_EPOXY_HAS_VISIBLITYHIDDEN)
		add_definitions(-fvisibility=hidden)
	endif()

	set(CMAKE_REQUIRED_FLAGS "--no-undefined")
	check_c_compiler_flag("" _EPOXY_HAS_NOUNDEFINED)
	if (_EPOXY_HAS_NOUNDEFINED)
		set(CMAKE_SHARED_LINKER_FLAGS "--no-undefined ${CMAKE_SHARED_LINKER_FLAGS}")
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
	
	set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endif()

#
# Build Configuration
#

set(_EPOXY_INSTALL_KHR 0)
set(_EPOXY_EGL 0)
set(_EPOXY_GLX 0)
set(_EPOXY_WGL 0)

find_package(EGL)
find_package(OpenGL)
find_package(X11)

find_path(KHR_INCLUDE_DIR NAMES KHR/khrplatform.h PATHS /opt/vc/include)
mark_as_advanced(KHR_INCLUDE_DIR)
if(NOT KHR_INCLUDE_DIR)
	message("-- Could NOT Find Khronos Platform Header (missing:  KHR/khrplatform.h)")
	message("-- Using Repository Khronos Platform Header")
	set(KHR_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/registry")
	set(_EPOXY_INSTALL_KHR 1)
else()
	message("-- Found Khronos Platform Header: ${KHR_INCLUDE_DIR}/KHR/khrplatform.h)")
endif()

if(WIN32 OR MSYS)
	set(_EPOXY_WGL 1)
endif()
if(EPOXY_BUILD_EGL AND (EGL_LIBRARY OR EGL_INCLUDE_DIR))
	set(_EPOXY_EGL 1)
endif()
if(EPOXY_BUILD_GLX AND X11_FOUND)
	set(_EPOXY_GLX 1)
endif()

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/include/epoxy")
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src")
configure_file (
	"${PROJECT_SOURCE_DIR}/include/epoxy_config.h.in"
	"${PROJECT_BINARY_DIR}/include/epoxy_config.h"
)
include_directories("${PROJECT_BINARY_DIR}/include" "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/src" "${KHR_INCLUDE_DIR}")

#
# Generated Files
#

find_package(PythonInterp)
if(PYTHONINTERP_FOUND)
	set(PYTHON_COMMAND ${PYTHON_EXECUTABLE})
else()
	set(PYTHON_COMMAND python)
endif()

add_custom_command (
	OUTPUT
		${PROJECT_BINARY_DIR}/include/epoxy/egl_generated.h
		${PROJECT_BINARY_DIR}/src/egl_generated_dispatch.c
	COMMAND ${PYTHON_COMMAND} ${PROJECT_SOURCE_DIR}/src/gen_dispatch.py --dir ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/registry/egl.xml
)
add_custom_command (
	OUTPUT
		${PROJECT_BINARY_DIR}/include/epoxy/gl_generated.h
		${PROJECT_BINARY_DIR}/src/gl_generated_dispatch.c
	COMMAND ${PYTHON_COMMAND} ${PROJECT_SOURCE_DIR}/src/gen_dispatch.py --dir ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/registry/gl.xml
)
add_custom_command (
	OUTPUT
		${PROJECT_BINARY_DIR}/include/epoxy/glx_generated.h
		${PROJECT_BINARY_DIR}/src/glx_generated_dispatch.c
	COMMAND ${PYTHON_COMMAND} ${PROJECT_SOURCE_DIR}/src/gen_dispatch.py --dir ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/registry/glx.xml
)
add_custom_command (
	OUTPUT
		${PROJECT_BINARY_DIR}/include/epoxy/wgl_generated.h
		${PROJECT_BINARY_DIR}/src/wgl_generated_dispatch.c
	COMMAND ${PYTHON_COMMAND} ${PROJECT_SOURCE_DIR}/src/gen_dispatch.py --dir ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/registry/wgl.xml
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
	find_package(EGL)
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

set(EPOXY_LIB_NAME epoxy_static)
set(EPOXY_LIB_TYPE STATIC)
if (BUILD_SHARED_LIBS)
	if(MSVC)
		add_definitions(-DEPOXY_DLL)
	endif()
	set(EPOXY_LIB_TYPE SHARED)
	set(EPOXY_LIB_NAME epoxy)
endif()

add_library(epoxy ${EPOXY_LIB_TYPE} ${GL_SOURCE} ${GL_HEADERS})
set_target_properties(epoxy PROPERTIES
		OUTPUT_NAME "${EPOXY_LIB_NAME}"
		VERSION "1.3.1"
		SOVERSION "1"
		POSITION_INDEPENDENT_CODE ON)

#
# Test Executables
#

if(EPOXY_BUILD_TESTS)
	enable_testing()

	if(_EPOXY_EGL AND X11_FOUND)
		add_executable(test_egl_has_extension_nocontext ${EGL_TESTS} "${PROJECT_SOURCE_DIR}/test/egl_has_extension_nocontext.c")
		target_link_libraries(test_egl_has_extension_nocontext epoxy ${X11_LIBRARIES})
		add_test(test_egl_has_extension_nocontext test_egl_has_extension_nocontext)

		add_executable(test_egl_gl ${EGL_TESTS} "${PROJECT_SOURCE_DIR}/test/egl_gl.c")
		target_link_libraries(test_egl_gl epoxy ${CMAKE_DL_LIBS} ${X11_LIBRARIES})
		add_test(test_egl_gl test_egl_gl)

		add_executable(test_egl_gles1_without_glx ${EGL_TESTS} "${PROJECT_SOURCE_DIR}/test/egl_without_glx.c")
		set_target_properties(test_egl_gles1_without_glx PROPERTIES COMPILE_FLAGS "-DGLES_VERSION=1")
		target_link_libraries(test_egl_gles1_without_glx epoxy ${CMAKE_DL_LIBS} ${X11_LIBRARIES})
		add_test(test_egl_gles1_without_glx test_egl_gles1_without_glx)

		add_executable(test_egl_gles2_without_glx ${EGL_TESTS} "${PROJECT_SOURCE_DIR}/test/egl_without_glx.c")
		set_target_properties(test_egl_gles2_without_glx PROPERTIES COMPILE_FLAGS "-DGLES_VERSION=2")
		target_link_libraries(test_egl_gles2_without_glx epoxy ${CMAKE_DL_LIBS} ${X11_LIBRARIES})
		add_test(test_egl_gles2_without_glx test_egl_gles2_without_glx)
	endif()

	if(_EPOXY_EGL AND _EPOXY_GLX AND NOT APPLE)
		add_executable(test_egl_and_glx_different_pointers ${EGL_TESTS} ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/egl_and_glx_different_pointers.c" "${PROJECT_SOURCE_DIR}/test/dlwrap.c" "${PROJECT_SOURCE_DIR}/test/dlwrap.h")
		set_target_properties(test_egl_and_glx_different_pointers PROPERTIES COMPILE_FLAGS "-DUSE_EGL -DUSE_GLX" LINK_FLAGS "-rdynamic")
		target_link_libraries(test_egl_and_glx_different_pointers epoxy ${CMAKE_DL_LIBS} ${X11_LIBRARIES})
		add_test(test_egl_and_glx_different_pointers test_egl_and_glx_different_pointers)

		add_executable(test_egl_different_pointers ${EGL_TESTS} ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/egl_and_glx_different_pointers.c" "${PROJECT_SOURCE_DIR}/test/dlwrap.c" "${PROJECT_SOURCE_DIR}/test/dlwrap.h")
		set_target_properties(test_egl_different_pointers PROPERTIES COMPILE_FLAGS "-DUSE_EGL" LINK_FLAGS "-rdynamic")
		target_link_libraries(test_egl_different_pointers epoxy ${CMAKE_DL_LIBS} ${X11_LIBRARIES})
		add_test(test_egl_different_pointers test_egl_different_pointers)

		add_executable(test_glx_different_pointers ${EGL_TESTS} ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/egl_and_glx_different_pointers.c" "${PROJECT_SOURCE_DIR}/test/dlwrap.c" "${PROJECT_SOURCE_DIR}/test/dlwrap.h")
		set_target_properties(test_glx_different_pointers PROPERTIES COMPILE_FLAGS "-DUSE_GLX" LINK_FLAGS "-rdynamic")
		target_link_libraries(test_glx_different_pointers epoxy ${CMAKE_DL_LIBS} ${X11_LIBRARIES})
		add_test(test_glx_different_pointers test_glx_different_pointers)
	endif()

	if(_EPOXY_GLX)
		if(!APPLE)
			add_executable(test_glx_alias_prefer_same_name ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/glx_alias_prefer_same_name.c" "${PROJECT_SOURCE_DIR}/test/dlwrap.c" "${PROJECT_SOURCE_DIR}/test/dlwrap.h")
			set_target_properties(test_glx_alias_prefer_same_name PROPERTIES LINK_FLAGS "-rdynamic")
			target_link_libraries(test_glx_alias_prefer_same_name epoxy ${CMAKE_DL_LIBS} ${X11_LIBRARIES})
			add_test(test_glx_alias_prefer_same_name test_glx_alias_prefer_same_name)
		endif()

		add_executable(test_glx_beginend ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/glx_beginend.c")
		target_link_libraries(test_glx_beginend epoxy ${OPENGL_gl_LIBRARY} ${X11_LIBRARIES})
		add_test(test_glx_beginend test_glx_beginend)

		if(!APPLE)
			add_executable(test_glx_gles2 ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/glx_gles2.c" "${PROJECT_SOURCE_DIR}/test/dlwrap.c" "${PROJECT_SOURCE_DIR}/test/dlwrap.h")
			set_target_properties(test_glx_gles2 PROPERTIES LINK_FLAGS "-rdynamic")
			target_link_libraries(test_glx_gles2 epoxy ${CMAKE_DL_LIBS} ${X11_LIBRARIES})
			add_test(test_glx_gles2 test_glx_gles2)
		endif()

		add_executable(test_glx_public_api ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/glx_public_api.c")
		target_link_libraries(test_glx_public_api epoxy ${X11_LIBRARIES})
		add_test(test_glx_public_api test_glx_public_api)

		add_executable(test_glx_public_api_core ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/glx_public_api_core.c")
		target_link_libraries(test_glx_public_api_core epoxy ${X11_LIBRARIES})
		add_test(test_glx_public_api_core test_glx_public_api_core)

		add_executable(test_glx_getprocaddress_nocontext ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/glx_glxgetprocaddress_nocontext.c")
		target_link_libraries(test_glx_getprocaddress_nocontext epoxy ${X11_LIBRARIES})
		add_test(test_glx_getprocaddress_nocontext test_glx_getprocaddress_nocontext)

		add_executable(test_glx_has_extension_nocontext ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/glx_has_extension_nocontext.c")
		target_link_libraries(test_glx_has_extension_nocontext epoxy ${X11_LIBRARIES})
		add_test(test_glx_has_extension_nocontext test_glx_has_extension_nocontext)

		if(BUILD_SHARED_LIBS)
			add_executable(test_glx_shared_znow ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/glx_static.c")
			set_target_properties(test_glx_shared_znow PROPERTIES LINK_FLAGS "-Wl,-z,now")
			target_link_libraries(test_glx_shared_znow epoxy ${CMAKE_DL_LIBS} ${X11_LIBRARIES})
			add_test(test_glx_shared_znow test_glx_shared_znow)
		else()
			add_executable(test_glx_static ${GLX_TESTS} "${PROJECT_SOURCE_DIR}/test/glx_static.c")
			set_target_properties(test_glx_static PROPERTIES COMPILE_FLAGS "-DNEEDS_TO_BE_STATIC")
			target_link_libraries(test_glx_static epoxy ${CMAKE_DL_LIBS} ${X11_LIBRARIES})
			add_test(test_glx_static test_glx_static)
		endif()
	endif()

	add_executable(test_headerguards "${PROJECT_SOURCE_DIR}/test/headerguards.c")
	if(APPLE)
		set(GLEXT_HEADERS OpenGL/glext.h)
	else()
		set(GLEXT_HEADERS GL/glext.h)
	endif()
	if(_EPOXY_EGL)
		list(APPEND GLEXT_HEADERS EGL/eglext.h)
	endif()
	if(_EPOXY_GLX)
		list(APPEND GLEXT_HEADERS GL/glxext.h)
	endif()
	if(_EPOXY_WGL)
		list(APPEND GLEXT_HEADERS GL/wglext.h)
	endif()
	find_path(GLEXT_INCLUDE_DIR NAMES ${GLEXT_HEADERS} PATHS /opt/vc/include)
	if(GLEXT_INCLUDE_DIR)
		message("-- Found GL Extension Headers: ${GLEXT_INCLUDE_DIR}/${GLEXT_HEADERS}")
	else()
		message("-- Could NOT Find GL Extension Headers (missing:  ${GLEXT_HEADERS})")
		set_target_properties(test_headerguards PROPERTIES COMPILE_DEFINITIONS "NO_GLEXT_H")
	endif()
	target_link_libraries(test_headerguards epoxy)
	add_test(test_headerguards test_headerguards)

	add_executable(test_miscdefines "${PROJECT_SOURCE_DIR}/test/miscdefines.c")
	target_link_libraries(test_miscdefines epoxy)
	add_test(test_miscdefines test_miscdefines)

	if(_EPOXY_WGL)
		add_executable(test_wgl_core_and_exts ${WGL_TESTS} "${PROJECT_SOURCE_DIR}/test/wgl_core_and_exts.c")
		target_link_libraries(test_wgl_core_and_exts epoxy)
		add_test(test_wgl_core_and_exts test_wgl_core_and_exts)

		add_executable(test_wgl_per_context_funcptrs ${WGL_TESTS} "${PROJECT_SOURCE_DIR}/test/wgl_per_context_funcptrs.c")
		target_link_libraries(test_wgl_per_context_funcptrs epoxy)
		add_test(test_wgl_per_context_funcptrs test_wgl_per_context_funcptrs)

		add_executable(test_wgl_usefontbitmaps_ascii ${WGL_TESTS} "${PROJECT_SOURCE_DIR}/test/wgl_usefontbitmaps.c")
		target_link_libraries(test_wgl_usefontbitmaps_ascii epoxy)
		add_test(test_wgl_usefontbitmaps_ascii test_wgl_usefontbitmaps_ascii)

		add_executable(test_wgl_usefontbitmaps_unicode ${WGL_TESTS} "${PROJECT_SOURCE_DIR}/test/wgl_usefontbitmaps.c")
		set_target_properties(test_wgl_usefontbitmaps_unicode PROPERTIES COMPILE_DEFINITIONS "_UNICODE;UNICODE")
		target_link_libraries(test_wgl_usefontbitmaps_unicode epoxy)
		add_test(test_wgl_usefontbitmaps_unicode test_wgl_usefontbitmaps_unicode)
	endif()
endif()

#
# Install
#

include("GNUInstallDirs")
install(TARGETS epoxy
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY "${PROJECT_BINARY_DIR}/include/epoxy" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/epoxy" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(_EPOXY_INSTALL_KHR)
	install(DIRECTORY "${PROJECT_SOURCE_DIR}/registry/KHR" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
