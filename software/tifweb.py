##---------------------------------------------------------
# Name:        tifweb.py
# Purpose:     control a tif board via a web server
#
# Author:      Tim
#
# Created:     08/07/2013
# Copyright:   (c) Tim 2013
# Licence:     Creative Commons Attribution-ShareAlike 3.0 Unported License.
##---------------------------------------------------------
# uses web.py - see  www.webpy.org
#
# windows command line start: python tifweb.py
#
#!/usr/bin/env python

import sys, web, ctypes, tifglobs
from web      import form
from ctypes   import *
from pyhidapi import hid
from tifglobs import *

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
# write val into the Misc register inside the FPGA
def setMiscRegister(val):
  try:
    sendAddressByte(tifglobs.handle, W_MISC_REG)
    sendDataByte(tifglobs.handle, val)
  except:
    pass

##---------------------------------------------------------
urls    = ('/', 'index')
render  = web.template.render('templates/', base='layout')
tag     = 'Red and Green LEDs '
myform  = web.form.Form(
  form.Dropdown(tag, [STR_LEDS_ALT, STR_LEDS_SYNC, STR_LEDS_OFF]))

class index:
  def GET(self):
    form = myform()
    return render.index(tifglobs.state, form)

  def POST(self):
    form = myform()
    if form.validates():
      tifglobs.state = form[tag].value
      if tifglobs.state==STR_LEDS_ALT:
        setMiscRegister(LED_ALTERNATING)
      elif tifglobs.state==STR_LEDS_SYNC:
        setMiscRegister(LED_SYNC)
      elif tifglobs.state==STR_LEDS_OFF:
        setMiscRegister(LED_OFF)
    return render.index(tifglobs.state, form)

##---------------------------------------------------------
def main():
  try:
    tifglobs.tif = ctypes.CDLL("libtif.dll")

    strBuf = create_string_buffer(1000)
    rv = tifglobs.tif.version(strBuf, sizeof(strBuf))
    print('Using tif library version: %s\n' % repr(strBuf.value))

    if findTif():
      print('tif detected')
      tifglobs.handle = c_int(tifglobs.tif.tifInit(VID_HID, PID_HID))
      tifglobs.state = STR_LEDS_ALT
      setMiscRegister(LED_ALTERNATING)
      app = web.application(urls, globals())
      app.run()

  except:
    e = sys.exc_info()[0]
    print("\nException caught %s\n" % e)

  if tifglobs.handle:
    tifglobs.tif.tifClose(tifglobs.handle)

##---------------------------------------------------------
if __name__ == '__main__':
  main()

# EOF -----------------------------------------------------------------
