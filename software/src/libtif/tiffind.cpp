//---------------------------------------------------------------------
// tiffind.cpp

#pragma hdrstop

#include <iostream>
#include <iomanip>
#include <ostream>

using namespace std;

#include <stdio.h>
#include <stdlib.h>

#include "hidapi.h"
#include "cp2112.h"

//#define VID   0x10c4
//#define PID   0xea90

//---------------------------------------------------------------------
void printHidStrings(hid_device *handle) {
  if (handle == NULL)
    return;

  #define MAX_STR 255
  wchar_t wstr[MAX_STR];
  int res = 0;

  // Read the Manufacturer String
  res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
  if (res < 0)
    return;
  wcout << "Manufacturer String : " << wstr << endl;

  // Read the Product String
  res = hid_get_product_string(handle, wstr, MAX_STR);
  if (res < 0)
    return;
  wcout << "Product String      : " << wstr << endl;

  // Read the Serial Number String
  res = hid_get_serial_number_string(handle, wstr, MAX_STR);
  if (res < 0)
    return;
  wcout << "Serial Number String: " << wstr << endl;
  }

//---------------------------------------------------------------------
int main(int argc, char* argv[]) {
  hid_device *handle = NULL;

  hid_init();

  // Enumerate and print the HID devices on the system
  struct hid_device_info *devs    = hid_enumerate(0, 0);
  struct hid_device_info *cur_dev = devs;

  while (cur_dev) {
    printf("HID Device Found\n  type: %04hx %04hx\n  serial_number: %ls",
      cur_dev->vendor_id, cur_dev->product_id, cur_dev->serial_number);
    printf("\n");
    printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
    printf("  Product:      %ls\n", cur_dev->product_string);
    printf("\n");
    cur_dev = cur_dev->next;
    }
  hid_free_enumeration(devs);

  // Open the device using the VID, PID,
  // and optionally the Serial number.
  handle = hid_open(VID_HID, PID_HID, NULL);

  printHidStrings(handle);
  hid_close(handle);
  hid_exit();
  return 0;
  }

// EOF ----------------------------------------------------------------
