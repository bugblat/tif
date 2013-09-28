###########################################
# Simple Makefile for HIDAPI test program
#
# Alan Ott
# Signal 11 Software
# 2010-06-01
#
# modified from Alan Ott's original program to just build the libs
# and to install them in /usr/lib
###########################################

all: libs

SO_LIB = libhidapi-hidraw.so

libs: $(SO_LIB)

CC          ?= gcc
CFLAGS      ?= -Wall -g -fpic

LDFLAGS     ?= -Wall -g

COBJS_LIBUSB =  hid-libusb.o
COBJS_HIDRAW =  hid.o
COBJS        =  $(COBJS_HIDRAW)
OBJS         =  $(COBJS)

LIBS_USB     =  `pkg-config libusb-1.0 --libs`
LIBS_UDEV    =  `pkg-config libudev --libs`
LIBS         =  $(LIBS_USB)

INCLUDES    ?= -I../hidapi `pkg-config libusb-1.0 --cflags`


libhidapi-libusb.so: $(COBJS_LIBUSB)
	$(CC) $(LDFLAGS) $(LIBS_USB) -shared -fpic -Wl,-soname,$@.0 $^ -o $@ -lusb

libhidapi-hidraw.so: $(COBJS_HIDRAW)
	$(CC) $(LDFLAGS) $(LIBS_UDEV) -shared -fpic -Wl,-soname,$@.0 $^ -o $@ -ludev


# Objects
$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@



clean:
	rm -f $(OBJS) $(SO_LIB)

	
install:
	cp $(SO_LIB) /usr/lib
	chmod 0755 /usr/lib/$(SO_LIB)
	ldconfig
	

.PHONY: clean libs
