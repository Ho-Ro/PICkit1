/*
 * pic14.c
 *
 * Orion Sky Lawlor, olawlor@acm.org, 2003/8/3
 * Jeff Boly, jboly@teammojo.org, 2005/12/31
 * David Henry, tfc_duke@club-internet.fr, 2006/9/1
 *
 * This code is licenced under the MIT license.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code.
 *
 * Programmable state of the 14-bit instruction word Microchip PIC
 * microcontrollers, such as the PIC 12F675 or the PIC 16F684.
 */

#include "common.h"
#include "pic14.h"
#include "hex.h"

/*
 * describes an address range that can be treated uniformly
 */
typedef struct
{
  const char *desc; /* human-readable description */
  pic14_addr addr;  /* first word address */
  pic14_addr len;   /* number of word addresses */
  pic14_word *data; /* actual data (in a pic14_program) */

} pic14_span;

/* list of spans */
enum {
  SPAN_PROGRAM,
  SPAN_EEPROM,
  SPAN_CONFIG,
  SPAN_USERID,
  SPAN_OSCCAL,

  PIC14_PROGRAM_NSPANS
};


/*
 * initialize to a reasonable power-on value.
 */
void
pic14_state_init (pic14_state *p)
{
  unsigned int i;

  /* clear program memory */
  for (i = 0; i < PIC14_INST_LEN; ++i)
    p->program.inst[i] = 0x3fff;

  /* clear EEPROM data memory */
  for (i = 0; i < PIC14_EE_LEN; ++i)
    p->program.ee[i] = 0xff;

  /* currently no data to write */
  p->program.max_prog = 0;
  p->program.max_ee = 0;

  /*
   * default configuration word value.
   *
   * JEB - 0x184 is a good value for the 12F devices.
   * better to set explicity in your .hex file
   *
   * disable watchdog and code protect, enable INTOSCIO
   */
  p->config.config = 0x184;

  /* clear configuration memory */
  for (i = 0; i < PIC14_ID_LEN; ++i)
    p->config.id[i] = 0x3fff;

  p->config.osccal = 0x2000;
}

/*
 * get a device info, given a device ID.
 * return NULL if not found.
 */
const pic14_device_info*
pic14_get_device (pic14_word id)
{
  const pic14_device_info *d = __devices;

  /* look through each device until 0xffff device id
     (end of list marker) */
  while (d->device_id != 0xffff)
    {
      if (d->device_id == id)
	return d;

      d++;
    }

  return NULL;
}

/*
 * extract a list of spans from this program.
 */
static void
pic14_program_spans (pic14_state *p, pic14_span s[])
{
  pic14_span *ps;

  ps = &s[SPAN_PROGRAM];
  ps->desc = "Program (14-bit instruction words)";
  ps->addr = 0x0000;
  ps->len  = p->program.inst_len;
  ps->data = p->program.inst;

  ps = &s[SPAN_EEPROM];
  ps->desc = "EEPROM (8-bit data)";
  ps->addr = 0x2100; /* only meaningful in Intel MDS HEX file */
  ps->len  = p->program.ee_len;
  ps->data = p->program.ee;

  ps = &s[SPAN_CONFIG];
  ps->desc = "Configuration word";
  ps->addr = 0x2007;
  ps->len  = 1;
  ps->data = &p->config.config;

  ps = &s[SPAN_USERID];
  ps->desc = "User ID words";
  ps->addr = 0x2000;
  ps->len  = PIC14_ID_LEN;
  ps->data = p->config.id;

  ps = &s[SPAN_OSCCAL];
  ps->desc = "OSCCAL";
  ps->addr = 0x03ff;
  ps->len  = 1;
  ps->data = &p->config.osccal;
}

/*
 * write this word wherever it belongs in this span list.
 */
void
pic14_write_word (pic14_span *spans, unsigned int addr,
		  pic14_word w, pic14_state *p)
{
  unsigned int s;
  unsigned int index;

  /* look for the span of this word */
  for (s = 0; s < PIC14_PROGRAM_NSPANS; ++s)
    {
      if ((addr >= spans[s].addr) &&
	  (addr < spans[s].addr + spans[s].len))
	{
	  /* this is our span, write us into it */
	  index = addr - spans[s].addr;
	  spans[s].data[index] = w;

	  /* go from index of last element (0-based) to length (count
	     from 1) */
	  index++;

	  switch (s)
	    {
	    case SPAN_PROGRAM:
	      if (index > p->program.max_prog)
		p->program.max_prog = index;
	      break;

	    case SPAN_EEPROM:
	      if (index > p->program.max_ee)
		p->program.max_ee = index;
	      break;

	    case SPAN_CONFIG:
	      /*
	       * JEB - print a message to let the user know that a
	       * config word was found in the .HEX file.
	       * this is based on a recommendation from Microchop to
	       * let the user know about where there config value
	       * comes from.
	       */
	      printf (".hex file contains a configuration word\n");
	      break;
	    }

	  return;
	}
    }
}

/*
 * accept this segment of a pic14 program from a .hex file, in which
 * everything is stored as *bytes*, not words.
 */
static void
pic14_hex_segment (void *vp, unsigned int baddr,
		   unsigned int blen, byte *src)
{
  unsigned int i, addr = baddr/2, len = blen/2;
  pic14_state *p = (pic14_state *)vp;
  pic14_span spans[PIC14_PROGRAM_NSPANS];

  pic14_program_spans (p, spans);

  for (i = 0; i < len; ++i)
    {
      pic14_write_word (spans, addr + i,
	src[2 * i + 0] + (src[2 * i + 1] << 8), p);
    }
}

/*
 * read a program from a .hex file.  Return non-zero value
 * if success.
 */
int
pic14_hex_read (pic14_state *p, FILE *src)
{
  return hex_read (src, pic14_hex_segment, p);
}

/*
 * write a program to a .hex file.
 */
void
pic14_hex_write (pic14_state *p, FILE *dest)
{
  pic14_span spans[PIC14_PROGRAM_NSPANS];
  int s;

  pic14_program_spans (p, spans);
  hex_write_begin (dest);

  for (s = 0; s < PIC14_PROGRAM_NSPANS; ++s)
    {
      /* Must convert address and data from words to bytes: */
      byte data[2 * PIC14_INST_LEN];
      unsigned int addr = 2 * spans[s].addr;
      unsigned int w, len = 2 * spans[s].len;

      for (w = 0; w < spans[s].len; ++w)
	{
	  unsigned int v = spans[s].data[w];

	  data[2 * w + 0] = (byte)(0xff & v); /* Low end first */
	  data[2 * w + 1] = (byte)(0xff & (v >> 8)); /* High end second */
	}

      hex_write (dest, addr, len, data);
    }

  hex_write_end (dest);
}

#ifdef TEST_PIC_HEX
#include <stdlib.h>

/*
 * test out pic14 .hex file reading and writing, by using the reader
 * and writer to copy a .hex.
 */
int
main (int argc, char *argv[])
{
  FILE *src, *dest;
  pic14_state p;

  if (argc != 3)
    {
      printf ("usage: %s <in file> <out file>\n", argv[0]);
      exit (EXIT_FAILURE);
    }

  src = fopen (argv[1], "r");
  dest = fopen (argv[2], "w");

  pic14_state_init (&dfile.state);

  if (!pic14_hex_read (&p, src))
    {
      fprintf (stderr, "Error reading hex file %s\n", argv[1]);
      return 0;
    }

  pic14_hex_write (&p, dest);

  return 0;
}
#endif /* TEST_PIC_HEX */
