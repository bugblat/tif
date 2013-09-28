// cp2112.h -----------------------------------------------------------
//
// Copyright (c) 2001 to 2012  te
//
// Licence: Creative Commons Attribution-ShareAlike 3.0 Unported License.
//          http://creativecommons.org/licenses/by-sa/3.0/
//---------------------------------------------------------------------
#ifndef cp2112H
#define cp2112H

#include <stdint.h>
#include "hidbufs.h"

#define SILABS_VID                  0x10c4
#define CP2112_PID                  0xea90

#define VID_HID                     SILABS_VID
#define PID_HID                     CP2112_PID

#define HID_RESET_DEVICE                  0x01
#define HID_GET_SET_GPIO_CONFIGURATION    0x02
#define HID_GET_GPIO                      0x03
#define HID_SET_GPIO                      0x04
#define HID_GET_VERSION_INFORMATION       0x05
#define HID_GET_SET_SMBUS_CONFIGURATION   0x06

#define HID_DATA_READ_REQUEST             0x10
#define HID_DATA_WRITE_READ_REQUEST       0x11
#define HID_DATA_READ_FORCE_SEND          0x12
#define HID_DATA_READ_RESPONSE            0x13
#define HID_DATA_WRITE                    0x14
#define HID_TRANSFER_STATUS_REQUEST       0x15
#define HID_TRANSFER_STATUS_RESPONSE      0x16
#define HID_CANCEL_TRANSFER               0x17

#define HID_GET_SET_LOCK_BYTE             0x20
#define HID_GET_SET_USB_CONFIGURATION     0x21
#define HID_GET_SET_MANUFACTURING_STRING  0x22
#define HID_GET_SET_PRODUCT_STRING        0x23
#define HID_GET_SET_SERIAL_STRING         0x24

#define CP_STATUS_IDLE                    0     /* report 0x13 */
#define CP_STATUS_BUSY                    1
#define CP_STATUS_COMPLETE                2
#define CP_STATUS_COMPLETE_ERROR          3

#define CP_MAX_READBACK                   512

#define I2C_CFG_WR_ADDR                   0x80
#define I2C_CFG_RD_ADDR                   (I2C_CFG_WR_ADDR | 1)

#define I2C_APP_WR_ADDR                   0x82
#define I2C_APP_RD_ADDR                   (I2C_APP_WR_ADDR | 1)

#define I2C_RESET_WR_ADDR                 0x86

#define HID_INIT_FAIL                     ((void *)-1)
#define HID_OPEN_FAIL                     ((void *)-2)

//---------------------------------------------------------------------
class Tcp2112 {
  private:
    void  *FhidHandle;
    bool   FautoSendIsOn;

    int _getReport(uint8_t *pBuf, int AbufSize);
    int _sendReport(uint8_t *pBuf, int AbufSize);
    int _sendReport(THidWrBuf& buf) {
      return _sendReport(buf.data(), buf.length());
      }

    int _getFeatureReport(THidRdBuf& Abuf, int len);
    int _sendFeatureReport(THidWrBuf& Abuf);

    bool _dataReadForceSend(int Acount);

    //-------------------------------------------
    // reserved feature report
    int _resetDevice();

    bool _resetI2C();

  public:
    //-------------------------------------------
    // configuration feature reports
    int getGpioConfig(int& Adir, int& ApushPull, int& Aspecial, int& Aclk);
    int setGpioConfig(int Adir, int ApushPull, int Aspecial, int Aclk);
    int getGpio(int& Aval);
    int setGpio(int Aval, int Amask);
    int getVersion(int& ApartNumber, int& AdeviceVersion);
    int getSMbusConfiguration(int& AclockSpeed,
                              int& AdeviceAddr,
                              int& AautoSendRead,
                              int& AwrTimeout,
                              int& ArdTimeout,
                              int& AsclLowTimeout,
                              int& AretryTime);
    int setSMbusConfiguration(int AclockSpeed,
                              int AdeviceAddr,
                              int AautoSendRead,
                              int AwrTimeout,
                              int ArdTimeout,
                              int AsclLowTimeout,
                              int AretryTime);
    void AutoSendOn(bool v);
    void setGsrOn(bool v);

    //-------------------------------------------
    // interrupt transfers
    bool dataWrite(int AslaveAddr, uint8_t *pWrData, int AwrLen, int& AnumWritten);
    int dataReadRequest(int AslaveAddr, int Alen);
    bool dataWriteReadRequest(int AslaveAddr, int ArdLen, THidWrBuf& AcmdBuf);

    bool dataReadResponse(int AreportNum, int& Astatus, uint8_t *pRdBuf, int ArdLen,
                                              int& AnumRead);

    int transferStatusRequest();
    int transferStatusResponse(int* Astatus);
    int cancelTransfer();

    //-------------------------------------------
    Tcp2112();
    ~Tcp2112();

    void *init(int vid=VID_HID, int pid=PID_HID, wchar_t *Aname=0);
    void finalise();
    void flushIncomingReports();
  };

#endif

// EOF ----------------------------------------------------------------
