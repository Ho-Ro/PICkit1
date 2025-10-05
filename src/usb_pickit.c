/*
 * usb_pickit.c
 *
 * a USB interface to the Microchip PICkit 1 FLASH Starter Kit
 * device programmer and breadboard.
 *
 * These functions deal directly with the PICkit programmer.
 *
 * Orion Sky Lawlor, olawlor@acm.org, 2003/8/3
 * Mark Rages, markrages@gmail.com, 2005/4/1
 * Jeff Boly, jboly@teammojo.org, 2005/12/31
 * David Henry, tfc_duke@club-internet.fr, 2007/1/27
 * Martin Homuth-Rosemann, 2025/10/05
 */

#ifndef PATH_MAX
#ifdef __linux__
#include <linux/limits.h>
#elif defined (__MINGW32__)
#include <limits.h>
#else
#define PATH_MAX 256
#endif
#endif /* PATH_MAX */

#include <sys/types.h>
#include <errno.h>
#include <usb.h>
#include "common.h"
#include "usb_pickit.h"

#ifdef _WIN32
#include <windows.h>
#define sleep(n) Sleep((n)*1000)
#endif

/* PICkit USB values */
const static int pickit_vendorID = 0x04d8; /* Microchip, Inc */
const static int pickit_productID = 0x0032; /* PICkit 1 FLASH starter kit */
const static int pickit_configuration = 2; /* 1: HID; 2: vendor specific */
const static int pickit_interface = 0;
const static int pickit_endpoint_out = 1; /* endpoint 1 address for OUT */
const static int pickit_endpoint_in = 0x81; /* endpoint 0x81 address for IN */
const static int pickit_timeout = 1000; /* timeout in ms */

/* PICkit always uses 8-byte transfers */
#define REQ_LEN 8

/*
 * Firmware 2.0.2 implements thirteen commands:
 *
 *   'P' - Enter Programming mode.  Enables Programming mode in the
 *         device.  Must be done before any other commands can be
 *         executed.
 *   'p' - Exit Programming mode.  Shuts down Programming mode.
 *   'E' - Bulk erase program memory
 *   'e' - Bulk erase data memory
 *   'W' - Write program memory and increment PC to the next word
 *         address
 *   'D' - Write byte to EE Data memory
 *   'C' - Advance PC to Configuration memory (0x2000)
 *   'I' - Increment address n times
 *   'R' - Read four words from program memory
 *   'r' - Read eight bytes from EE Data memory
 *   'V' - Control device power and 2.5 kHz control
 *   'v' - Return firmware version, three bytes <major>, <minor>,
 *         <dot>
 *   'S' - Calculate checksums for both Program memory and EE Data
 *         memory.
 *   'Z' - Null command, used to pad out the 8 byte command packets
 *
 * commands must be sent in 8-byte packets.  unused command bytes must
 * be written as null commands ('Z').
 *
 * see PROTOCOLE.txt for more informations about these commands.
 */


/*
 * Stupidity:
 *  In an excellent example of sacrificing backward compatability
 * for conformance to some proported "standard", the latest Linux
 * kernel USB drivers (uhci-alternate in 2.4.24+, and uhci in 2.6.x)
 * no longer support low speed bulk mode transfers -- they give
 * "invalid argument", errno = -22, on any attempt to do a low speed
 * bulk write.  Thus, we need interrupt mode transfers, which are
 * only available via the CVS version of libusb.
 *
 * (Thanks to Steven Michalske for diagnosing the true problem here.)
 */

/*
 * JEB - Note here, for the Mac, there is no interrupt mode, so need to set this
 * define to zero to get it to compile.  I use Mac OS X.
 */
#ifndef HAVE_LIBUSB_INTERRUPT_MODE
#define HAVE_LIBUSB_INTERRUPT_MODE 1
#endif

#if HAVE_LIBUSB_INTERRUPT_MODE
/* latest libusb: interrupt mode, works with all kernels */
#define PICKIT_USB(direction) usb_interrupt_##direction
#else
/* older libusb: bulk mode, will only work with older kernels */
#define PICKIT_USB(direction) usb_bulk_##direction
#endif


/*
 * send a 8-byte command packet to PICKit.
 */
static void
send_usb (usb_pickit *d, const char *src)
{
  int r = PICKIT_USB(write)(d, pickit_endpoint_out, (char *)src,
			    REQ_LEN, pickit_timeout);

  if (r != REQ_LEN)
    {
      fprintf (stderr, "USB PICKit write: %s\n", usb_strerror ());
      exit (errno);
    }
}

/*
 * write the next program counter with this word.
 */
static void
send_usb_word (usb_pickit *d, pic14_word w)
{
  char cmd[REQ_LEN + 1] = "W__ZZZZZ";

  cmd[1] = (char)(w & 0xff);
  cmd[2] = (char)((w >> 8) & 0xff);

  send_usb (d, cmd);
}

/*
 * write the next n program counters with these words.
 * JEB - I like the way that MAR added the '.' that print out during
 * the write, nice touch.
 */
static void
send_usb_words (usb_pickit *d, unsigned int n, pic14_word *w)
{
  unsigned int i;
  char cmd[REQ_LEN + 1] = "W__W__ZZ";

  /* send words two by two in a 8-byte packet */
  for (i = 0; i < n / 2; ++i)
    {
      /* pick two words to send */
      pic14_word w1 = w[i * 2 + 0];
      pic14_word w2 = w[i * 2 + 1];

      printf ("."); /* MAR add */
      fflush (stdout);

      cmd[1] = (char)(w1 & 0xff);
      cmd[2] = (char)((w1 >> 8) & 0xff);

      cmd[4] = (char)(w2 & 0xff);
      cmd[5] = (char)((w2 >> 8) & 0xff);

      send_usb (d, cmd);
    }

  /* if the number of words to send is odd,
     send the last one */
  if (n % 2)
    send_usb_word (d, w[n - 1]);

  printf ("\n"); /* MAR add */
}

/*
 * read len bytes from the device
 */
static void
recv_usb (usb_pickit *d, int len, byte *dest)
{
  int r = PICKIT_USB(read)(d, pickit_endpoint_in, (char *)dest,
			   len, pickit_timeout);

  if (r != len)
    {
      fprintf (stderr, "USB PICKit read: %s\n", usb_strerror ());
      exit (errno);
    }
}

/*
 * read 4 words from the current address
 */
static void
recv_usb_words4 (usb_pickit *d, pic14_word *dest)
{
  int i;
  byte buffer[REQ_LEN];

  send_usb (d, "RZZZZZZZ");
  recv_usb (d, REQ_LEN, buffer);

  /* reconstitute the 4 words from the 8 bytes received */
  for (i = 0; i < 4; ++i)
    dest[i] = buffer[2 * i + 0] + (buffer[2 * i + 1] << 8);
}

/*
 * read len words from the device
 */
static void
recv_usb_words (usb_pickit *d, unsigned int len, pic14_word *dest)
{
  while (len > 0)
    {
      pic14_word buffer[4];
      unsigned int i, c = 4; /* number of words to copy out */

      /* read next four words */
      recv_usb_words4 (d, buffer);

      if (c > len)
	c = len;

      /* copy them to destination buffer */
      for (i = 0; i < c; ++i)
	dest[i] = buffer[i];

      dest += c;
      len -= c;
    }
}

/*
 * initialize USB connection with PICKit
 */
static int
usb_pickit_init (usb_dev_handle *d)
{
  byte version[REQ_LEN];

  /* set the configuration for USB PICKit */
  if (usb_set_configuration (d, pickit_configuration) < 0)
    {
      fprintf (stderr, "%s\n", usb_strerror ());
      return 0;
    }

  /* this is our device, claim it */
  if (usb_claim_interface (d, pickit_interface) < 0)
    {
      fprintf (stderr, "%s\n", usb_strerror ());
      return 0;
    }

  /*
   * turn off power to the chip before doing anything.
   * this prevents weird random errors during programming.
   * (thanks to Curtis Sell for this fix)
   */
  usb_pickit_off (d);

  /* read firmware version */
  send_usb (d, "vZZZZZZZ");
  recv_usb (d, REQ_LEN, version);

  printf ("communication established, "
	  "onboard firmware version is %d.%d.%d\n",
	  version[0], version[1], version[2]);

  if (version[0] > 0x02)
    {
      printf ("Warning: USB PICkit major version is %d; "
	      "last known working version is 2\n", version[0]);
    }

  return 1;
}

/*
 * find the first USB device with this vendor and
 * product.  exits on errors, like if the device couldn't be found.
 */
usb_pickit *
usb_pickit_open ()
{
  struct usb_device *device;
  struct usb_bus *bus;
#if 0
#ifndef _WIN32
  /* ensure user have root privileges for executing this program */
  if (geteuid () != 0)
    {
      fprintf (stderr, "this program must be run as root, "
	       "or made setuid root\n");
      return NULL;
    }
#endif /* _WIN32 */
#endif
  /* announce what we are looking for */
  printf ("Locating USB Microchip(tm) PICkit(tm) "
	  "(vendor 0x%04x/product 0x%04x)\n",
	  pickit_vendorID, pickit_productID);

  /* libusb setup code stolen from John Fremlin's cool "usb-robot" */
  usb_init ();
#ifdef DEBUG
  usb_set_debug (4);
#endif
  usb_find_busses ();
  usb_find_devices ();

  /* look through each bus */
  for (bus = usb_busses; bus != NULL; bus = bus->next)
    {
      struct usb_device *usb_devices = bus->devices;

      /* look through each device of this bus */
      for (device = usb_devices; device != NULL;
	   device = device->next)
	{
	  /* check if vendor ID and product ID correspond */
	  if (device->descriptor.idVendor == pickit_vendorID &&
	      device->descriptor.idProduct == pickit_productID)
	    {
	      /* we found PICKit! */

	      int retval;
	      char dname[32] = {0};
	      usb_dev_handle *d;

	      printf ("found USB PICkit as device '%s' on USB bus %s\n",
		      device->filename, device->bus->dirname);

	      /* open the device */
	      d = usb_open (device);
	      if (!d)
		{
		  fprintf (stderr, "Error: failed to open USB device\n");
		  fprintf (stderr, "%s\n", usb_strerror ());
		  return NULL;
		}

#ifdef __linux__
	      /* look if a driver doesn't already claim this interface */
	      retval = usb_get_driver_np (d, 0, dname, 31);
	      if (!retval)
		{
		  /* detach it so we can use the interface via libusb */
		  usb_detach_kernel_driver_np (d, 0);

		  /* reopen the device */
		  usb_close (d);
		  d = usb_open (device);
		  if (!d)
		    {
		      fprintf (stderr, "Error: failed to open USB device\n");
		      fprintf (stderr, "%s\n", usb_strerror ());
		      return NULL;
		    }
		}
#endif /* __linux__ */

	      /* initialize USB connection with PICKit */
	      if (!usb_pickit_init (d))
		{
		  usb_pickit_close (d);
		  return NULL;
		}

	      return d;
	    }
	}
    }

  /* we looked through each device of each bus and didn't
     find PICKit */

  fprintf (stderr, "Could not find USB PICKit device!\n"
	   "you might try lsusb to see if it's actually there.\n");
  return NULL;
}

/*
 * close the USB PICKit device.
 */
void
usb_pickit_close (usb_pickit *d)
{
  /* release claimed interface */
  if (usb_release_interface (d, pickit_interface) < 0)
    {
      fprintf (stderr, "%s\n", usb_strerror ());
      exit (EXIT_FAILURE);
    }

#ifdef _WIN32
  /* !!!HACK: for some reasons, the usb device need to be reset before
     closing.  Otherwise, you'll have to deal with weird behaviours... */
  if (usb_reset (d) < 0)
    {
      fprintf (stderr, "%s\n", usb_strerror ());
      exit (EXIT_FAILURE);
    }

  return;
#endif /* _WIN32 */

  /* close usb device */
  if (usb_close (d) < 0)
    {
      fprintf (stderr, "%s\n", usb_strerror ());
      exit (EXIT_FAILURE);
    }
}

/*
 * turn the device on
 */
void
usb_pickit_on (usb_pickit *d)
{
  send_usb (d, "V1ZZZZZZ");
}

/*
 * turn the device off
 */
void
usb_pickit_off (usb_pickit *d)
{
  send_usb (d, "V0ZZZZZZ");
}

/*
 * turn the 2.5 kHz osc on (JEB)
 */
void
usb_pickit_osc_on (usb_pickit *d)
{
  send_usb (d, "V3ZZZZZZ");
}

/*
 * turn the 2.5 kHz osc off (JEB)
 */
void
usb_pickit_osc_off (usb_pickit *d)
{
  send_usb (d, "V1ZZZZZZ");
}

/*
 * return device info if device is supported by the programmer.
 *
 * JEB - fixed bug here where programming mode is ended using the 'p'
 * but the device power was not turned back on.  This is important
 * as it is the only way to reset the device after enter/exiting the
 * VPP high programming mode. Also added config word mask for each
 * device for computing checksum also added 1 to the inst_len field
 * that Mark Rages created.  There seems to be a bug when reading from
 * the PICkit.  Not sure where, but the end result is that the last
 * byte read is corrupted if the read does not fall on a four byte
 * boundary. So I simply increased the size of the program memory read
 * by 1.
 * this bug only seems to effect the 627, 675, 630, 676 devices.
 */
int
usb_pickit_get_device (usb_pickit *d, pic14_device *dev)
{
  pic14_state *s = &dev->state;
  pic14_word id_word;

  send_usb (d, "pV0V1PCZ");

  /* read ID word from 0x2006 */
  send_usb (d, "pPCI\x06\x00ZZ");
  recv_usb_words (d, 1, &id_word);
  send_usb (d, "pV1ZZZZZ");

  /* get revision value */
  dev->rev = id_word & 0x1f;

  /* search device in the supported device list */
  const pic14_device_info *dinfo = pic14_get_device (id_word & 0xffe0);
  if (dinfo)
    {
      /* found the device, copy values */
      s->program.inst_len = dinfo->inst_len;
      s->program.ee_len = dinfo->ee_len;
      s->config.save_osccal = dinfo->save_osccal;
      s->config.configmask = dinfo->configmask;

      /* write revision value to device info */
      dev->dinfo = dinfo;

      printf ("PIC%s Rev %d found\n", dinfo->device_name, dev->rev);
      return 1;
    }
  else
    {
      /* device not found */
      fprintf (stderr, "no PIC or unsupported PIC found!\n");
    }

  return 0;
}


/*
 * generate checksum. (JEB)
 * device is read into memory and then the checksum is computed.
 *
 * note this is different than using the "S" command to let the PICkit 1
 * firmware calculate the checksum.  the two checksums can be compared
 * to make sure they match.
 *
 * must be called after usb_pickit_read so that buffers contain the
 * values of program memory and config word.
 *
 * does not take into account any code protection that is turned on.
 */
void
usb_pickit_calc_checksum (pic14_state *s)
{
  int i;

  /* add the CONFIG word to the checksum */
  s->program.instchecksum = (s->config.config & s->config.configmask);

  /* sum all instruction words */
  for (i = 0; i < s->program.inst_len; ++i)
    s->program.instchecksum += s->program.inst[i];

  s->program.instchecksum &= 0xffff;
}

/*
 * read checksum from device via programmer "S" function. (JEB)
 */
void
usb_pickit_read_checksum (usb_pickit *d, pic14_state *s)
{
  pic14_word checksum[2];

  /* fill in program and data length values */
  char cmd[REQ_LEN + 1] = "S____V1Z";
  cmd[1] = (char)(s->program.inst_len & 0xff);
  cmd[2] = (char)(s->program.inst_len >> 8);
  cmd[3] = (char)(s->program.ee_len & 0xff);
  cmd[4] = (char)(s->program.ee_len >> 8);

  /* query for PICKit checksum computation */
  send_usb (d, cmd);
  recv_usb_words (d, 2, checksum);
  send_usb (d, "pV1ZZZZZ");

  /* save results into PIC's state */
  s->config.pgmchecksum = checksum[0];
  s->config.eechecksum = (byte)(checksum[1] & 0x00ff);
}

/*
 * read current EEPROM Data memory from the device.
 */
void
usb_pickit_read_eeprom (usb_pickit *d, pic14_program *p)
{
  int nee = 0;

  /* enter programming mode */
  send_usb (d, "PZZZZZZZ");

  /* read EEPROM data */
  while (nee < p->ee_len)
    {
      byte eeData[64];
      int i;

      /* read 8x8 bytes from EE data memory */
      send_usb (d, "rrrrrrrr");
      recv_usb (d, 64, eeData);

      for (i = 0; i < 64; ++i)
	p->ee[nee++] = eeData[i];
    }

  /* exit programming mode */
  send_usb (d, "pZZZZZZZ");
}

/*
 * read current program memory from the device.
 */
void
usb_pickit_read_program (usb_pickit *d, pic14_program *p)
{
  /* enter programming mode */
  send_usb (d, "PZZZZZZZ");

  /* read program memory */
  recv_usb_words (d, p->inst_len, p->inst);

  /* exit programming mode; power on */
  send_usb (d, "pV1ZZZZZ");
}

/*
 * read current configuration from the device.
 */
void
usb_pickit_read_config (usb_pickit *d, pic14_config *c)
{
  /* read OSCCAL from 0x03ff */
  send_usb (d, "V0V1PI\xff\x03");
  recv_usb_words (d, 1, &c->osccal);

  /* read configuration IDs from 0x2000 */
  send_usb (d, "pV0V1PCZ");
  recv_usb_words (d, PIC14_ID_LEN, c->id);

  /* read CONFIG word from 0x2007 */
  send_usb (d, "pPCI\x07\x00ZZ");
  recv_usb_words (d, 1, &c->config);
  send_usb (d, "pV1ZZZZZ");
}

/*
 * fill out this state with the contents of the device.
 * Read EEPROM Data, program memory and config words.
 */
void
usb_pickit_read (usb_pickit *d, pic14_state *s)
{
  usb_pickit_read_eeprom (d, &s->program);
  usb_pickit_read_program (d, &s->program);
  usb_pickit_read_config (d, &s->config);
}

/*
 * write this program's data to device's EEPROM.
 */
void
usb_pickit_write_eeprom (usb_pickit *d, pic14_program *p)
{
  char cmd[REQ_LEN + 1];
  unsigned int i, j;

  /* enter programming mode */
  send_usb (d, "PZZZZZZZ");

  /* write out the EEPROM data */
  printf ("writing %d eeprom words\n", p->max_ee);

  /* write data bytes to EEPROM four by four */
  sprintf (cmd, "D_D_D_D_");
  for (i = 0; i < p->max_ee / 4; ++i)
    {
      cmd[1] = (char)(p->ee[i * 4 + 0]);
      cmd[3] = (char)(p->ee[i * 4 + 1]);
      cmd[5] = (char)(p->ee[i * 4 + 2]);
      cmd[7] = (char)(p->ee[i * 4 + 3]);

      send_usb (d, cmd);
    }

  /* if max_ee is not a multiple of four, write the last bytes */
  int r = p->max_ee % 4;
  if (r != 0)
    {
      i = p->max_ee - r;

      /* encapsulate last bytes into a single packet */
      sprintf (cmd, "ZZZZZZZZ");
      for (j = 0; j < r; ++j, ++i)
	{
	  cmd[j * 2 + 0] = 'D';
	  cmd[j * 2 + 1] = (char)(p->ee[i]);
	}

      /* burn last data bytes */
      send_usb (d, cmd);
    }

  /* exit programming mode */
  send_usb (d, "pZZZZZZZ");
}

/*
 * write this program's instructions to the device.
 */
void
usb_pickit_write_program (usb_pickit *d, pic14_program *p)
{
  /* enter programming mode */
  send_usb (d, "PZZZZZZZ");

  /* write out the program data */
  printf ("writing %d program words\n", p->max_prog);
  send_usb_words (d, p->max_prog, p->inst);

  /* exit programming mode; power on */
  send_usb (d, "pV1ZZZZZ");
}

/*
 * write the configuration (osccal, id, and config word) to
 * the device.  Writes all the bits in the config. word.
 */
void
usb_pickit_write_config (usb_pickit *d, pic14_config *c)
{
  /* write OSCCAL to 0x03ff */
  send_usb (d, "V0V1PI\xff\x03");
  if (c->save_osccal)
    send_usb_word (d, c->osccal);

  /* write configuration ID's to 0x2000 */
  send_usb (d, "pV0V1PCZ");
  send_usb_words (d, PIC14_ID_LEN, c->id);

  /* write configuration word to 0x2007 */
  send_usb (d, "pPCI\x07\x00ZZ");
  send_usb_word (d, c->config);
  send_usb (d, "pV1ZZZZZ");
}

/*
 * write this state to the device.  If keepOld (RECOMMENDED),
 * will preserve old osccal and BG bits.
 */
void
usb_pickit_write (usb_pickit *d, pic14_state *s, bool keep_old)
{
  /* save old config bits */
  int keep_eeprom;
  pic14_config oldconfig;

  /* calculate checksum by software */
  usb_pickit_calc_checksum (s);
  printf ("calculated checksum from .hex file: %#04x\n",
	  s->program.instchecksum);

  if (keep_old)
    usb_pickit_read_config (d, &oldconfig);

  if (s->program.max_ee == 0)
    keep_eeprom = 1;
  else
    keep_eeprom = 0;

  usb_pickit_reset (d, keep_eeprom);

  /* write new program to device */
  usb_pickit_write_eeprom (d, &s->program);
  usb_pickit_write_program (d, &s->program);

/*
 * Ho-Ro - Check disabled
 * Checksums do not match b/c they are calculated differently:
 * Checksum by PICKit uses new program code and old config word.
 * Config word will be merged from old and new config values
 * and written later
 */
#if 0
  /*
   * JEB - calculate checksum, and get checksum from PICkit, compare
   * and then announce result.  this is the same way as it is done in
   * the software that comes with the PICKit.  more reliable to verify
   * after the write, same functionality as Windows version.
   */

  /* calculate checksum by software */
  usb_pickit_calc_checksum (s);
  printf ("calculated checksum from .hex file: %#04x\n",
	  s->program.instchecksum);

  /* calculate checksum by PICKit */
  usb_pickit_read_checksum (d, s);
  pic14_word pgmchecksum = ((s->config.config & s->config.configmask)
		 + s->config.pgmchecksum) & 0xffff;
  printf ("checksum from PICKit: %#20x\n", pgmchecksum);

  if (s->program.instchecksum == pgmchecksum)
    printf ("checksums are equal: device programming successful.\n");
  else
    printf ("checksum verify failed: error in programming!\n");
#endif

  /*
   * JEB - moved config word program to after program memory because
   * othererwise, would set the code protect bits before the write of
   * program and data memory, which would not allow the write of
   * program or data memory.  works fine with 2.0.2 firmware.
   */
  if (keep_old)
    {
      /* normal case: merge new and old configs */
      usb_pickit_merge_config (d, &oldconfig, &s->config);
    }
  else
    {
      /* DANGEROUS: blast in new config */
      usb_pickit_write_config (d, &s->config);
    }
}

/*
 * erase device. (JEB)
 * checks to see if save_osccal is set, and if so preserves osccal and
 * BG bits.
 */
void
usb_pickit_erase (usb_pickit *d, pic14_state *s)
{
  pic14_config oldconfig;
  pic14_word bgbits;

  /* if OscCal device, save old config bits */
  if (s->config.save_osccal)
    usb_pickit_read_config (d, &oldconfig);

  /* wipe device */
  usb_pickit_reset (d, 0);

  /* if needed, write in saved config bits */
  if (s->config.save_osccal)
    {
      /* write OSCCAL to 0x03ff */
      send_usb (d, "V0V1PI\xff\x03");
      send_usb_word (d, oldconfig.osccal);

      /* restore BG bits and then write configuration word to 0x2007 */
      bgbits = (0x3000 & oldconfig.config) | s->config.configmask;
      send_usb (d, "pPCI\x07\x00ZZ");
      send_usb_word (d, bgbits);
      send_usb (d, "pV1ZZZZZ");
    }

  printf ("device erased.\n");
}

/*
 * do a hard chip reset.  you *must* preserve config first.
 */
void
usb_pickit_reset (usb_pickit *d, bool keep_eeprom)
{
  /* blank out the device completely */
  if (keep_eeprom)
    send_usb (d, "PCEpZZZZ");
  else
    send_usb (d, "PCEepZZZ");
}

/*
 * merge oldconfig with newconfig.  keep OSCCAL and Bandgap from
 * oldconfig, the other parameters are taken from newconfig.
 *
 * the merged config is written to device.
 */
void
usb_pickit_merge_config (usb_pickit *d, pic14_config *oldconfig,
			 pic14_config *newconfig)
{
  pic14_config merged = *newconfig;

#define BG_MASK 0x3000 /* mask to extract bandgap bits */

  merged.osccal = oldconfig->osccal;
  merged.config = (oldconfig->config & BG_MASK)
    + (newconfig->config & ~BG_MASK);

  usb_pickit_write_config (d, &merged);
}

/*
 * set Bandgap bits. (JEB)
 * for 629, 675, 630 and 676 only.
 */
void
usb_pickit_set_bandgap (usb_pickit *d, pic14_state *s, int bgarg)
{
  pic14_config oldconfig;
  pic14_word configword, bg;

  bg = (pic14_word)bgarg;

  if (bg > 3)
    {
      fprintf (stderr, "Error: bandgap must be between 0 and 3");
      exit (-1);
    }

  /* 629, 675, 630 or 676, only 629, 675, 630 and 676 devices
     use osccal/bg, check if we are dealing with one of them */
  if (s->config.save_osccal)
    {
      /* get old OSCCAL to preserve */
      usb_pickit_read_config (d, &oldconfig);

      /* wipe device */
      usb_pickit_reset (d, 0);

      /* write OSCCAL to 0x03ff */
      send_usb (d, "V0V1PI\xff\x03");
      send_usb_word (d, oldconfig.osccal);

      /* insert BG bits and then write CONFIG word to 0x2007 */
      configword = (bg << 12) | s->config.configmask;

      send_usb (d, "pPCI\x07\x00ZZ");
      send_usb_word (d, configword);
      send_usb (d, "pV1ZZZZZ");

      printf ("device erased.\n");
      printf ("OSCCAL 0x%04x reprogrammed.\n", oldconfig.osccal);
      printf ("Bandgap 0x%1x programmed.\n", bg);
    }
  else
    {
      fprintf (stderr, "Error programming Bandgap.\n");
      fprintf (stderr, "reason: only PIC 629, 675, 630 and 676 "
	       "support Bandgap bits.\n");
    }
}

/*
 * regenerate OSCCAL using the PICkit 2.5 kHz oscillator and the
 * autocal.hex file that comes with the PICkit. (JEB)
 *
 * in general, one should try not to lose the OSCCAL in the first
 * place but this is a reasonable backup.  if you need to get really
 * accurate onboard oscillator function then you need to write a
 * program that twiddles a bit, accounts for the instruction cycle
 * timing and then use an oscilloscope to measure the output.
 * you can also do this with the by selecting the internal Osc with
 * clkout option and watch that pin with your scope.  this is the only
 * way to get a really accurate IntOsc and is still probably varies a
 * bit with temperature.
 *
 * this function is for use with the 629, 675, 630 and 676 devices
 * only.
 */
void
usb_pickit_osccal_regen (usb_pickit *d, pic14_state *s)
{
  pic14_word configword, osccal;
  byte eedata[8];

  /* if 629, 675, 630 or 676, only devices to use osccal/bg */
  if (s->config.save_osccal)
    {
      /* read CONFIG word from 0x2007, and power off device */
      send_usb (d, "pPCI\x07\x00ZZ");
      recv_usb_words (d, 1, &configword);
      send_usb (d, "pV1ZZZZZ");

      /* start PICkit 2.5 kHz Osc and then power up device.
	 delay and then power down device */
      usb_pickit_osc_on (d);
      sleep (1);
      usb_pickit_off (d);

      /* get the calibrated value stored in the last location of
	 data memory */
      send_usb (d, "PI\x78\x00rpZZ");
      recv_usb (d, 8, eedata);

      /* wipe device */
      usb_pickit_reset (d, 0);

      /* write OSCCAL to 0x03ff */
      osccal = eedata[7];
      osccal = osccal | 0x3400; /* or with 0x34 to create retlw value */

      send_usb (d, "pV1ZZZZZ");
      send_usb (d, "PI\xff\x03ZZZZ");
      send_usb_word (d, osccal);

      /* write configuration word to 0x2007 */
      configword = (0x3000 & configword) | s->config.configmask;

      send_usb (d, "pPCI\x07\x00ZZ");
      send_usb_word (d, configword);
      send_usb (d, "pV1ZZZZZ");

      printf ("device erased.\n");
      printf ("OSCCAL 0x%04x regenerated and programmed.\n", osccal);
      printf ("Config Word & Bandgap 0x%04x restored.\n", configword);
    }
  else
    {
      fprintf (stderr, "Error regenerating OSCCAL.\n");
      fprintf (stderr, "reason: only PIC 629, 675, 630 and 676 "
	       "support OSCCAL regeneration.\n");
    }
}

/*
 * print program memory map of the device.
 */
static void
usb_pickit_program_map (usb_pickit *d, pic14_state *s)
{
  int i, j;
  pic14_addr memlength;
  memlength = s->program.inst_len;

  printf ("program memory:\n");

  /* include last byte (OscCal) for 629, 675, 630 and 676 devices */
  if(s->config.save_osccal)
    {
      memlength = memlength + 1;
      s->program.inst[s->program.inst_len] = s->config.osccal;
    }

  /* print program memory */
  for (i = 0; i < memlength; i += 8)
    {
      printf ("Addr 0x%04x:[", i);
      for (j = 0; j < 8; ++j)
	{
	  printf ("0x%04x", s->program.inst[i + j]);
	  printf ("%s", (j < 7) ? " " : "]\n");
	}
    }

  printf ("\n");
}

/*
 * print data memory map of the device.
 */
static void
usb_pickit_eeprom_map (usb_pickit *d, pic14_state *s)
{
  int i, j;

  printf ("EEPROM data memory:\n");

  for (i = 0; i < s->program.ee_len; i += 8)
    {
      printf ("Addr 0x%02x:[", i);
      for (j = 0; j < 8; ++j)
	{
	  printf ("0x%02x", s->program.ee[i + j]);
	  printf ("%s", (j < 7) ? " " : "]\n");
	}
    }

  printf ("\n");
}

/*
 * print the memory map of the device. (JEB)
 * first the program memory to the length specified in the get_device
 * routine, and then the eeprom data memory.
 *
 * this is a convenient way to visually inspect the device if needed.
 * gives most of the functionality of the GUI based programs and is a
 * good substitute until I have time to write a Cocoa GUI interface.
 */
void
usb_pickit_memory_map (usb_pickit *d, pic14_state *s)
{
  usb_pickit_program_map (d, s);
  usb_pickit_eeprom_map (d, s);
}

/*
 * print the whole configuration set (osccal, id, and config word).
 *
 * JEB - here I have added code to print the device code that Mark
 * Rages added support for, and to print the OSCCAL value if it
 * exists.  also uses the functions I have added to compute the device
 * checksum using the PICkit 1 firmware, which is different than the
 * calculated checksum.  lastly, added code to mask off the ID bits
 * above 7.  this is specified by all Microship programmers
 */
void
usb_pickit_print_config (usb_pickit *d, pic14_state *s)
{
  pic14_word id[8], osccal[1], checksum[2];
  char cmd[REQ_LEN + 1];
  int i;

  /* read OSCCAL from 0x3ff */
  if (s->config.save_osccal)
    {
      send_usb (d, "V0V1PI\xff\x03");
      recv_usb_words (d, 1, osccal);
      printf("               OSCCAL data: [0x03ff]=0x%04x\n", osccal[0]);
    }

  /* now reset and read 8 configuration bytes at 0x2000 */
  send_usb (d, "pV0V1PCZ");
  recv_usb_words (d, 8, id);
  send_usb (d, "pV1ZZZZZ");

  for (i = 0; i < 4; ++i)
    printf("          configuration ID: [0x%04x]=0x%02x\n",
	   0x2000 + i, id[i] & 0x7f);

  for (i = 4; i < 8; ++i)
    printf("        configuration data: [0x%04x]=0x%04x\n",
	   0x2000 + i, id[i] );

  printf("        masked CONFIG word: 0x%04x\n",
	 id[7] & s->config.configmask);

  if (s->config.save_osccal)
    printf ("       masked Bandgap bits: 0x%01x\n",
	    (id[7] & 0x3000) >> 12);

  /* read programmer checksum values  */
  sprintf (cmd, "S____V1Z");

  cmd[1] = (char)(s->program.inst_len & 0xff);
  cmd[2] = (char)(s->program.inst_len >> 8);
  cmd[3] = (char)(s->program.ee_len & 0xff);
  cmd[4] = (char)(s->program.ee_len >> 8);

  send_usb (d, cmd);
  recv_usb_words (d, 2, checksum);
  send_usb (d, "pV1ZZZZZ");

  printf ("PICkit Programmer checksum: 0x%04x\n", checksum[0]);
  printf ("PICkit Prg+Config checksum: 0x%04x\n", (checksum[0]
		     + (id[7] & s->config.configmask)) & 0xffff);
  printf ("PICkit Prgrmr chksm EEData: 0x%02x\n", checksum[1] & 0x00ff);
}

/*
 * compare program memory between .hex file and device.
 * Return true if they are equal.
 */
static int
usb_pickit_verify_program (pic14_state *file, pic14_state *dev)
{
  int i;

  for (i = 0; i < dev->program.inst_len; ++i)
    {
      if (file->program.inst[i] != dev->program.inst[i])
	{
	  fprintf (stderr, "Error: program memory does not match "
		   "with .hex file!\n");
	  return 0;
	}
    }

  return 1;
}

/*
 * compare program memory checksums between .hex file and device.
 * Return true if they are equal.
 */
static int
usb_pickit_verify_program_checksum (pic14_state *file, pic14_state *dev)
{
  if (file->program.instchecksum != dev->program.instchecksum)
    {
      fprintf (stderr, "Error: program memory checksum does not "
	       "match with .hex file!\n");
      return 0;
    }

  return 1;
}

/*
 * compare configuration words between .hex file and device.
 * Return true if they are equal.
 */
static int
usb_pickit_verify_config_word (pic14_state *file, pic14_state *dev)
{
  if ((file->config.config & dev->config.configmask) !=
      (dev->config.config & dev->config.configmask))
    {
      fprintf (stderr, "Error: CONFIG word does not match with .hex file!\n");
      return 0;
    }

  return 1;
}

/*
 * compare configuration IDs between .hex file and device.
 * Return true if they are equal.
 */
static int
usb_pickit_verify_config_id (pic14_state *file, pic14_state *dev)
{
  int i;

  for (i = 0; i < 4; ++i)
    {
      if ((file->config.id[i] & 0x7f) != (dev->config.id[i] & 0x7f))
	{
	  fprintf (stderr, "Error: config IDs don't match with .hex file!\n");
	  return 0;
	}
    }

  return 1;
}

/*
 * compare EEPROM content between .hex file and device.
 * Return true if they are equal.
 */
static int
usb_pickit_verify_eeprom (pic14_state *file, pic14_state *dev)
{
  int i;

  for (i = 0; i < dev->program.ee_len; ++i)
    {
      if (file->program.ee[i] != dev->program.ee[i])
	{
	  fprintf (stderr, "Error: EE Data memory does not "
		   "match with .hex file!\n");
	  return 0;
	}
    }

  return 1;
}

/*
 * JEB - This routine called from main to do all
 * of the comparisons for a .hex file to device verify operation.
 */
int
usb_pickit_verify (pic14_state *file, pic14_state *dev)
{
  if (usb_pickit_verify_program (file, dev) &&
      usb_pickit_verify_program_checksum (file, dev) &&
      usb_pickit_verify_config_word (file, dev) &&
      usb_pickit_verify_config_id (file, dev) &&
      usb_pickit_verify_eeprom (file, dev))
    {
      printf ("device successfully verified with .hex file.\n");
      return 1;
    }

  fprintf (stderr, "Error: device failed to verify with .hex file!\n");
  return 0;
}

/*
 * return true if program memory is blank.
 */
static int
usb_pickit_blank_check_program (pic14_state *s)
{
  int i;

  for (i = 0; i <s->program.inst_len; ++i)
    {
      if (s->program.inst[i] != 0x3fff)
	{
	  fprintf (stderr, "Error: program memory is not blank!\n");
	  return 0;
	}
    }

  return 1;
}

/*
 * return true if configuration word is blank.
 */
static int
usb_pickit_blank_check_config_word (pic14_state *s)
{
  if ((s->config.config & s->config.configmask) !=
      (0x3fff & s->config.configmask))
    {
      fprintf (stderr, "Error: CONFIG word is not blank!\n");
      return 0;
    }

  return 1;
}

/*
 * return true if configuration IDs are blank.
 */
static int
usb_pickit_blank_check_config_id (pic14_state *s)
{
  int i;

  for (i = 0; i < 4; ++i)
    {
      if ((s->config.id[i] & 0x7f) != 0x7f)
	{
	  fprintf (stderr, "Error: config IDs are not blank!\n");
	  return 0;
	}
    }

  return 1;
}

/*
 * return true if EEPROM memory is blank.
 */
static int
usb_pickit_blank_check_eeprom (pic14_state *s)
{
  int i;

  for (i = 0; i < s->program.ee_len; ++i)
    {
      if (s->program.ee[i] != 0xff)
	{
	  fprintf (stderr, "Error: EE Data Memory is not blank!\n");
	  return 0;
	}
    }

  return 1;
}

/*
 * JEB - This routine called from main to do all of the
 *  comparisons to blank check the device.
 */
int
usb_pickit_blank_check (pic14_state *s)
{
  if (usb_pickit_blank_check_program (s) &&
      usb_pickit_blank_check_config_word (s) &&
      usb_pickit_blank_check_config_id (s) &&
      usb_pickit_blank_check_eeprom (s))
    {
      printf ("device is blank.\n");
      return 1;
    }

  return 0;
}
