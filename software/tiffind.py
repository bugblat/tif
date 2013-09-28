#----------------------------------------------------------------------
# Name:        tiffind.py
# Purpose:     sarch for HID devices via the hidapi DLL/SO
#
# Author:      Tim
#
# Created:     11/07/2012
# Copyright:   (c) Tim 2013
# Licence:     Creative Commons Attribution-ShareAlike 3.0 Unported License.
#----------------------------------------------------------------------
# uses Austin Morton's pyhidapi package.
# See  https://github.com/Juvenal1228/pyhidapi
#
#!/usr/bin/env python

import ctypes
from ctypes import *
from pyhidapi import hid

def main():
  print("====================hello==========================")
  try:
    devs = hid.enumerate()
    for dev in devs:
      s  = "Manufacturer:%s" % dev['manufacturer_string']
      print(s)
      s  = "  Product:%s" % dev['product_string']
      print(s)
      s  = "  VID:" + ("%04X" % dev['vendor_id'])
      s += "  PID:" + ("%04X" % dev['product_id'])

      # test for a null serial number with just a language ID
      serialNumber = dev['serial_number']
      if serialNumber == u'\u0409':
        s += "  no Serial Number"
      else:
        s += "  Serial Number:" + repr(serialNumber)
      print(s)

  except:
    e = sys.exc_info()[0]
    print("\n|| exceptional exception caught %s ||\n" % e)

  print("==================== bye ==========================")

if __name__ == '__main__':
  main()

# EOF -----------------------------------------------------------------
