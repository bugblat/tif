#! /bin/bash
cd src/linux

echo "--Building both variants of the hidapi libraries--"
make -f hidapi_hidraw.mak
make -f hidapi_libusb.mak

echo "--Installing both variants of the hidapi libraries--"
make -f hidapi_hidraw.mak install
make -f hidapi_libusb.mak install
ldconfig

cd ../libtif

echo "--Building the libtif library--"
make -f libtif.mak

echo "--Installing the libtif library--"
make -f libtif.mak install
ldconfig
