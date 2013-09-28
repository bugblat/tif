#! /bin/bash
#apt-get update

#
# Install libudev and libusb, plus symbolic links
#
apt-get install libudev-dev libusb-1.0-0-dev -y
ln -sf /usr/lib/i386-linux-gnu/libusb-1.0.so /usr/lib/libusb-1.0.so
ln -sf /usr/lib/libusb-1.0.so /usr/lib/libusb.so
ldconfig

#
# Install c++ compiler tools
#
apt-get install pkg-config build-essential g++ -y

#
# Open up USB so we can run scripts without sudo
# "trigger" forces reread of the udev rules, but this needs more work.
# It may be necessary to unplug/replug the tif board after some commands.
# This is still being worked on. Linux gurus please...
#
cp tif.rules /etc/udev/rules.d
udevadm trigger
