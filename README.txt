

            -----------------------------
               USB PICkit 1 programmer
            -----------------------------


Tiny command-line program to control the USB
   Microchip(tm) PICkit(tm) 1 FLASH Starter Kit
from Linux.

Originally written by Orion Sky Lawlor, olawlor@acm.org
Modified by Mark Rages, markrages@gmail.com
Modified by Jeff Boly, jboly@teammojo.org
Modified by David Henry, tfc_duke@club-internet.fr

(please contact David Henry for anything about this version)


This program is based on the usb_pickit 1.5-branch from Jeff Boly.


 Introduction
--------------------------------------------------------------------

This is a little text-based utility to control the excellent and very
cheap Microchip(tm) PICkit(tm) 1 FLASH Starter Kit device programmer
from Linux.

The PICkit is a little board that plugs into your USB port, and lets
you upload programs into the flash memory of the 8-pin and 14-pin
Microchip Programmable Interrupt Controllers (PICs), which are some of
the best computers you can buy new for $2 apiece.  The programmer is
only $35 (€30) direct from Microchip, and comes with a pile of useful
(but Windows) software.  Buy one -- you can't beat the price.

This program can upload and download .hex files to/from the PIC chip
plugged into the PICkit's socket, as shown in the usage:

 pickit1 [OPTION]

where OPTION can be:

  -p, --program=<file>     Writes .hex file to chip
  -x, --extract=<file>     Read from chip into .hex file
  -v, --verify=<file>      Read from chip and compare with .hex file
  -b, --blankcheck         Read chip, check all locations for 1 or blank
  -e, --erase              Erase device.  Preserve OscCal and BG Bits if
                           implemented
  -m, --memorymap          Show device Program and EE Data Memory
  -c, --config             Show configuration data
  -r, --reset              Power cycle the chip
  --off                    Turn chip power off
  --on                     Turn chip power back on
  --oscoff                 Turn 2.5 kHz osc off, leave chip on
  --oscon                  Turn 2.5 kHz osc on, with chip on
  --bandgap=<int>          Erase device.  Preserve OscCal and write specified
                           BG Bits
  --osccalregen            Erase device.  Regenerate OscCal using autocal.hex
  --programall=<file>      Overwrite OscCal and BG (dangerous!)

<file> is an Intel MDS .hex file; the standard format used by almost
all compilers, assemblers, and disassemblers.


 Operating Systems
--------------------------------------------------------------------

 GNU/Linux
 ^^^^^^^^^^^^
This programmer should work for any Linux kernel >= 2.6.15.  It has
been developped and tested with Ubuntu 6.06 and 6.10.  For older
kernels, it may work... or not :)
If there are some endianess issues, contact me.

 MacOS X
 ^^^^^^^^^^
The 1.5 version was compatible with MacOS X.  Since I haven't a Mac, I
couldn't test if this version still works under OS X.  Any feeback
about this program compiling and running under MacOS X would be
appreciated. Please tell me if it works or not, and eventually
what should be corrected in order to make it working.

 Windows
 ^^^^^^^^^^
The programmer has been ported to MS Windows platforms.  It is still
harder to build and install under Windows, since it requires
libusb-win32 to be correctly installed.  It has been compiled with
MinGW and probably doesn't build with any other compiler.
Windows is not officially supported, I just ported it to keep the code
portable at a certain point.

IMPORTANT: You must first install the usb_pickit Windows driver for
libusb-win32 before you can run the program!

Known bug: After uploading a program, you need to deconnect/reconnect
the device in order to make it running (dunno why).

 Other systems
 ^^^^^^^^^^^^^^^^
You can tell me if it works on other platforms/systems and send
patches if needed to make the programmer working on them.


 Compiling
--------------------------------------------------------------------

There is no configure script.  Enter into pickit1's directory and just
type:

	make

You'll need libraries and development files of libusb and libpopt.  The
program is know working with libusb >= version 0.10.

JEB - Notes: For Mac OS X, you will want to set the
HAVE_LIBUSB_INTERRUPT_MODE to 0 in the usb_pickit.c file, or change
the make options.  Also, make sure and get the Mac OS X libusb if you
plan to edit the code.  You can get it here: 
http://charm.cs.uiuc.edu/users/olawlor/projects/2003/microchip/libusb-0.1.7.olawlor.tgz  

To build using the older "bulk writes" with an older libusb
on an older (pre 2.6) kernel, you could also say:

	make OPTS=-DHAVE_LIBUSB_INTERRUPT_MODE=0


 Installing
--------------------------------------------------------------------

!!!For Windows version, you must first install the usb_pickit
driver for libusb-win32!!!

You can install the program in /usr/local/bin with root privileges:

    make install

The program accesses the USB hardware directly, so you
have to run it as root.  If you're willing to accept 
the (small) security risk, run:

    make suid-install

to make usb_pickit setuid root and copy it to /usr/local/bin.
You'll then be able to run it as a non-root user. I've reviewed
the code for security holes, but let me know if I missed any.

NOTE: the program need the autocal.hex file for the --osccalregen
option.  You'll need to run the program in the same directory than
where autocal.hex is if you want to use this option.


 Examples
--------------------------------------------------------------------

You can find some examples for the PIC 12F675 and 16F684 in the
`example' directory:

 - blink: makes blinking alternatively LEDs D0 and D1;
 - blink8: makes blinking alternatively the eight LEDs, D0 to D7;
 - switch: switch between LEDs D0 and D1 when pressing SW1;
 - switch8: switch between LEDs D0 to D7 when pressing SW1;
 - timer8: makes blinking alternatively the eight LEDs, D0 to D7 using
   an internal timer.


Older Troubleshooting
--------------------------------------------------------------------

For version 1.0, you'll need to disable the USB hid driver somehow,
because the hid driver claims the pickit, because the
pickit (for Windows compatability) claims it's a Human 
Input Device in its default configuration.  (Configuration
2 doesn't have this problem.)

The easy way to detach the hid driver in Linux is just

    rmmod hid

which blows away the USB hid driver completely
(assuming it's built as a module, which it should be).  

Sadly, the easy fix will kill off your USB keyboard and mouse, 
so a cleaner fix is to add the lines

#define USB_VENDOR_ID_MICROCHIP         0x04d8
#define USB_DEVICE_ID_PICKIT1           0x0032

to linux/drivers/usb/hid-core.c, and add this line to 
that file's big "hid_blacklist":

     { USB_VENDOR_ID_MICROCHIP, USB_DEVICE_ID_PICKIT1, HID_QUIRK_IGNORE },

and finally rebuild and reinstall your kernel modules.
(This fix might never go into the mainstream kernel)

On anything else, like BSD or MacOS X (which both should
work otherwise), just use Version 1.1 or later, which 
should work fine.


 Links
--------------------------------------------------------------------

usb_pickit 1.5 for MacOS X and Linux, by Jeff Boly:
    http://www.teammojo.org/PICkit/pickit1.html

usb_pickit 1.21 for Linux, by Orion Sky Lawlor:
    http://lawlor.cs.uaf.edu/~olawlor/projects/2003/microchip/

Scott Dattalo maintains an excellent page of PIC-related free
software at:
    http://www.gnupic.org/

HI-TECH gives away a full-featured "lite" version of their excellent
PIC C compiler PICC, including a Linux version:
    http://www.htsoft.com/products/piclite/piclite.html

libusb, if you need a newer version than your system has or
documentation for hacking the program:
    http://libusb.sourceforge.net/

For Windows version, you'll need libusb-win32 and libpopt-win32:
    http://libusb-win32.sourceforge.net/
    http://gnuwin32.sourceforge.net/packages/popt.htm


 Version History
--------------------------------------------------------------------
2012/09/22: Version 1.6.2-dh (tfc.duke@gmail.com)
Fixed buffer overflows when using sprintf function for reading
config bits and writing eeprom to device.

2007/01/27: Version 1.6.1-dh (tfc_duke@club-internet.fr)
Bug fixes due to the absence of warnings at compilation.

2006/09/01: Version 1.6-dh (tfc_duke@club-internet.fr)
No new feature but heavy changes in source code (organization,
 comments, structures, coding style, etc.)
Use libpopt to parse command line options.
Some optimizations when sending data to the programmer: send two
 commands by packet instead of only one.
Fix problems with libusb and Linux kernels >= 2.6.15
Work with lubusb-win32 (compile with mingw)
License changed to MIT license.
Examples for PIC 12F675 and 16F684 have been added.

2005/12/31: Version 1.5-jeb (jboly@teammojo.org)
Added support for lots of new command line options.
Also added support for all of the devices that 
Microchip supports with the latest 1.70 Windows software
and the 2.0.2 firmware on the 16C745.    I used Mark Rages
device format, but modified it slightly to better accomodate
the 12F chips with OscCal in the last program memory location.
Also added the configmask value to each chip in the database
to better handle ANDind of the configword with cheksums.
Fixed several bugs.   In particular, there was a bug that 
prevented the eeprom data memory from writing properly.
Also added lots of debuging print statements which I left in the
code and commented out.   Might be useful if you are trying to debug
something.     I've tested this version with many PIC devices and
against the Windows version.    Here are the command line options
that I added:

 usb_pickit --verify  <file> (Read from chip and compare with .hex file)
 usb_pickit --blankcheck (Read chip, check all locations for 1 or blank)
 usb_pickit --erase (Erase device.   Preserve OscCal and BG Bits if implemented)
 usb_pickit --bandgap <bg> (Erase device.  Preserve OscCal and write specified BG Bits)
 usb_pickit --osccalregen (Erase device.  Regenerate OscCal using autocal.hex)
 usb_pickit --memorymap (Show device Program and EE Data Memory)
 usb_pickit --config (Show configuration data)
 usb_pickit --oscoff   (Turn 2.5 hHz osc off, leave chip on)
 usb_pickit --oscon    (Turn 2.5 kHz osc on, with chip on)

The --verify option now allows one to compare a .hex file with the contents
of the deivce.  And I made use of the firmware "S" option to assist in 
verifying programming operations.
The --osccalregen option uses the 2.5 kHz Oscillator on the C745 to
calibrate the osccal on the 629,675,630 or 676 chips.    Best not to lose
the OscCal in the first place, but this is a nice failsafe.   I've added the
autocal.hex file to the distribution which is the included calibration file
from Microchip that gets loaded onto the device and allows the device to compute
its own OscCal and store it into eeprom data memory.   Then, the --osccalregen
code retries the stores cal value and writes it to 3fff, the last location of
program memory.   I felt it was important to have this functionality because
without it, a trip to a Windows box is required.
I enhanced the print --config option to print more information about the 
config.   I wrote in the checksum code similar to what is offered in the 
Windows version.   In fact, I added functionality for almost every feature
that is in the Windows version.   The only thing missing is the Code Protect
feature, but if you put the config word directive in your code and set the
Code or Data Protect Bits accordingly, your code will be protected once the
chip is written.    The --memorymap function simply prints the program and
data memory out to the screen.   Useful if you want to see what's in a particular
location, and nice because you can pipe to grep to help find what you are looking
for.  I am happy to correct any bugs you may find or answer any other questions.
And I am also happy to test any other PIC devices if you want to send me any that
are on the list.    I've got adapters for 18,20,28 and 40 pin DIP devices.
My code is not as compact as Orion's but it gets the job done.    I hope this
is helpful to electrical engineers who use Mac OS X. Note, I have made no
attempt to add support for the 10F devices, although I might add it if many 
people want this functionality. 

2005/4/30: Version 1.21-mar (markrages@gmail.com)
Added support for automatically detecting chip type.
Will not clobber EEPROM data unless there is data in 
the hex file.  Only writes data that is in the hex file,
then quit. (saves time with large chips and short programs).

2004/11/12: Version 1.21
Added *beta* support for version 2.0 of the PICkit
onboard firmware.  Dunno if it'll work with new 10F devices.
WARNING: *programming* 12F and 16F devices seems to work; 
but --extract hangs a new board hard: you've got to unplug 
and replug the USB connector to make it work again.  
Everything else seems to work fine, though -- thanks to 
Mattew Wilson for sending me a v2.0 16C745!

Added small startup bugfix pointed out by Curtis Sell.
Previously, if the chip power was not off, programming 
or even configuration read could fail intermittently.
usb_pickit now turns power off before doing anything.

Added --on and --off commands, as suggested by Rogier Wolff
to turn off the annoying blinking lights.

Fixed command-line handling so USB isn't touched unless
you've specifically asked usb_pickit to do something.

MacOS X version is unchanged, since my OS X machine has
moved to Alaska, but I haven't (yet!).

2004/2/6: Version 1.2
Updated to latest libusb to use USB interrupt mode.
This should fix an "invalid argument" error using
the latest Linux kernels.  Thanks to Steven Michalske 
for diagnosing the cause of this problem.

RANT:
  In an excellent example of sacrificing backward compatability
for conformance to some proported "standard", the latest Linux 
kernel USB drivers (uhci-alternate in 2.4.24+, and uhci in 2.6.x)
no longer support low speed bulk mode transfers-- they give
"invalid argument", errno=-22, on any attempt to do a low speed 
bulk write. 

2004/2/3: Version 1.11
   Source code is identical; the README file is updated
and the Linux executable now linked against glibc 2.2.4.

2004/1/19: Version 1.1
   Now this program works on OS X, and out-of-the box on 
Linux--no bizarre HID troubles.  Thanks to Joseph Julicher, 
a friendly Microchip engineer that (out of the blue!) recommended
the usb_set_configuration technique that fixes this.

2003/8/3: Version 1.0
   Initial version.


 Support
--------------------------------------------------------------------

I'd love to hear what you're doing with this program, and I'd be glad
to include any improvements you make.

I am (we are) in no way affiliated with the company Microchip, so
don't bug them about problems with this program.

If you have problems, please read this file completely, and check
google, *before* sending me e-mail.  Also check for older version,
some troubleshootings info in their README.  I probably couldn't debug
your Linux installation, even if I wanted to.

No, I can't help you with your EE homework/project/thesis. No, not
even (and especially) if you really, really need help.
