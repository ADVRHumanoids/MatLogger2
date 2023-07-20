#ifndef VISIBILITY_H
#define VISIBILITY_H

// Generic helper definitions for shared library support
#if defined _WIN32 || defined __CYGWIN__
  #define MATL2_HELPER_DLL_IMPORT __declspec(dllimport)
  #define MATL2_HELPER_DLL_EXPORT __declspec(dllexport)
  #define MATL2_HELPER_DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define MATL2_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
    #define MATL2_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
    #define MATL2_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define MATL2_HELPER_DLL_IMPORT
    #define MATL2_HELPER_DLL_EXPORT
    #define MATL2_HELPER_DLL_LOCAL
  #endif
#endif

// Now we use the generic helper definitions above to define MATL2_API and MATL2_LOCAL.
// MATL2_API is used for the public API symbols. It either DLL imports or DLL exports (or does nothing for static build)
// MATL2_LOCAL is used for non-api symbols.

#ifdef MATL2_DLL // defined if MATL2 is compiled as a DLL
  #ifdef MATL2_DLL_EXPORTS // defined if we are building the MATL2 DLL (instead of using it)
    #define MATL2_API MATL2_HELPER_DLL_EXPORT
  #else
    #define MATL2_API MATL2_HELPER_DLL_IMPORT
  #endif // MATL2_DLL_EXPORTS
  #define MATL2_LOCAL MATL2_HELPER_DLL_LOCAL
#else // MATL2_DLL is not defined: this means MATL2 is a static lib.
  #define MATL2_API
  #define MATL2_LOCAL
#endif // MATL2_DLL


#endif // VISIBILITY_H
