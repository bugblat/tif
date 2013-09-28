// tifwrap.cpp --------------------------------------------------------
//
// Copyright (c) 2001 to 2013  te
//
// a C wrapper for the tif code
//
// Licence: Creative Commons Attribution-ShareAlike 3.0 Unported License.
//          http://creativecommons.org/licenses/by-sa/3.0/
//---------------------------------------------------------------------
#if defined(_WIN32)
  #define _CRT_SECURE_NO_WARNINGS   // Disable deprecation warning in VS2005
#else
  #define _XOPEN_SOURCE 600         // For PATH_MAX on linux
#endif

#include <string.h>

#include "tifwrap.h"
#include "tif.h"

#define pTif ((Ttif *)h)

//---------------------------------------------------------------------
int version(char *outStr, int outLen) {
  if (outLen<=0)
    return 0;
  const char * retval = (const char *)("tif_lib," __DATE__ "," __TIME__);
  int retlen = strlen(retval);
  strncpy(outStr, retval, outLen);
  outStr[outLen-1] = 0;
  return retlen;
  }

int  getDeviceIdCode(tifHandle h, uint32_t* v) {
  return pTif->getDeviceIdCode(*v);
  }
int  getStatusReg(tifHandle h, uint32_t* v) {
  return pTif->getStatusReg(*v);
  }
int  getTraceId(tifHandle h, uint8_t* p) {
  return pTif->getTraceId(p);
  }
int  enableCfgInterfaceOffline(tifHandle h) {
  return pTif->enableCfgInterfaceOffline();
  }
int  enableCfgInterfaceTransparent(tifHandle h) {
  return pTif->enableCfgInterfaceTransparent();
  }
int  disableCfgInterface(tifHandle h) {
  return pTif->disableCfgInterface();
  }
int  refresh(tifHandle h) {
  return pTif->refresh();
  }
int  progDone(tifHandle h) {
  return pTif->progDone();
  }
int  erase(tifHandle h, int Amask) {
  return pTif->erase(Amask);
  }
int  eraseAll(tifHandle h) {
  return pTif->eraseAll();
  }
int  initCfgAddr(tifHandle h) {
  return pTif->initCfgAddr();
  }
int  eraseCfg(tifHandle h) {
  return pTif->eraseCfg();
  }
int  progCfgPage(tifHandle h, const uint8_t *p) {
  return pTif->progCfgPage(p);
  }
int  readCfgPages(tifHandle h, int numPages, uint8_t *p) {
  return pTif->readCfgPages(numPages, p);
  }
int  eraseUfm(tifHandle h) {
  return pTif->eraseUfm();
  }
int  readUfmPages(tifHandle h, int numPages, uint8_t *p) {
  return pTif->readUfmPages(numPages, p);
  }
int  readUfmPages(tifHandle h, int pageNumber, int numPages, uint8_t *p) {
  return pTif->readUfmPages(pageNumber, numPages, p);
  }
int  writeUfmPages(tifHandle h, int pageNumber, int numPages, uint8_t *p) {
  return pTif->writeUfmPages(pageNumber, numPages, p);
  }
int  getBusyFlag(tifHandle h, int *pFlag) {
  return pTif->getBusyFlag(pFlag);
  }
int  waitUntilNotBusy(tifHandle h, int maxLoops) {
  return pTif->waitUntilNotBusy(maxLoops);
  }
int  setUsercode(tifHandle h, uint8_t* p) {
  return pTif->setUsercode(p);
  }
int  getUsercode(tifHandle h, uint8_t* p) {
  return pTif->getUsercode(p);
  }

void *cp2112(tifHandle h) {
  return (void *)pTif->cp2112();
  }
int  appRead(tifHandle h, uint8_t *p, int AnumBytes, int* pNumRead) {
  return pTif->appRead(p, AnumBytes, *pNumRead);
  }
int  appWrite(tifHandle h, uint8_t *p, int AnumBytes, int* pNumWritten) {
  return pTif->appWrite(p, AnumBytes, *pNumWritten);
  }

tifHandle tifInit(int Avid, int Apid) {
  return (tifHandle)(new Ttif(Avid, Apid));
  }
void tifClose(tifHandle h) {
  delete pTif;
  }

// EOF ----------------------------------------------------------------
