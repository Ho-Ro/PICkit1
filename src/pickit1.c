/*
 * pickit1.c
 *
 * David Henry, tfc_duke@club-internet.fr, 2007/1/27
 *
 * This code is licenced under the MIT license.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code.
 *
 * Command line front end for PICKit1 programmer.
 */

#include <stdlib.h>
#include <string.h>
#include <popt.h>
#include "usb_pickit.h"

/* program's "about" description */
static const char *description =
  "Microchip(tm) PICkit(tm) 1 USB Programmer controller program\n"
  "Rewritten by David Henry, tfc_duke@club-internet.fr, 2006/8/20\n"
  "Based on version 1.5 by Jeff Boly, jboly@teammojo.org, 2005/12/25\n"
  "Other contributions by Mark Rages, markrages@gmail.com, 2005/4/1\n"
  "Original Code, Orion Sky Lawlor, olawlor@acm.org, 2004/1/19\n\n";

/* autocal.hex path */
static const char *autocal = "autocal.hex";

/* declaration of program's mode functions */
static int pickit1_program (usb_pickit *d, const char *filename, bool programall);
static int pickit1_extract (usb_pickit *d, const char *filename);
static int pickit1_verify (usb_pickit *d, const char *filename);
static int pickit1_blank_check (usb_pickit *d);
static int pickit1_erase (usb_pickit *d);
static int pickit1_memory_map (usb_pickit *d);
static int pickit1_config (usb_pickit *d);
static int pickit1_reset (usb_pickit *d);
static int pickit1_off (usb_pickit *d);
static int pickit1_on (usb_pickit *d);
static int pickit1_oscoff (usb_pickit *d);
static int pickit1_oscon (usb_pickit *d);
static int pickit1_bandgap (usb_pickit *d, int bg);
static int pickit1_osccal_regen (usb_pickit **d);

#ifdef DEBUG
static int pickit1_test_write_program (usb_pickit *d);
static int pickit1_test_write_eeprom (usb_pickit *d);
#endif

/* list of programer's options */
enum programer_mode {
  OPT_PROGRAM = 1, /* pickit1_program */
  OPT_EXTRACT,     /* pickit1_extract */
  OPT_VERIFY,      /* pickit1_verify */
  OPT_BLANKCHECK,  /* pickit1_blank_check */
  OPT_ERASE,       /* pickit1_erase */
  OPT_MEMORYMAP,   /* pickit1_memory_map */
  OPT_CONFIG,      /* pickit1_config */
  OPT_RESET,       /* pickit1_reset */
  OPT_OFF,         /* pickit1_off */
  OPT_ON,          /* pickit1_on */
  OPT_OSCOFF,      /* pickit1_oscoff */
  OPT_OSCON,       /* pickit1_oscon */
  OPT_BANDGAP,     /* pickit1_bandgap */
  OPT_OSCCALREGEN, /* pickit1_osccal_regen */
  OPT_PROGRAMALL,  /* pickit1_program */

#ifdef DEBUG
  OPT_TEST_WR_PROGRAM, /* pickit1_test_write_program */
  OPT_TEST_WR_EEPROM,  /* pickit1_test_write_eeprom */
#endif
};

/*
 * write a .hex file to the PIC.
 */
static int
pickit1_program (usb_pickit *d, const char *filename, bool programall)
{
  pic14_device dev;
  FILE *fp;

  fp = fopen (filename, "r");
  if (!fp)
    {
      perror ("Could not open program file");
      return 0;
    }

  /* zero out the state first, so anything that isn't read
     won't be uninitialized */
  pic14_state_init (&dev.state);

  /* find the device on the PICKit board */
  if (!usb_pickit_get_device (d, &dev))
    {
      fclose (fp);
      return 0;
    }

  /* read the .hex file containing the program to burn to the PIC */
  if (!pic14_hex_read (&dev.state, fp))
    {
      fclose (fp);
      return 0;
    }

  fclose (fp);

  /* write the program and exit */
  if (programall)
    usb_pickit_write (d, &dev.state, 0);
  else
    usb_pickit_write (d, &dev.state, 1);

  return 1;
}

/*
 * extract program and EEPROM data memory from a PIC
 * and write them in an output file.
 */
static int
pickit1_extract (usb_pickit *d, const char *filename)
{
  pic14_device dev;
  FILE *fp;

  fp = fopen (filename, "w+");
  if (!fp)
    {
      perror ("Could not create output file");
      return 0;
    }

  /* zero out the state first, so anything that isn't read
     won't be uninitialized */
  pic14_state_init (&dev.state);

  /* find the device on the PICKit board */
  if (!usb_pickit_get_device (d, &dev))
    {
      fclose (fp);
      return 0;
    }

  /* read memory from the device */
  usb_pickit_read (d, &dev.state);

  /* JEB added calc checksum function */
  usb_pickit_calc_checksum (&dev.state);

  /* write the program to output file */
  pic14_hex_write (&dev.state, fp);
  fclose (fp);

  return 1;
}

/*
 * verify the contents of the device with a .hex file. (JEB)
 */
static int
pickit1_verify (usb_pickit *d, const char *filename)
{
  pic14_device dev, dfile;
  FILE *fp;

  fp = fopen (filename, "r");
  if (!fp)
    {
      perror ("Could not open program file");
      return 0;
    }

  /* zero out the state first, so anything that isn't read
     won't be uninitialized */
  pic14_state_init (&dfile.state);

  /* find the device on the PICKit board */
  if (!usb_pickit_get_device (d, &dfile))
    {
      fclose (fp);
      return 0;
    }

  /* read the .hex file containing the program to verify */
  if (!pic14_hex_read (&dfile.state, fp))
    {
      fclose (fp);
      return 0;
    }

  fclose (fp);

  usb_pickit_calc_checksum (&dfile.state);

  dev.state.config.configmask = dfile.state.config.configmask;
  dev.state.program.inst_len = dfile.state.program.inst_len;
  dev.state.program.ee_len = dfile.state.program.ee_len;

  usb_pickit_read (d, &dev.state);
  usb_pickit_calc_checksum (&dev.state);
  usb_pickit_verify (&dfile.state, &dev.state);

  return 1;
}

/*
 * check to make sure chip is blank. (JEB)
 */
static int
pickit1_blank_check (usb_pickit *d)
{
  pic14_device dev;

  /* zero out the state first, so anything that isn't read
     won't be uninitialized */
  pic14_state_init (&dev.state);

  /* find the device on the PICKit board */
  if (!usb_pickit_get_device (d, &dev))
    return 0;

  usb_pickit_read (d, &dev.state);
  usb_pickit_calc_checksum (&dev.state);
  usb_pickit_blank_check (&dev.state);

  return 1;
}

/*
 * erase the PIC, program and data memory.
 */
static int
pickit1_erase (usb_pickit *d)
{
  pic14_device dev;

  /* zero out the state first, so anything that isn't read
     won't be uninitialized */
  pic14_state_init (&dev.state);

  /* find the device on the PICKit board */
  if (!usb_pickit_get_device (d, &dev))
    return 0;

  usb_pickit_erase (d, &dev.state);

  return 1;
}

/*
 * print current program and data memory from the PIC.
 */
static int
pickit1_memory_map (usb_pickit *d)
{
  pic14_device dev;

  /* zero out the state first, so anything that isn't read
     won't be uninitialized */
  pic14_state_init (&dev.state);

  /* find the device on the PICKit board */
  if (!usb_pickit_get_device (d, &dev))
    return 0;

  usb_pickit_read (d, &dev.state);
  usb_pickit_memory_map (d, &dev.state);

  return 1;
}

/*
 * print PIC's current configuration words.
 */
static int
pickit1_config (usb_pickit *d)
{
  pic14_device dev;

  /* zero out the state first, so anything that isn't read
     won't be uninitialized */
  pic14_state_init (&dev.state);

  /* find the device on the PICKit board */
  if (!usb_pickit_get_device (d, &dev))
    return 0;

  usb_pickit_print_config (d, &dev.state);

  return 1;
}

/*
 * make a hard reset of the PIC.
 */
static int
pickit1_reset (usb_pickit *d)
{
  usb_pickit_off (d);
  usb_pickit_on (d);

  return 1;
}

/*
 * power off the PIC
 */
static int
pickit1_off (usb_pickit *d)
{
  usb_pickit_off (d);

  return 1;
}

/*
 * power on the PIC
 */
static int
pickit1_on (usb_pickit *d)
{
  usb_pickit_on (d);

  return 1;
}

/*
 * disable the 2.5 kHz oscillator.
 */
static int
pickit1_oscoff (usb_pickit *d)
{
  usb_pickit_osc_off (d);

  return 1;
}

/*
 * enable the 2.5 kHz oscillator.
 */
static int
pickit1_oscon (usb_pickit *d)
{
  usb_pickit_osc_on (d);

  return 1;
}

/*
 * set PIC's bandgap.
 * NOTE: for 629, 675, 630 and 676 devices only.
 */
static int
pickit1_bandgap (usb_pickit *d, int bg)
{
  pic14_device dev;

  /* zero out the state first, so anything that isn't read
     won't be uninitialized */
  pic14_state_init (&dev.state);

  /* find the device on the PICKit board */
  if (!usb_pickit_get_device (d, &dev))
    return 0;

  usb_pickit_set_bandgap (d, &dev.state, bg);

  return 1;
}

/*
 * regenerate OSCCAL from the 2.5 kHz oscillator.
 */
static int
pickit1_osccal_regen (usb_pickit **d)
{
  pic14_device dev;
  FILE *fp;

  fp = fopen (autocal, "r");
  if (!fp)
    {
      perror ("Could not open the autocal.hex file");
      return 0;
    }

  /* zero out the state first, so anything that isn't read
     won't be uninitialized */
  pic14_state_init (&dev.state);

  /* find the device on the PICKit board */
  if (!usb_pickit_get_device (*d, &dev))
    return 0;

  if (dev.state.config.save_osccal)
    {
      if (!pic14_hex_read (&dev.state, fp))
	{
	  fclose (fp);
	  return 0;
	}

      fclose (fp);
      usb_pickit_write (*d, &dev.state, 1);

      /*
       * JEB - For some reason, have to close the USB device and reopen
       * to get the OscCal regen to work.  This function is not used
       * all that often, so it's not worth trying to figure out why
       * this is so.  It works as is, but just prints the Device found
       * info twice because of the multiple open calls.
       */

      usb_pickit_close (*d);
      *d = usb_pickit_open ();
      usb_pickit_osccal_regen (*d, &dev.state);
    }
  else
    {
      fprintf (stderr, "Only PIC 629, 675, 630, 676 support "
	       "OscCalRegeneration.\n");
      return 0;
    }

  return 1;
}

#ifdef DEBUG
/*
 * write dummy data to program memory
 * !!!TEST
 */
static int
pickit1_test_write_program (usb_pickit *d)
{
  pic14_device dev;
  int i, j;

  /* zero out the state first, so anything that isn't read
     won't be uninitialized */
  pic14_state_init (&dev.state);

  /* find the device on the PICKit board */
  if (!usb_pickit_get_device (d, &dev))
    return 0;

  printf ("== Program memory writing test ==\n");

  dev.state.program.max_prog = dev.state.program.inst_len;

  for (i = 0; i < dev.state.program.inst_len; i += 8)
    for (j = 0; j < 8; ++j)
      dev.state.program.inst[i + j] = i + j;

  usb_pickit_write (d, &dev.state, 1);

  return 1;
}
#endif /* DEBUG */

#ifdef DEBUG
/*
 * write dummy data to data memory
 * !!!TEST
 */
static int
pickit1_test_write_eeprom (usb_pickit *d)
{
  pic14_device dev;
  int i, j;

  /* zero out the state first, so anything that isn't read
     won't be uninitialized */
  pic14_state_init (&dev.state);

  /* find the device on the PICKit board */
  if (!usb_pickit_get_device (d, &dev))
    return 0;

  printf ("== EEPROM Data memory writing test ==\n");

  dev.state.program.max_ee = dev.state.program.ee_len;

  for (i = 0; i < dev.state.program.ee_len; i += 8)
    for (j = 0; j < 8; ++j)
      dev.state.program.ee[i + j] = i + j;

  usb_pickit_write (d, &dev.state, 1);

  return 1;
}
#endif /* DEBUG */

/*
 * programer's main entry point.  enter the proper mode given
 * parameters passed to the program.
 */
int
main (int argc, const char *argv[])
{
  usb_pickit *d = NULL;
  char *filename = NULL;
  int bg, rc;

  /* programer's command line options */
  struct poptOption options[] = {
    { "program", 'p', POPT_ARG_STRING, &filename, OPT_PROGRAM,
      "Writes .hex file to chip", "<file>" },
    { "extract", 'x', POPT_ARG_STRING, &filename, OPT_EXTRACT,
      "Read from chip into .hex file", "<file>" },
    { "verify", 'v', POPT_ARG_STRING, &filename, OPT_VERIFY,
      "Read from chip and compare with .hex file", "<file>" },
    { "blankcheck", 'b', POPT_ARG_NONE, NULL, OPT_BLANKCHECK,
      "Read chip, check all locations for 1 or blank", NULL },
    { "erase", 'e', POPT_ARG_NONE, NULL, OPT_ERASE,
      "Erase device.  Preserve OscCal and BG Bits if implemented", NULL },
    { "memorymap", 'm', POPT_ARG_NONE, NULL, OPT_MEMORYMAP,
      "Show device Program and EE Data Memory", NULL },
    { "config", 'c', POPT_ARG_NONE, NULL, OPT_CONFIG,
      "Show configuration data", NULL },
    { "reset", 'r', POPT_ARG_NONE, NULL, OPT_RESET,
      "Power cycle the chip", NULL },
    { "off", '\0', POPT_ARG_NONE, NULL, OPT_OFF,
      "Turn chip power off", NULL },
    { "on", '\0', POPT_ARG_NONE, NULL, OPT_ON,
      "Turn chip power back on", NULL },
    { "oscoff", '\0', POPT_ARG_NONE, NULL, OPT_OSCOFF,
      "Turn 2.5 kHz osc off, leave chip on", NULL },
    { "oscon", '\0', POPT_ARG_NONE, NULL, OPT_OSCON,
      "Turn 2.5 kHz osc on, with chip on", NULL },
    { "bandgap", '\0', POPT_ARG_INT, &bg, OPT_BANDGAP,
      "Erase device.  Preserve OscCal and write specified BG Bits", "<int>" },
    { "osccalregen", '\0', POPT_ARG_NONE, NULL, OPT_OSCCALREGEN,
      "Erase device.  Regenerate OscCal using autocal.hex", NULL },
    { "programall", '\0', POPT_ARG_STRING, &filename, OPT_PROGRAMALL,
      "Overwrite OscCal and BG (dangerous!)", "<file>" },
#ifdef DEBUG
    { "testprog", '\0', POPT_ARG_NONE, NULL, OPT_TEST_WR_PROGRAM,
      "Test write program memory", NULL },
    { "testee", '\0', POPT_ARG_NONE, NULL, OPT_TEST_WR_EEPROM,
      "Test write data memory", NULL },
#endif
    POPT_AUTOHELP
    POPT_TABLEEND
  };

  /* context for parsing command-line options */
  poptContext poptcon = poptGetContext (NULL, argc, argv, options, 0);
  poptSetOtherOptionHelp (poptcon, "[OPTION]");

  /* peek first option, ignore others */
  rc = poptGetNextOpt (poptcon);
  if (rc > 0)
    {
      /* open PICKit device */
      if (NULL == (d = usb_pickit_open ()))
	exit (EXIT_FAILURE);

      switch (rc)
	{
	case OPT_PROGRAM:
	  rc = pickit1_program (d, filename, 0);
	  break;

	case OPT_EXTRACT:
	  rc = pickit1_extract (d, filename);
	  break;

	case OPT_VERIFY:
	  rc = pickit1_verify (d, filename);
	  break;

	case OPT_BLANKCHECK:
	  rc = pickit1_blank_check (d);
	  break;

	case OPT_ERASE:
	  rc = pickit1_erase (d);
	  break;

	case OPT_MEMORYMAP:
	  rc = pickit1_memory_map (d);
	  break;

	case OPT_CONFIG:
	  rc = pickit1_config (d);
	  break;

	case OPT_RESET:
	  rc = pickit1_reset (d);
	  break;

	case OPT_OFF:
	  rc = pickit1_off (d);
	  break;

	case OPT_ON:
	  rc = pickit1_on (d);
	  break;

	case OPT_OSCOFF:
	  rc = pickit1_oscoff (d);
	  break;

	case OPT_OSCON:
	  rc = pickit1_oscon (d);
	  break;

	case OPT_BANDGAP:
	  rc = pickit1_bandgap (d, bg);
	  break;

	case OPT_OSCCALREGEN:
	  rc = pickit1_osccal_regen (&d);
	  break;

	case OPT_PROGRAMALL:
	  rc = pickit1_program (d, filename, 1);
	  break;

#ifdef DEBUG
	case OPT_TEST_WR_PROGRAM:
	  rc = pickit1_test_write_program (d);
	  break;

	case OPT_TEST_WR_EEPROM:
	  rc = pickit1_test_write_eeprom (d);
	  break;
#endif /* DEBUG */
	}

      usb_pickit_close (d);
    }
  else
    {
      fprintf (stderr, "%s", description);

      if (rc < -1)
	{
	  fprintf (stderr, "Error: %s", poptStrerror (rc));
	  fprintf (stderr, " `%s'\n\n", poptBadOption (poptcon, 0));
	}

      poptPrintHelp (poptcon, stderr, 0);
    }

  poptFreeContext (poptcon);

  return rc;
}
