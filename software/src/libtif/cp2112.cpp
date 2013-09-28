// cp2112.cpp ---------------------------------------------------------
//
// Copyright (c) 2001 to 2013  te
//
// Licence: Creative Commons Attribution-ShareAlike 3.0 Unported License.
//          http://creativecommons.org/licenses/by-sa/3.0/
//---------------------------------------------------------------------

#include <assert.h>

#ifdef  _DEBUG
# include <stdio.h>
#endif

#include <algorithm>

#include "hidapi.h"
#include "cp2112.h"

#define HID_MAX_RETRIES             5

#define PIN_Spare_1                 (1 << 0)
#define g_ucPinGSRn                 (1 << 1)
#define g_ucPinENABLE               (1 << 2)     /* JTAGENA */
#define g_ucPinTCK                  (1 << 3)
#define g_ucPinTMS                  (1 << 4)
#define g_ucPinTDO                  (1 << 5)
#define g_ucPinTDI                  (1 << 6)
#define g_ucPinK24                  (1 << 7)

#define CP_OUTMASK                  ( PIN_Spare_1    & 0xff) |  /* out */  \
                                    ( g_ucPinGSRn    & 0xff) |  /* out */  \
                                    ( g_ucPinENABLE  & 0xff) |  /* out */  \
                                    ( g_ucPinTCK     & 0xff) |  /* out */  \
                                    ( g_ucPinTMS     & 0xff) |  /* out */  \
                                    ( g_ucPinTDO     &    0) |  /* in  */  \
                                    ( g_ucPinTDI     & 0xff) |  /* out */  \
                                    ( g_ucPinK24     & 0xff)    /* out */

#define CP_PUSH_PULL                0xff   /* all output pins are push-pull */
#define CP_CLOCK_DIVIDER            1      /* gives 48/(2*n) = 24MHz        */
#define CP_SPECIAL_PINS             1      /* bit 0 : clock on GPIO_7       */

#define CP_I2C_CLOCK_SPEED          (400 * 1000)
#define CP_MAX_RETRIES              5

/*
CP2112 HID Reports

-------------------------------------------------------
Device Configuration (Feature Request)
-------------------------------------------------------
0x01  Reset Device
0x02  Get/Set GPIO Configuration
0x03  Get GPIO
0x04  Set GPIO
0x05  Get Version Information
0x06  Get/Set SMBus Configuration

-------------------------------------------------------
Data Transfer (Interrupt Transfer)
-------------------------------------------------------
0x10  Data Read Request
0x11  Data Write Read Request
0x12  Data Read Force Send
0x13  Data Read Response
0x14  Data Write
0x15  Transfer Status Request
0x16  Transfer Status Response
0x17  Cancel Transfer

-------------------------------------------------------
USB Customization (Feature Requests)
-------------------------------------------------------
0x20  Get/Set Lock Byte
0x21  Get/Set USB Configuration
0x22  Get/Set Manufacturing String
0x23  Get/Set Product String
0x24  Get/Set Serial String
*/

#define HID_TIMEOUT_MILLISECS             2

//---------------------------------------------------------------------
int Tcp2112::_getReport(uint8_t *p, int AbufSize) {
  assert(AbufSize == HID_RD_BUFF_SIZE);
  assert(p != 0);

  int len = -1;
  if (FhidHandle) {
    len = hid_read_timeout((hid_device *)FhidHandle, p, AbufSize,
                                                HID_TIMEOUT_MILLISECS);
    }
  return len;
  }

//-------------------------------------------
int Tcp2112::_sendReport(uint8_t *p, int AbufSize) {
  if (AbufSize <= 0)
    return 0;

  assert(AbufSize <= HID_WR_BUFF_SIZE);
  assert(p != 0);

  int len = -1;
  if (FhidHandle) {
    len = hid_write((hid_device *)FhidHandle,  p, AbufSize);  // -1 for errs
    }
  return len;
  }

//-------------------------------------------
void Tcp2112::flushIncomingReports() {
  if (FhidHandle==0)
    return;

  uint8_t buf[HID_RD_BUFF_SIZE * 8];
  hid_set_nonblocking((hid_device *)FhidHandle, 1);
  while (1) {
    memset(buf, 0, sizeof(buf));
buf[0] = 0x13;
    int len = hid_read((hid_device *)FhidHandle, buf, sizeof(buf));
    if (len <= 0)
      break;

#ifdef  _DEBUG
    printf("\n(%2d)\t", len);
    for (int i=0; i<len; i++) {
      printf("%02x",buf[i]);
      if ((i+1) >= len)
        continue;
      else if ((i % 16) == 15)
        printf("\n\t");
      else if ((i % 8) == 7)
        printf("_");
      else
        printf(",");
      }
#endif

    }
  hid_set_nonblocking((hid_device *)FhidHandle, 0);
  }

//-------------------------------------------
int Tcp2112::_getFeatureReport(THidRdBuf& Abuf, int len) {
  int res = -1;
  if (FhidHandle && (len > 0)) {
    assert(len < HID_RD_BUFF_SIZE);
    res = hid_get_feature_report((hid_device *)FhidHandle, Abuf.data(), len+1);
    }
  return res;
  }

int Tcp2112::_sendFeatureReport(THidWrBuf& Abuf) {
  int res = -1;
  if (FhidHandle) {
    int len = (int)Abuf.length();
    if (len > 0)
      res = hid_send_feature_report((hid_device *)FhidHandle, Abuf.data(), len);
    }
  Abuf.clear();
  return res;
  }

//-------------------------------------------
// reserved feature report
int Tcp2112::_resetDevice() {
  THidWrBuf buf;
  buf.byte(HID_RESET_DEVICE).byte(0x01);
  return _sendFeatureReport(buf);
  }

//-------------------------------------------
// feature reports
int Tcp2112::getGpioConfig(int& Adir, int& ApushPull, int& Aspecial,
                                                                int& Aclk) {
  Adir      = 0;
  ApushPull = 0;
  Aspecial  = 0;
  Aclk      = 0;
  THidRdBuf buf(HID_GET_SET_GPIO_CONFIGURATION);
  int len = _getFeatureReport(buf, 4);
  if (len >= 5) {
    Adir      = buf.data()[1];
    ApushPull = buf.data()[2];
    Aspecial  = buf.data()[3];
    Aclk      = buf.data()[4];
    }
  return len;
  }

int Tcp2112::setGpioConfig(int Adir, int ApushPull, int Aspecial, int Aclk) {
  THidWrBuf buf;
  buf.byte(HID_GET_SET_GPIO_CONFIGURATION)
     .byte(Adir)
     .byte(ApushPull)
     .byte(Aspecial)
     .byte(Aclk);
  int res = _sendFeatureReport(buf);
  return res;
  }

int Tcp2112::getGpio(int& Aval) {
  Aval = 0;
  THidRdBuf buf(HID_GET_GPIO);
  int len = _getFeatureReport(buf, 1);
  if (len >= 2) {
    Aval = buf.data()[1];
    }
  return len;
  }

int Tcp2112::setGpio(int Aval, int Amask) {
  THidWrBuf buf;
  buf.byte(HID_SET_GPIO).byte(Aval).byte(Amask);
  int res = _sendFeatureReport(buf);
  return res;
  }

void Tcp2112::setGsrOn(bool v) {            // On is Lo
  int val = v ? 0 : g_ucPinGSRn;
  setGpio(val, g_ucPinGSRn);
  }

int Tcp2112::getVersion(int& ApartNumber, int& AdeviceVersion) {
  ApartNumber    = 0;
  AdeviceVersion = 0;
  THidRdBuf buf(HID_GET_VERSION_INFORMATION);
  int len = _getFeatureReport(buf, 2);
  if (len >= 3) {
    /* int reportID = */
                     buf.byte();
    ApartNumber    = buf.byte();
    AdeviceVersion = buf.byte();
    }
  return len;
  }

int Tcp2112::getSMbusConfiguration(int& AclockSpeed,
                                   int& AdeviceAddr,
                                   int& AautoSendRead,
                                   int& AwrTimeout,
                                   int& ArdTimeout,
                                   int& AsclLowTimeout,
                                   int& AretryTime) {
  THidRdBuf buf(HID_GET_SET_SMBUS_CONFIGURATION);
  int res = _getFeatureReport(buf, 13);
  /* int reportID = */
                   buf.byte();
  AclockSpeed    = buf.dwordBE();
  AdeviceAddr    = buf.byte();
  AautoSendRead  = buf.byte();
  AwrTimeout     = buf.wordBE();
  ArdTimeout     = buf.wordBE();
  AsclLowTimeout = buf.byte();
  AretryTime     = buf.wordBE();
  return res;
  }

int Tcp2112::setSMbusConfiguration(int AclockSpeed,
                                   int AdeviceAddr,
                                   int AautoSendRead,
                                   int AwrTimeout,
                                   int ArdTimeout,
                                   int AsclLowTimeout,
                                   int AretryTime) {
  THidWrBuf buf;
  buf.byte(HID_GET_SET_SMBUS_CONFIGURATION)
     .dwordBE(AclockSpeed)
     .byte   (AdeviceAddr)
     .byte   (AautoSendRead)
     .wordBE (AwrTimeout)
     .wordBE (ArdTimeout)
     .byte   (AsclLowTimeout)
     .wordBE (AretryTime);
  int res = _sendFeatureReport(buf);
  return res;
  }

//-------------------------------------------
// interrupt transfers
int Tcp2112::dataReadRequest(int AslaveAddr, int Alen) {
  int res = 0;
  assert(((AslaveAddr & 1) == 0) && (Alen <= CP_MAX_READBACK));
  if (Alen > 0) {
    THidWrBuf xbuf;
    xbuf.byte(HID_DATA_READ_REQUEST).byte(AslaveAddr).wordBE(Alen);
    res = _sendReport(xbuf);
    }
  return res;
  }

//-------------------------------------------
bool Tcp2112::dataWriteReadRequest(int AslaveAddr, int ArdLen,
                                                        THidWrBuf& AcmdBuf) {
  int cmdLen = AcmdBuf.length();
  assert(((AslaveAddr & 1)==0) && (ArdLen<=CP_MAX_READBACK) && (cmdLen==4));

  bool ok = true;

  if (ArdLen > 0) {
    uint8_t *pCmdBuf = AcmdBuf.data();
    THidWrBuf xbuf;
    xbuf.byte(HID_DATA_WRITE_READ_REQUEST).byte(AslaveAddr).wordBE(ArdLen)
                                                              .byte(cmdLen);
    for (int i=0; i<cmdLen; i++)
      xbuf.byte(*pCmdBuf++);

    int len = xbuf.length();
    int numSent = _sendReport(xbuf.data(), len);
    if (numSent < len)
      return false;
    ok = _dataReadForceSend(ArdLen);
    }
  return ok;
  }

//-------------------------------------------
bool Tcp2112::_dataReadForceSend(int Acount) {
  assert((Acount > 0) && (Acount <= CP_MAX_READBACK));

  bool ok = true;
  if (Acount > 0) {
    THidWrBuf xbuf;
    xbuf.byte(HID_DATA_READ_FORCE_SEND).wordBE(Acount);
    int len = xbuf.length();
    int numSent = _sendReport(xbuf.data(), len);
    ok = (numSent >= len);
    }
  return ok;
  }

//-------------------------------------------------
bool Tcp2112::dataReadResponse(int AreportNum, int& Astatus, uint8_t *pRdBuf,
                                              int ArdLen, int& AnumRead) {
  assert((AreportNum != 0) && (ArdLen > 0) && (ArdLen <= CP_MAX_READBACK));

  int needed = ArdLen;

  memset(pRdBuf, 0, ArdLen);
  Astatus  = 0;
  AnumRead = 0;

  uint8_t hidBuf[HID_RD_BUFF_SIZE];
  int failedReads = 0;

  while (failedReads < HID_MAX_RETRIES) {
    memset(hidBuf, 0, sizeof(hidBuf));
    hidBuf[0] = (uint8_t)AreportNum;
    int len = _getReport(hidBuf, sizeof(hidBuf));

    if (len < 2) {
      failedReads++;
      continue;
      }

    Astatus = hidBuf[1];
    int numRead = hidBuf[2];
    switch (Astatus) {
      case CP_STATUS_IDLE :
        failedReads++;
        break;

      case CP_STATUS_BUSY :
        if (numRead == 0) {
          failedReads++;
          break;
          }

        numRead = (numRead <= needed) ? numRead : needed ;
        for (int i=0; i<numRead; i++)
          *pRdBuf++ = hidBuf[i+3];
        needed -= numRead;
        AnumRead += numRead;
        if (needed <= 0) {
          Astatus = CP_STATUS_COMPLETE;                       // TODO - ??
          return true;
          }
        break;

      case CP_STATUS_COMPLETE :
        numRead = (numRead <= needed) ? numRead : needed ;
        for (int i=0; i<numRead; i++)
          *pRdBuf++ = hidBuf[i+3];
        needed -= numRead;
        AnumRead += numRead;
        if (needed <= 0)
          return true;
        break;

      case CP_STATUS_COMPLETE_ERROR :
        return false;

      }
    }

  return false;             // too many failed reads
  }

//-------------------------------------------------
bool Tcp2112::dataWrite(int AslaveAddr,  uint8_t *pwrData, int wrDataLen,
                                                      int& AnumWritten) {
  if (wrDataLen <= 0)
    return true;

  assert((AslaveAddr & 1) == 0);    // write addr

  AnumWritten = 0;
  int needed = wrDataLen;
  uint8_t buf[HID_WR_BUFF_SIZE];
  int failedWrites = 0;

  while (failedWrites < HID_MAX_RETRIES) {
    int num = std::min(needed, HID_WR_BUFF_SIZE-3);
    memset(buf, 0, sizeof(buf));
    buf[0] = HID_DATA_WRITE;
    buf[1] = (uint8_t)AslaveAddr;
    buf[2] = (uint8_t)num;
    for (int i=0; i<num; i++)
      buf[i+3] = pwrData[i];

    int res = _sendReport(buf, sizeof(buf));
    if (res < 0)
      return false;
    needed -= num;
    if (needed <= 0)
      return true;
    AnumWritten += num;
    pwrData     += num;
    }

  return false;             // too many failed writes
  }

//-------------------------------------------------
int Tcp2112::transferStatusRequest() {
  uint8_t buf[HID_WR_BUFF_SIZE];
  memset(buf, 0, sizeof(buf));
  buf[0] = HID_TRANSFER_STATUS_REQUEST;
  buf[1] = 1;
  return _sendReport(buf, sizeof(buf));
  }

int Tcp2112::transferStatusResponse(int* Astatus) {
  Astatus[0] = Astatus[1] = Astatus[2] = Astatus[3] = 0;

  uint8_t buf[HID_WR_BUFF_SIZE];
  buf[0] = HID_TRANSFER_STATUS_RESPONSE;
  int res = _getReport(buf, sizeof(buf));

  Astatus[0] = buf[1];
  Astatus[1] = buf[2];
  Astatus[2] = (buf[3] << 8) | buf[4];
  Astatus[3] = (buf[5] << 8) | buf[6];
  return res;
  }


int Tcp2112::cancelTransfer() {
  uint8_t buf[HID_WR_BUFF_SIZE];
  memset(buf, 0, sizeof(buf));
  buf[0] = HID_CANCEL_TRANSFER;
  buf[1] = 1;
  return _sendReport(buf, sizeof(buf));
  }

//---------------------------------------------------------------------
Tcp2112::Tcp2112() : FhidHandle(0) {
  }

//---------------------------------------------------------------------
bool Tcp2112::_resetI2C() {
  int written;
  uint8_t cmdBuf = 0;
  return dataWrite(I2C_RESET_WR_ADDR, &cmdBuf, 1, written);
  }

//---------------------------------------------------------------------
void *Tcp2112::init(int vid, int pid, wchar_t *Aname) {
  FhidHandle = NULL;
  int res = hid_init();
  if (res < 0)
    return HID_INIT_FAIL;

  // Open the device using the VID, PID, and optionally the Serial number.
  FhidHandle = (void *)hid_open(vid, pid, Aname);

  if (FhidHandle == 0)
    return HID_OPEN_FAIL;

  /*int res = */setGpioConfig(CP_OUTMASK, CP_PUSH_PULL, CP_SPECIAL_PINS,
                                                          CP_CLOCK_DIVIDER);

  int clockSpeed    = 0;
  int deviceAddr    = 0;
  int autoSendRead  = 0;
  int wrTimeout     = 0;
  int rdTimeout     = 0;
  int sclLowTimeout = 0;
  int retryTime     = 0;

  /*res = */
        getSMbusConfiguration(clockSpeed,
                              deviceAddr,
                              autoSendRead,
                              wrTimeout,
                              rdTimeout,
                              sclLowTimeout,
                              retryTime       );

  /* res = */
        setSMbusConfiguration(CP_I2C_CLOCK_SPEED,
                              deviceAddr,
                              0,                // TODO
                              wrTimeout,
                              rdTimeout,
                              sclLowTimeout,
                              CP_MAX_RETRIES  );

  FautoSendIsOn = false;                        // TODO here as well!

  setGsrOn(true);
  setGsrOn(false);
  _resetI2C();

  return FhidHandle;
  }

//---------------------------------------------------------------------
void Tcp2112::finalise() {
  if (FhidHandle)
    hid_close((hid_device *)FhidHandle);
  FhidHandle = 0;
  hid_exit();
  }

//---------------------------------------------------------------------
void Tcp2112::AutoSendOn(bool v) {
  if (v == FautoSendIsOn)
    return;

  int clockSpeed    = 0;
  int deviceAddr    = 0;
  int autoSendRead  = 0;
  int wrTimeout     = 0;
  int rdTimeout     = 0;
  int sclLowTimeout = 0;
  int retryTime     = 0;

  /* int res = */
        getSMbusConfiguration(clockSpeed,
                              deviceAddr,
                              autoSendRead,
                              wrTimeout,
                              rdTimeout,
                              sclLowTimeout,
                              retryTime       );

  bool isOn = (autoSendRead != 0);
  if (v != isOn) {
    autoSendRead = v ? 1 : 0;
    /* res = */
          setSMbusConfiguration(clockSpeed,
                                deviceAddr,
                                autoSendRead,
                                wrTimeout,
                                rdTimeout,
                                sclLowTimeout,
                                retryTime       );
    }
  FautoSendIsOn = v;
  }

//---------------------------------------------------------------------
Tcp2112::~Tcp2112() {
  finalise();
  }

// EOF ----------------------------------------------------------------
