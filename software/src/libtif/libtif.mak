UNIFLAGS	= -Wall -g -fPIC -I. -DBUILDING_LIBTIF -fvisibility=hidden -lusb
CXX		= g++
CXXFLAGS	= -ansi $(UNIFLAGS) -fvisibility-inlines-hidden
CC			= gcc
CCFLAGS	= $(UNIFLAGS)

DEPS		= tif.h tifwrap.h cp2112.h ../hidapi/hidapi.h
OBJS		= tif.o tifwrap.o cp2112.o ../linux/hid-libusb.o
TARGET	= libtif.so
LIBS		= -lstdc++
LDFLAGS	= -shared -Wl,-soname,$(TARGET) -fvisibility=hidden -lusb
INC		= -I../hidapi -I/usr/include/libusb-1.0



%.o : %.cpp $(DEPS)
	$(CXX) -c -o $@ $(CXXFLAGS) $(INC) $<

%.o : %.c $(DEPS)
	$(CC) -c -o $@ $(CCFLAGS) $(INC) $<

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)



install:
	cp $(TARGET) /usr/lib
	chmod 0755 /usr/lib/$(TARGET)



.PHONY: clean

clean:
	rm -f *.o $(TARGET)
