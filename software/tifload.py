#----------------------------------------------------------------------
# Name:        tifload.py
# Purpose:     load a configuration into a tif board via the hidapi DLL/SO
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

traceFile = None
DEVICE_NAME_TAG = 'NOTE DEVICE NAME:'
PACKAGE_TAG = 'CSBGA132'

##---------------------------------------------------------
def showCfgStatus(handle):
  ## tbd
  return True

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
def pageCount(model):
  if model == "XO2-1200HC":
    return CFG_PAGE_COUNT_1200
  elif model == "XO2-2000HC":
    return CFG_PAGE_COUNT_2000
  elif model == "XO2-4000HC":
    return CFG_PAGE_COUNT_4000
  elif model == "XO2-7000HC":
    return CFG_PAGE_COUNT_7000
  else:
    return 0

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
  res = tifglobs.tif.enableCfgInterfaceTransparent(handle)
  res = tifglobs.tif.getUsercode(handle, buff)
  res = tifglobs.tif.disableCfgInterface(handle)
  print("XO2 usercode from Flash: ") ,
  print4(buff)

  notBusy(handle)
  res = tifglobs.tif.getUsercode(handle, buff)
  print("XO2 usercode from SRAM : ") ,
  print4(buff)

##---------------------------------------------------------
def processLine(line):
  global traceFile
  l = line.strip('\n')
  n = len(l)
  v = []
  for i in range(0, CFG_PAGE_SIZE):
    s = line[(i*8):(i*8+8)]
    x = int(s, 2)
    v.append(x)
    if traceFile:
      traceFile.write('0x%02x,' % x)
      if (i % 4) == 3:
        traceFile.write(' ')
  if traceFile:
    traceFile.write('\n')
  return v

##---------------------------------------------------------
def readJedecFile(fname, dev):
  global traceFile
  print('JEDEC file is ' + fname)

  data = []
  correctDevice = False
  jedecID = None
  try:
    f = open(fname, 'r')
  except:
    print('Could not open ' + fname)
    return data

  print('starting to read JEDEC file ') ,
  lnum = 0;
  state = 'initial'

  for line in f:
    lnum += 1
    if (lnum % 250) == 0:
      print('.') ,

    if len(line) < 1:
      continue

    # check JEDEC for, e.g., NOTE DEVICE NAME:  LCMXO2-7000HC-4TQFP144*
    if DEVICE_NAME_TAG in line:
      jedecID = line.strip()
      correctDevice = (dev in line) and (PACKAGE_TAG in line)
      if not correctDevice:
        break

    c0 = line[0]
    valid = (c0=='0') or (c0=='1')
    if state == 'initial':
      if valid:
        print('\nfirst configuration data line: %d' % lnum)
        state = 'inData'
        v = processLine(line)
        data.append(v)
    elif state == 'inData':
      if valid:
        v = processLine(line)
        data.append(v)
      else:
        print('\nlast configuration data line: %d' % (lnum-1))
        state = 'finished'
        break

  f.close()
  if traceFile:
    traceFile.close()

  if not correctDevice:
    print('\nJEDEC file does not match FPGA')
    print('\n  FPGA is ' + dev)
    if jedecID:
      print('\n  JEDEC identifies as "' + jedecID + '"')
    return []

  print('%d frames' % len(data))
  print('finished reading JEDEC file')
  return data

##---------------------------------------------------------
def configure(handle, fname, dev):
  jedecData = readJedecFile(fname, dev)

  if len(jedecData) == 0:
    return

  tifglobs.tif.waitUntilNotBusy(handle, -1)
  showCfgStatus(handle)

  res = tifglobs.tif.enableCfgInterfaceOffline(handle)
  showCfgStatus(handle)

  print('erasing configuration flash ... ') ,
  res = tifglobs.tif.initCfgAddr(handle)
  res = tifglobs.tif.eraseCfg(handle)
  showCfgStatus(handle)
  print('erased')

  res = tifglobs.tif.initCfgAddr(handle)

  print('programming configuration flash ... '),
  frameData = create_string_buffer(CFG_PAGE_SIZE)
  numPages = len(jedecData);
  for pageNum in range(0, numPages) :
    frame = jedecData[pageNum]
    for i in range(0, CFG_PAGE_SIZE) :
      frameData[i] = chr(frame[i])
    res = tifglobs.tif.progCfgPage(handle, frameData)
    if (pageNum % 25) == 0:
      print('.') ,

  print('programmed \ntransferring ...  ')

  res = tifglobs.tif.progDone(handle)
  res = tifglobs.tif.refresh(handle)
  res = tifglobs.tif.disableCfgInterface(handle)
  showCfgStatus(handle)
  print('configuration finished.')

##---------------------------------------------------------
def main():
  print("====================hello==========================")
  handle = None
  jedecFile = None
  try:
    try:
      jedecFile = sys.argv[1]
      print('Configuration file is ' + jedecFile)
    except:
      print('pifload.py <JEDEC_File>')
      jedecFile = None
      #sys.exit(2)


    try:
      tifglobs.tif = ctypes.CDLL('libtif.dll')
    except OSError:
      tifglobs.tif = ctypes.CDLL('libtif.so')

    strBuf = create_string_buffer(1000)
    rv = tifglobs.tif.version(strBuf, sizeof(strBuf))
    print('Using tif library version: %s\n' % repr(strBuf.value))

    if findTif():
      handle = c_int(tifglobs.tif.tifInit(VID_HID, PID_HID))
      dev = showDeviceID(handle)
      showTraceID(handle)
      showUsercode(handle)

      if jedecFile:
        configure(handle, jedecFile, dev)

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
