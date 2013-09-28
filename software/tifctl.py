#----------------------------------------------------------------------
# Name:        tifctl.py
# Purpose:     control a tif board via the hidapi DLL/SO
#
# Author:      Tim
#
# Created:     01/07/2013
# Copyright:   (c) Tim 2013
# Licence:     Creative Commons Attribution-ShareAlike 3.0 Unported License.
#----------------------------------------------------------------------
# uses Austin Morton's pyhidapi package.
# See  https://github.com/Juvenal1228/pyhidapi
#
#!/usr/bin/env python

import sys, ctypes, tifglobs
from ctypes   import *
from pyhidapi import hid
from tifglobs import *

##---------------------------------------------------------
def notBusy(handle):
  tifglobs.tif.waitUntilNotBusy(handle, DEFAULT_BUSY_LOOPS)

##---------------------------------------------------------
def findTif():
  found = False;
  try:
    devs = hid.enumerate()
    for dev in devs:
      s  = "Manufacturer:%s" % dev['manufacturer_string']
      print(s)
      s  = "  Product:%s" % dev['product_string']
      print(s)
      vid = dev['vendor_id']
      pid = dev['product_id']
      s  = "  VID:" + ("%04X" % vid)
      s += "  PID:" + ("%04X" % pid)

      if (vid==VID_HID) and (pid==PID_HID):
        found = True

      # test for a null serial number with just a language ID
      serialNumber = dev['serial_number']
      if serialNumber == u'\u0409':
        s += "  no Serial Number"
      else:
        s += "  Serial Number:" + repr(serialNumber)
      print(s)
    print("\n")

  except:
    found = False;

  return found

##---------------------------------------------------------
def print4(v):
  x = list(v.raw)
  s = ''
  for i in range(0, 4):
    if i==0:
      s += ''
    else:
      s+= '.'
    s += ('%02X' % ord(x[i]))
  print(s)

##---------------------------------------------------------
def showDeviceID(handle):

  dw = c_ulong(0xdeadbeef)
  res = tifglobs.tif.getDeviceIdCode(handle, byref(dw))

  if (res == 0):
    print("\nread ID code failed\n")
    return "failed"

  deviceID = dw.value
  print('XO2 Device ID: %08x' % deviceID) ,

  s = "unrecognised"
  ok = (deviceID & 0xffff8fff) == (0x012ba043 & 0xffff8fff)
  model = (deviceID >> 12) & 7;

  if model == 0 :
    s = "XO2-256HC"
  elif model == 1 :
    s = "XO2-640HC"
  elif model == 2 :
    s = "XO2-1200HC"
  elif model == 3 :
    s = "XO2-2000HC"
  elif model == 4 :
    s = "XO2-4000HC"
  elif model == 5 :
    s = "XO2-7000HC"
  else:
    s = "unrecognised"
    ok = false;

  if ok == True:
    print(" - device is an " + s)
  else:
    print(" - unrecognised ID!")

  return s;

##---------------------------------------------------------
## one byte programmable, seven bytes are die ID
def showTraceID(handle):
  buff = create_string_buffer(8)
  res = tifglobs.tif.getTraceId(handle, buff)

  s = "XO2 Trace ID : "
  tid = list(buff.raw)
  for i in range(0, 8):
    if i==0:
      s += ''
    elif i==4:
      s += '_'
    else:
      s+= '.'
    s += ('%02X' % ord(tid[i]))
  print(s)
  return (res >= 0)

##---------------------------------------------------------
## EN on  - read from CFG Flash
## EN off - read from SRAM
def showUsercode(handle):
  buff = create_string_buffer(4)
  res = tifglobs.tif.doEnableCfgInterfaceTransparent(handle)
  notBusy(handle)
  res = tifglobs.tif.getUsercode(handle, buff)
  res = tifglobs.tif.doDisableCfgInterface(handle)
  print("XO2 usercode from Flash: ") ,
  print4(buff)

  notBusy(handle)
  res = tifglobs.tif.getUsercode(handle, buff)
  print("XO2 usercode from SRAM : ") ,
  print4(buff)

##---------------------------------------------------------
def sendAddressByte(handle, a):
  try:
    cmdLength = 1
    buff = create_string_buffer(chr(ADDRESS_MASK | a), cmdLength)
    numWritten = c_ulong(0)
    res = tifglobs.tif.appWrite(handle, buff, cmdLength, byref(numWritten))
  except:
    print('FAILED: address byte send')

##---------------------------------------------------------
def sendDataByte(handle, v):
  try:
    cmdLength = 1
    buff = create_string_buffer(chr(DATA_MASK | v), cmdLength)
    numWritten = c_ulong(0)
    res = tifglobs.tif.appWrite(handle, buff, cmdLength, byref(numWritten))
  except:
    print('FAILED: data byte send')

##---------------------------------------------------------
# read the ID register inside the FPGA
def showIDregister(handle):
  try:
    sendAddressByte(handle, R_ID)
    numWanted = 16
    buff = create_string_buffer(numWanted)
    numRead = c_ulong(0)
    res = tifglobs.tif.appRead(handle, buff, numWanted, byref(numRead))
    print(repr(buff.value))
  except:
    pass

##---------------------------------------------------------
# write val into the Misc register inside the FPGA
def setMiscRegister(handle, val):
  try:
    sendAddressByte(handle, W_MISC_REG)
    sendDataByte(handle, val)
  except:
    pass

##---------------------------------------------------------
def main():
  print("====================hello==========================")
  handle = None
  try:
    tifglobs.tif = ctypes.CDLL("libtif.dll")

    strBuf = create_string_buffer(1000)
    rv = tifglobs.tif.version(strBuf, sizeof(strBuf))
    print('Using tif library version: %s\n' % repr(strBuf.value))

    if findTif():
      handle = c_int(tifglobs.tif.tifInit(VID_HID, PID_HID))
      dev = showDeviceID(handle)
      showTraceID(handle)
      showUsercode(handle)
      showIDregister(handle)
      # The change in the Misc register will show up in the third byte of
      # the ID register, but you really need to run this under the control
      # of a debugger if you want to see the effect on the flashing LEDs.
      # The LEDs start out synchronized and end up alternating
      setMiscRegister(handle, LED_OFF)
      showIDregister(handle)
      setMiscRegister(handle, LED_SYNC)
      showIDregister(handle)
      setMiscRegister(handle, LED_ALTERNATING)
      showIDregister(handle)

  except:
    e = sys.exc_info()[0]
    print("\nException caught %s\n" % e)

  if handle:
    tifglobs.tif.tifClose(handle)
  print("\n==================== bye ==========================")

##---------------------------------------------------------
if __name__ == '__main__':
  main()

# EOF -----------------------------------------------------------------

