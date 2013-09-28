// tif.cpp -----------------------------------------------------------
//
// Copyright (c) 2001 to 2013  te
//
// Licence: Creative Commons Attribution-ShareAlike 3.0 Unported License.
//          http://creativecommons.org/licenses/by-sa/3.0/
//---------------------------------------------------------------------

/*
XO2 Programming Interface

-----------------------------------------------------------------------------
UFM (Sector 1) Commands
-----------------------------------------------------------------------------

 Read Status Reg        0x3C  Read the 4-byte Configuration Status Register

 Check Busy Flag        0xF0  Read the Configuration Busy Flag status

 Bypass                 0xFF  Null operation.

 Enable Config I'face   0x74  Enable Transparent UFM access - All user I/Os
 (Transparent Mode)           (except the hardened user SPI port) are governed
                              by the user logic, the device remains in User
                              mode. (The subsequent commands in this table
                              require the interface to be enabled.)

 Enable Config I'face   0xC6  Enable Offline UFM access - All user I/Os
 (Offline Mode)               (except persisted sysCONFIG ports) are tri-stated
                              User logic ceases to function, UFM remains
                              accessible, and the device enters 'Offline'
                              access mode. (The subsequent commands in this
                              table require the interface to be enabled.)

 Disable Config I'face  0x26  Disable the configuration (UFM) access.

 Set Address            0xB4  Set the UFM sector 14-bit Address Register

 Init UFM Address       0x47  Reset to the Address Register to point to the
                              first UFM page (sector 1, page 0).

 Read UFM               0xCA  Read the UFM data. Operand specifies number
                              pages to read and number of dummy bytes to
                              prepend. Address Register is post-incremented.

 Erase UFM              0xCB  Erase the UFM sector only.

 Program UFM            0xC9  Write one page of data to the UFM. Address
                              Register is post-incremented.

-----------------------------------------------------------------------------
Config Flash (Sector 0) Commands
-----------------------------------------------------------------------------

 Read Device ID code    0xE0  Read the 4-byte Device ID (0x01 2b 20 43)

 Read USERCODE          0xC0  Read 32-bit USERCODE

 Read Status Reg        0x3C  Read the 4-byte Configuration Status Register

 Check Busy Flag        0xF0  Read the Configuration Busy Flag status

 Refresh                0x79  Launch boot sequence (same as toggling PROGRAMN)

 Flash Check            0x7D  This reads the on-chip config Flash bitstream
                              and checks the CRC of the Flash bits, without
                              actually writing the bits to the configuration
                              SRAM. (This is done in the background during
                              normal device operation). Query the Flash Check
                              Status bits of the Status register for result.

 Bypass                 0xFF  Null operation.

 Enable Config I'face   0x74  Enable Transparent Configuration Flash access -
 (Transparent Mode)           All user I/Os (except the hardened user SPI port)
                              are governed by the user logic, the device
                              remains in User mode. (The subsequent commands
                              in this table require the interface to be
                              enabled.)

 Enable Config I'face   0xC6  Enable Offline Configuration Flash access -
 (Offline Mode)               All user I/Os (except persisted sysCONFIG ports)
                              are tri-stated. User logic ceases to function,
                              UFM remains accessible, and the device enters
                              ‘Offline’ access mode. (The subsequent commands
                              in this table require the interface to be
                              enabled.)

 Disable Config I'face  0x26  Exit access mode.

 Set Address            0xB4  Set the 14-bit Address Register

 Verify Device ID       0xE2  Verify device ID with 32-bit input, set Fail
                              flag if mismatched.

 Init CFG Address       0x46  Reset to the Address Register to point to the
                              first Config flash page (sector 0, page 0).

 Read Config Flash      0x73  Read the Config Flash data. Operand specifies
                              number pages to read and number of dummy bytes
                              to prepend. Address Register is post-incremented.

 Erase Flash            0x0E  Erase the Config Flash, Done bit, Security bits
                              and USERCODE

 Program Config Flash   0x70  Write 1 page of data to the Config Flash.
                              Address Register is post-incremented.

 Program DONE           0x5E  Program the Done bit

 Program SECURITY       0xCE  Program the Security bit (Secures CFG Flash
                              sector)

 Program SECURITY PLUS  0xCF  Program the Security Plus bit
                              (Secures UFM Sector)
                              (only valid when Security bit is also set)

 Program USERCODE       0xC2  Program 32-bit USERCODE

-----------------------------------------------------------------------------
Non-Volatile Register (NVR) Commands
-----------------------------------------------------------------------------

 Read Trace ID code     0x19  Read 64-bit TraceID.
-----------------------------------------------------------------------------
*/

#include <assert.h>
#include <time.h>

#if defined _WIN32
  #include <windows.h>        /* for Sleep() */
#endif

#include "cp2112.h"
#include "tif.h"

#define ISC_ERASE               0x0e
#define ISC_DISABLE             0x26
#define ISC_INIT_CFG_ADDR       0x46
#define ISC_INIT_UFM_ADDR       0x47
#define ISC_PROG_DONE           0x5e
#define ISC_PROG_CFG_INCR       0x70
#define ISC_READ_CFG_INCR       0x73
#define ISC_ENABLE_X            0x74
#define ISC_REFRESH             0x79
#define ISC_ENABLE_PROG         0xc6
#define ISC_PROG_UFM_INCR       0xc9
#define ISC_READ_UFM_INCR       0xca
#define ISC_ERASE_UFM           0xcb
#define LSC_WRITE_ADDRESS       0xb4

#define BYPASS                  0xff
#define CHECK_BUSY_FLAG         0xf0

#define READ_DEVICE_ID_CODE     0xe0
#define READ_STATUS_REG         0x3c
#define READ_TRACE_ID_CODE      0x19

#define READ_USERCODE           0xc0
#define ISC_PROGRAM_USERCODE    0xc2

static const int MICROSEC = 1000;              // nanosecs
static const int MILLISEC = 1000 * MICROSEC;   // nanosecs

//---------------------------------------------------------------------
void Ttif::_nanosleep(uint32_t ns) {
#if defined _WIN32
  uint32_t ms = ns/MILLISEC;
  Sleep((ms>0) ? ms : 1);
#else
  struct timespec tim;
  tim.tv_sec = 0;
  tim.tv_nsec = (long)ns;
  nanosleep(&tim, NULL);
#endif
  }

//---------------------------------------------------------------------
bool Ttif::_cpDataWriteCfg(THidWrBuf& oBuf, int& Awritten) {
  return pCP->dataWrite(I2C_CFG_WR_ADDR, oBuf.data(), oBuf.length(), Awritten);
  }

//---------------------------------------------------------------------
bool Ttif::_cpDataWriteReadRequest(THidWrBuf& oBuf, int ArdCount) {
  return pCP->dataWriteReadRequest(I2C_CFG_WR_ADDR, ArdCount, oBuf);
  // false for errors
  }

//---------------------------------------------------------------------
bool Ttif::_cpReadDword(int AreportNum, THidRdBuf& iBuf) {
  const int numBytesToRead = 4;
  int status=0, numRead=0;
  bool ok = pCP->dataReadResponse(AreportNum, status, iBuf.data(),
                                                    numBytesToRead, numRead);
  iBuf.setLength(numRead);
  return ok && (numRead == numBytesToRead);
  }

//---------------------------------------------------------------------
bool Ttif::_cpReadDword(int AreportNum, uint32_t& v) {
  v = 0;
  THidRdBuf iBuf;
  if (!_cpReadDword(AreportNum, iBuf))
    return false;
  v = iBuf.dwordBE();
  return true;
  }

//---------------------------------------------------------------------
bool Ttif::_cpReadDword(int AreportNum, uint8_t* p) {
  assert(p);
  THidRdBuf iBuf;
  if (!_cpReadDword(AreportNum, iBuf))
    return false;
  for (int i=0; i<4; i++)
    p[i] = iBuf.byte();
  return true;
  }

//---------------------------------------------------------------------
bool Ttif::getDeviceIdCode(uint32_t& v) {
  v = 0;
  THidWrBuf oBuf;
  oBuf.byte(READ_DEVICE_ID_CODE).byte(0).byte(0).byte(0);
  bool rrOK = _cpDataWriteReadRequest(oBuf, 4);
  if (!rrOK)
    return false;

  return _cpReadDword(HID_DATA_READ_RESPONSE, v);
  }

//---------------------------------------------------------------------
bool Ttif::getStatusReg(uint32_t& v) {
  v = 0;
  THidWrBuf oBuf;
  oBuf.byte(READ_STATUS_REG).byte(0).byte(0).byte(0);
  bool rrOK = _cpDataWriteReadRequest(oBuf, 4);
  if (!rrOK)
    return false;

  return _cpReadDword(HID_DATA_READ_RESPONSE, v);
  }

//---------------------------------------------------------------------
int Ttif::getTraceId(uint8_t* p) {
  assert(p);

  const int numBytesToRead = 8;
  for (int i=0; i<numBytesToRead; i++)
    p[i] = 0;

  THidWrBuf oBuf;
  oBuf.byte(READ_TRACE_ID_CODE).byte(0).byte(0).byte(0);

  bool rrOK = _cpDataWriteReadRequest(oBuf, numBytesToRead);
  if (!rrOK)
    return -1;

  int status=0, numRead=0;
  bool ok = pCP->dataReadResponse(HID_DATA_READ_RESPONSE, status, p,
                                                    numBytesToRead, numRead);
  return (ok && (numRead == numBytesToRead)) ? numBytesToRead : -1;
  }

//---------------------------------------------------------------------
int Ttif::_doSimple(int Acmd, int Ap0) {
  THidWrBuf oBuf;
  oBuf.byte(Acmd).byte(Ap0).byte(0).byte(0);
  int written = 0;
  _cpDataWriteCfg(oBuf, written);
  return written;
  }

//---------------------------------------------------------------------
int Ttif::initCfgAddr() {
  return _doSimple(ISC_INIT_CFG_ADDR);
  }

//---------------------------------------------------------------------
int Ttif::_initUfmAddr() {
  return _doSimple(ISC_INIT_UFM_ADDR);
  }

int Ttif::_setUfmPageAddr(int pageNumber) {
  THidWrBuf oBuf;
  int hi = (pageNumber >> 8) & 0xff;
  int lo = (pageNumber >> 0) & 0xff;
  oBuf.byte(LSC_WRITE_ADDRESS).byte(0).byte(0).byte(0).byte(0x40).byte(0)
                                                          .byte(hi).byte(lo);
  int written = 0;
  _cpDataWriteCfg(oBuf, written);
  return written;
  }

int Ttif::progDone() {
  int ok = _doSimple(ISC_PROG_DONE);
  // sleep for 200us
  _nanosleep(200 * MICROSEC);
  return ok;
  }

int Ttif::refresh() {
  THidWrBuf oBuf;
  oBuf.byte(ISC_REFRESH).byte(0).byte(0);
  int written = 0;
  _cpDataWriteCfg(oBuf, written);
  // sleep for 5ms
  _nanosleep(5 * MILLISEC);
  return written;
  }

int Ttif::erase(int Amask) {
  int ok = _doSimple(ISC_ERASE, Amask);
  waitUntilNotBusy(-1);
  return ok;
  }

//---------------------------------------------------------------------
int Ttif::eraseCfg()  { return erase(CFG_ERASE); }
int Ttif::eraseAll()  { return erase(UFM_ERASE | CFG_ERASE | FEATURE_ERASE); }

int Ttif::eraseUfm() {
  return _doSimple(ISC_ERASE_UFM);
  }

//---------------------------------------------------------------------
int Ttif::enableCfgInterfaceOffline() {
  int ok = _doSimple(ISC_ENABLE_PROG, 0x08);
  _nanosleep(5 * MICROSEC);
  return ok;
  }

int Ttif::enableCfgInterfaceTransparent() {
  int ok = _doSimple(ISC_ENABLE_X, 0x08);
  _nanosleep(5 * MICROSEC);
  return ok;
  }

int Ttif::disableCfgInterface() {
  waitUntilNotBusy(-1);

  THidWrBuf oBuf;
  oBuf.byte(ISC_DISABLE).byte(0).byte(0);

  int written = 0;
  bool res = _cpDataWriteCfg(oBuf, written);

  oBuf.clear().byte(BYPASS).byte(0xff).byte(0xff).byte(0xff);

  res &= _cpDataWriteCfg(oBuf, written);
  return written;
  }

//---------------------------------------------------------------------
int Ttif::_progPage(int Acmd, const uint8_t *p) {
  THidWrBuf oBuf;
  oBuf.byte(Acmd).byte(0).byte(0).byte(1);
  for (int i=0; i<CFG_PAGE_SIZE; i++)
    oBuf.byte(*p++);

  int written = 0;
  /*bool res = */ _cpDataWriteCfg(oBuf, written);
  // sleep for 200us
  _nanosleep(200 * MICROSEC);
  return written;
  }

//---------------------------------------------------------------------
int Ttif::_readPages(int Acmd, int numPages, uint8_t *p) {
  assert((numPages > 0) && (p != 0));                   // TODO - max pages = 3

  THidWrBuf oBuf;
  int extraPage  = (numPages > 1) ? 1 : 0;
  int totalPages = numPages+extraPage;
  oBuf.byte(Acmd).byte(0x10).wordBE(totalPages);

  const int numBytesToRead = CFG_PAGE_SIZE * totalPages;
  bool rrOK = _cpDataWriteReadRequest(oBuf, numBytesToRead);
  if (!rrOK)
    return -1;

  THidRdBuf iBuf;
  int status=0, numRead=0;
  bool ok = pCP->dataReadResponse(HID_DATA_READ_RESPONSE, status, iBuf.data(),
                                                    numBytesToRead, numRead);
  if ((!ok) || (numRead != numBytesToRead))
    return -1;

  if (extraPage != 0)
    for (int i=0; i<CFG_PAGE_SIZE; i++)
      iBuf.byte();

  int numBytesToCpyOut = CFG_PAGE_SIZE * numPages;
  for (int i=0; i<numBytesToCpyOut; i++)
    p[i] = iBuf.byte();
  return numBytesToCpyOut;
  }

//---------------------------------------------------------------------
int Ttif::progCfgPage(const uint8_t *p) {
  return _progPage(ISC_PROG_CFG_INCR, p);
  }

int Ttif::readCfgPages(int numPages, uint8_t *p) {
  return _readPages(ISC_READ_CFG_INCR, numPages, p);
  }

int Ttif::_progUfmPage(const uint8_t *p) {
  return _progPage(ISC_PROG_UFM_INCR, p);
  }

int Ttif::readUfmPages(int numPages, uint8_t *p) {
  return _readPages(ISC_READ_UFM_INCR, numPages, p);
  }

int Ttif::readUfmPages(int pageNumber, int numPages, uint8_t *p) {
  int res = enableCfgInterfaceTransparent();
//waitUntilNotBusy();

  res = _setUfmPageAddr(pageNumber);
  res = readUfmPages(numPages, p);

  waitUntilNotBusy();
  res = progDone();
  waitUntilNotBusy();
  res = disableCfgInterface();
  return res;
  }

int Ttif::writeUfmPages(int pageNumber, int numPages, uint8_t *p) {
  int res = enableCfgInterfaceTransparent();
//waitUntilNotBusy();

  res = _setUfmPageAddr(pageNumber);
  for (int i=0; i<numPages; i++)
    res = _progUfmPage(p + UFM_PAGE_SIZE*i);

  waitUntilNotBusy();
  res = progDone();
  waitUntilNotBusy();
  res = disableCfgInterface();
  return res;
  }

//---------------------------------------------------------------------
int Ttif::setUsercode(uint8_t* p) {
  assert(p);
  THidWrBuf oBuf;
  oBuf.byte(ISC_PROGRAM_USERCODE).byte(0).byte(0).byte(0);
  for (int i=0; i<4; i++)
    oBuf.byte(p[i]);

  int written = 0;
  /*bool res = */ _cpDataWriteCfg(oBuf, written);
  _nanosleep(200 * MICROSEC);
  return written;
  }

int Ttif::getUsercode(uint8_t* p) {
  assert(p);
  THidWrBuf oBuf;
  const int numBytesToRead = 4;

  oBuf.byte(READ_USERCODE).byte(0).byte(0).byte(0);
  bool rrOK = _cpDataWriteReadRequest(oBuf, numBytesToRead);
  if (!rrOK)
    return -1;

  bool ok = _cpReadDword(HID_DATA_READ_RESPONSE, p);
  return (ok ? numBytesToRead : -1);
  }

//---------------------------------------------------------------------
int Ttif::getBusyFlag(int *pFlag) {
  *pFlag = 1;
  THidWrBuf oBuf;
  const int numBytesToRead = 1;

  oBuf.byte(CHECK_BUSY_FLAG).byte(0).byte(0).byte(0);
  bool rrOK = _cpDataWriteReadRequest(oBuf, numBytesToRead);
  if (!rrOK)
    return -1;

  int status=0, numRead=0;
  uint8_t flag = 0;
  bool ok = pCP->dataReadResponse(HID_DATA_READ_RESPONSE, status, &flag,
                                                    numBytesToRead, numRead);
  if ((!ok) || (numRead != numBytesToRead))
    return -1;

  *pFlag = (flag >> 7) & 1;
  return numBytesToRead;
  }

//---------------------------------------------------------------------
bool Ttif::_isBusy() {
  int busyFlag = 0;
  getBusyFlag(&busyFlag);
  return (busyFlag == 1);
  }

bool Ttif::waitUntilNotBusy(int maxLoops) {
  int i;
  for (i=0; (maxLoops<0) || (i<maxLoops); i++)
    if (_isBusy() == false)
      return true;
  return false;
  }

//---------------------------------------------------------------------
bool Ttif::appRead(uint8_t *p, int AnumBytes, int& AnumRead) {
  AnumRead = 0;
  if (AnumBytes <= 0)
    return true;

  assert((AnumBytes <= CP_MAX_READBACK) && (p != 0));

  pCP->AutoSendOn(true);

  int res = pCP->dataReadRequest(I2C_APP_WR_ADDR, AnumBytes);
  if (res<6)
    return false;

  int status=0;
  bool ok =
    pCP->dataReadResponse(HID_DATA_READ_RESPONSE, status, p, AnumBytes, AnumRead);

  return ok && (AnumRead == AnumBytes);
  }

//---------------------------------------------------------------------
bool Ttif::appWrite(uint8_t *p, int AnumBytes, int& AnumWritten) {
  AnumWritten = 0;
  if (AnumBytes <= 0)
    return true;

  assert(p != 0);

  return pCP->dataWrite(I2C_APP_WR_ADDR, p, AnumBytes, AnumWritten);
  }

//---------------------------------------------------------------------
Ttif::Ttif(int Avid, int Apid) {
  pCP = new Tcp2112;
  FinitResult = (int)(pCP->init(Avid, Apid));
  }

Ttif::~Ttif() {
  delete pCP;
  }

// EOF ----------------------------------------------------------------
