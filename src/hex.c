/*
 * hex.c
 *
 * Orion Sky Lawlor, olawlor@acm.org, 2003/8/3
 * David Henry, tfc_duke@club-internet.fr, 2006/9/1
 *
 * This code is licenced under the MIT license.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code.
 *
 * Intel MDS .hex file read and writer.
 */

#include <ctype.h>
#include "hex.h"

#define HEX_MAX_BYTES 16
#define HEX_MAX_DATA_LINE 64

/*
 * begin writing a .hex file here.
 */
void
hex_write_begin (FILE *fp)
{
  /* nothing to do, although you might set a type 02 or
     type 04 address marker here... */
}

/*
 * write this byte, updating checksum.
 */
void
hex_write_byte (FILE *fp, unsigned int bi, byte *checksum)
{
  byte b = (byte)(0xff & bi);
  fprintf (fp, "%02X", (int)b);
  *checksum -= b;
}

/*
 * write data to the .hex file at the given address.
 * can write any number of bytes of data -- splits lines internally.
 */
void
hex_write (FILE *fp, unsigned int addr, unsigned int len, byte *data)
{
  if (len <= HEX_MAX_BYTES)
    {
      /* write out one line */
      unsigned int i;
      byte checksum = 0;

      /* write line's header */
      fprintf (fp, ":");
      hex_write_byte (fp, len, &checksum);
      hex_write_byte (fp, addr >> 8, &checksum);
      hex_write_byte (fp, addr, &checksum);
      hex_write_byte (fp, 0x00, &checksum);

      /* write the data */
      for (i = 0; i < len; ++i)
	hex_write_byte (fp, data[i], &checksum);

      /* write the checksum */
      hex_write_byte (fp, checksum, &checksum);
      fprintf (fp, "\n");
    }
  else
    {
      /* len is too long -- split into smaller lines */
      while (len > 0)
	{
	  int n = len;
	  if (n > HEX_MAX_BYTES)
	    n = HEX_MAX_BYTES;

	  hex_write (fp, addr, n, data);

	  addr += n;
	  len -= n;
	  data += n;
	}
    }
}

/*
 * finish writing a .hex file here.
 */
void
hex_write_end (FILE *fp)
{
  /* write special end marker */
  fprintf (fp, ":00000001FF\n");
}

/*
 * read a .hex file from here, sending the resulting
 * address spans to this function.  return non-zero value if success.
 */
int
hex_read (FILE *fp, hex_dest_fn fn, void *param)
{
  unsigned int addrbase16 = 0; /* DOS-style "segment" of program */
  unsigned int addrbase32 = 0; /* high 16 bits of program counter */

  if (!fp)
    {
      fprintf (stderr, "Error reading .hex file: "
	       "could not open file!\n");
      return 0;
    }

  while (1)
    {
      /*
       * read one line of the .hex file:
       *  : <len> <addr hi> <addr lo> <type> [ < data > ] <checksum>
       */
      unsigned int addr, i, v, len, type, checksum = 0;
      byte data[HEX_MAX_DATA_LINE + 1];

      /* seek to start of next line */
      int ic;
      while (EOF != (ic = fgetc (fp)))
	{
	  char c = (char)ic;

	  if (c == ':')
	    {
	      /* start of next line */
	      break;
	    }
	  else if (!isspace (c))
	    {
	      fprintf (stderr, "Error reading .hex file: "
		       "unexpected characters!\n");
	      return 0;
	    }
	}

      if (ic == EOF)
	{
	  /* hit EOF */
	  return 1;
	}

      /* read address and length of line */
      if (3 != fscanf (fp, "%02x%04x%02x", &len, &addr, &type))
	{
	  fprintf (stderr, "Error reading .hex file: "
		   "unexpected start-of-line format!\n");
	  return 0;
	}

      checksum += len;
      checksum += addr + (addr >> 8);
      checksum += type;

      /* ensure line lenght is not too long */
      if (len > HEX_MAX_DATA_LINE)
	{
	  fprintf (stderr, "Error reading .hex file: "
		   "line too long!\n");
	  return 0;
	}

      /* read data and checksum for line */
      for (i = 0; i < len + 1; ++i)
	{
	  if (1 != fscanf (fp, "%02x", &v))
	    {
	      fprintf (stderr, "Error reading .hex file: "
		       "unexpected data format!\n");
	      return 0;
	    }

	  data[i] = v;
	  checksum += v;
	}

      if (0 != (checksum & 0xff))
	{
	  fprintf (stderr, "Error reading .hex file: "
		   "line checksum mismatch!\n");
	  return 0;
	}

      /* handle line */
      switch (type)
	{
	case 0x00:
	  /* regular data line -- pass data to user */
	  (fn)(param, addrbase16 + addrbase32 + addr, len, data);
	  break;

	case 0x01:
	  /* end-of-file line -- exit happily */
	  return 1;

	case 0x02:
	  /* latch bits 8-24 of PC */
	  addrbase16 = addr << 8;
	  break;

	case 0x04:
	  /* latch bits 16-32 of PC */
	  addrbase32 = addr << 16;
	  break;

	default:
	  fprintf (stderr, "Error reading .hex file: "
		   "unrecognized line type!\n");
	  return 0;
	}
    }
}

#ifdef TEST_HEX
#include <stdlib.h>

/*
 * test out .hex file reading and writing, by using the reader
 * and writer to copy a .hex.
 */
int
main (int argc, char *argv[])
{
  FILE *src, *dest;

  if (argc != 3)
    {
      printf ("usage: %s <in file> <out file>\n", argv[0]);
      exit (EXIT_FAILURE);
    }

  src = fopen (argv[1], "r");
  dest = fopen (argv[2], "w");

  hex_write_begin (dest);

  if (!hex_read (src, (hex_dest_fn)hex_write, dest))
    {
      fprintf (stderr, "Error reading hex file %s\n", argv[1]);
      return 0;
    }

  hex_write_end (dest);

  return 0;
}
#endif /* TEST_HEX */
