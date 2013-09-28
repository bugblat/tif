// tif.h -------------------------------------------------------------
//
// Copyright (c) 2001 to 2013  te
//
// Licence: Creative Commons Attribution-ShareAlike 3.0 Unported License.
//          http://creativecommons.org/licenses/by-sa/3.0/
//---------------------------------------------------------------------
#ifndef tifH
#define tifH

#include "cp2112.h"

#define CFG_PAGE_SIZE           16
#define UFM_PAGE_SIZE           16
#define CFG_PAGE_COUNT          2175
#define UFM_PAGE_COUNT          512

#define FEATURE_ERASE           (1<<1)
#define CFG_ERASE               (1<<2)
#define UFM_ERASE               (1<<3)

#define DEFAULT_BUSY_LOOPS      5

#define A_ADDR                  (0<<6)     /* sending an address */
#define D_ADDR                  (1<<6)     /* sending data       */

class Tcp2112;
//---------------------------------------------------------------------
class Ttif {
  private:
    Tcp2112 *pCP;
    int     FinitResult;

    bool _cpDataWriteCfg(THidWrBuf& oBuf, int& Awritten);
    bool _cpDataWriteReadRequest(THidWrBuf& oBuf, int ArdCount=4);
    bool _cpReadDword(int AreportNum, THidRdBuf& ibuf);
    bool _cpReadDword(int AreportNum, uint32_t& v);
    bool _cpReadDword(int AreportNum, uint8_t* p);
    int  _doSimple(int Acmd, int Ap0=0);

    int  _progPage(int Acmd, const uint8_t *p);
    int  _readPages(int Acmd, int numPages, uint8_t *p);

    int  _initUfmAddr();
    int  _setUfmPageAddr(int pageNumber);
    int  _progUfmPage(const uint8_t *p);

    bool _isBusy();

    void _nanosleep(uint32_t ns);

  public:
    bool getDeviceIdCode(uint32_t& v);

    bool getStatusReg(uint32_t& v);
    int  getTraceId(uint8_t* p);

    int  enableCfgInterfaceOffline();
    int  enableCfgInterfaceTransparent();
    int  disableCfgInterface();
    int  refresh();
    int  progDone();

    int  erase(int Amask);
    int  eraseAll();

    int  initCfgAddr();
    int  eraseCfg();
    int  progCfgPage(const uint8_t *p);
    int  readCfgPages(int numPages, uint8_t *p);

    int  eraseUfm();
    int  readUfmPages(int numPages, uint8_t *p);
    int  readUfmPages(int pageNumber, int numPages, uint8_t *p);
    int  writeUfmPages(int pageNumber, int numPages, uint8_t *p);

    int  getBusyFlag(int *pFlag);
    bool waitUntilNotBusy(int maxLoops=DEFAULT_BUSY_LOOPS);

    int  setUsercode(uint8_t* p);
    int  getUsercode(uint8_t* p);

    //---------------------
    Tcp2112 *cp2112() { return pCP; }

    bool appRead(uint8_t *p, int AnumBytes, int& AnumRead);
    bool appWrite(uint8_t *p, int AnumBytes, int& AnumWritten);

    Ttif(int Avid, int Apid);
    ~Ttif();

  };

#endif
// EOF ----------------------------------------------------------------
