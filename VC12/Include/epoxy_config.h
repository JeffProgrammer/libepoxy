/* #undef HAVE_KHRPLATFORM_H */
#define BUILD_EGL 0
#define BUILD_GLX 0
#define BUILD_WGL 1
/* #undef BUILD_SHARED_LIBS */

#ifdef BUILD_SHARED_LIBS
#define EPOXY_USE_DLLIMPORT
#endif
