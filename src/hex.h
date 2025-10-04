/*
 * hex.h
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

#ifndef __HEX_H__
#define __HEX_H__

#include <stdio.h>
#include "common.h"


/*
 * begin writing a .hex file here
 */
void hex_write_begin (FILE *fp);

/*
 * write data to the given address.  can write any number of bytes
 * of data -- splits lines internally.
 * WARNING: only low 16 bits of address supported.
 */
void hex_write (FILE *fp, unsigned int addr,
		unsigned int len, byte *data);


/*
 * finish writing a .hex file here
 */
void hex_write_end (FILE *fp);

/*
 * a user-supplied function containing what to do with the .hex spans
 * once they're read in.
 */
typedef void (*hex_dest_fn)(void *param, unsigned int addr,
			    unsigned int len, byte *data);

/*
 * read a .hex file from here, sending the resulting address
 * spans to this function.   returns non-zero value on success;
 * works with type 02 and type 04 to supply full 32 bits of address.
 */
int hex_read (FILE *fp, hex_dest_fn fn, void *param);

#endif /* __HEX_H__ */
