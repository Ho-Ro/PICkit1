# Makefile for USB pickit tools:

OPTS = -O2 -ansi -Wall
OBJS = pickit1.o hex.o pic14.o devices.o usb_pickit.o

CFLAGS = $(OPTS)
LDFLAGS = -lusb -lpopt -s
LDFLAGS_STATIC = /usr/include/libusb.a
STATIC_NAME = usb_pickit.`uname -s`.`uname -i`

# Needed for static linking under OS X:
# LDFLAGS=-lusb -lpopt -lIOKit -framework CoreFoundation

all: usb_pickit

usb_pickit: $(OBJS)
	$(CC) $(CFLAGS) -o ../$@ $(OBJS) $(LDFLAGS)

static: $(STATIC_NAME)

# This is wrong but it works.  Patches welcome.  MAR
$(STATIC_NAME): $(OBJS)
	$(CC) -static $(CFLAGS) -o ../$(STATIC_NAME) $(OBJS) $(LDFLAGS_STATIC)

# Test programs
test_hex: hex.c
	$(CC) $(CFLAGS) -DTEST_HEX -o ../$@ hex.c $(LDFLAGS)

test_pic_hex: hex.c pic14.c devices.c
	$(CC) $(CFLAGS) -DTEST_HEX -o ../$@ hex.c pic14.c devices.c $(LDFLAGS)

clean:
	rm -f \#* *.o core.* *~

#
# file dependencies
#

hex.o: hex.c hex.h common.h
pic14.o: pic14.c pic14.h hex.h common.h
devices.o: devices.c pic14.h common.h
usb_pickit.o: usb_pickit.c usb_pickit.h pic14.h common.h
