// tifwrap.h ----------------------------------------------------------
//
// Copyright (c) 2001 to 2013  te
//
// a C wrapper for the tif code
//
// Licence: Creative Commons Attribution-ShareAlike 3.0 Unported License.
//          http://creativecommons.org/licenses/by-sa/3.0/
//---------------------------------------------------------------------
#ifndef tifwrapH
#define tifwrapH

#include <stdint.h>

#if defined _WIN32 || defined __CYGWIN__
  #define TIF_DLL_IMPORT __declspec(dllimport)
  #define TIF_DLL_EXPORT __declspec(dllexport)
  #define TIF_DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define TIF_DLL_IMPORT __attribute__ ((visibility ("default")))
    #define TIF_DLL_EXPORT __attribute__ ((visibility ("default")))
    #define TIF_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define TIF_DLL_IMPORT
    #define TIF_DLL_EXPORT
    #define TIF_DLL_LOCAL
  #endif
#endif

// Add '-DBUILDING_LIBTIF' and '-fvisibility=hidden' to the makefile flags

#if BUILDING_LIBTIF // && HAVE_VISIBILITY
  #define TIF_API extern TIF_DLL_EXPORT
#else
  #define TIF_API extern
#endif

typedef void * tifHandle;

//---------------------------------------------------------------------
#ifdef __cplusplus
  extern "C" {
#endif
TIF_API int version(char *outStr, int outLen);

TIF_API int  getDeviceIdCode(tifHandle h, uint32_t* v);

TIF_API int  getStatusReg(tifHandle h, uint32_t* v);
TIF_API int  getTraceId(tifHandle h, uint8_t* p);

TIF_API int  enableCfgInterfaceOffline(tifHandle h);
TIF_API int  enableCfgInterfaceTransparent(tifHandle h);
TIF_API int  disableCfgInterface(tifHandle h);
TIF_API int  refresh(tifHandle h);
TIF_API int  progDone(tifHandle h);

TIF_API int  erase(tifHandle h, int Amask);
TIF_API int  eraseAll(tifHandle h);

TIF_API int  initCfgAddr(tifHandle h);
TIF_API int  eraseCfg(tifHandle h);
TIF_API int  progCfgPage(tifHandle h, const uint8_t *p);
TIF_API int  readCfgPages(tifHandle h, int numPages, uint8_t *p);

TIF_API int  eraseUfm(tifHandle h);
TIF_API int  readUfmPages(tifHandle h, int pageNumber, int numPages, uint8_t *p);
TIF_API int  writeUfmPages(tifHandle h, int pageNumber, int numPages, uint8_t *p);

TIF_API int  getBusyFlag(tifHandle h, int *pFlag);
TIF_API int  waitUntilNotBusy(tifHandle h, int maxLoops);

TIF_API int  setUsercode(tifHandle h, uint8_t* p);
TIF_API int  getUsercode(tifHandle h, uint8_t* p);

//---------------------
TIF_API void *cp2112(tifHandle h);

TIF_API int  appRead(tifHandle h, uint8_t *p, int AnumBytes, int* AnumRead);
TIF_API int  appWrite(tifHandle h, uint8_t *p, int AnumBytes, int* AnumWritten);

TIF_API tifHandle tifInit(int Avid, int Apid);
TIF_API void tifClose(tifHandle h);

#ifdef __cplusplus
  }
#endif

#endif
// EOF ----------------------------------------------------------------
