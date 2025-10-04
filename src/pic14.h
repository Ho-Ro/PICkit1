/*
 * pic14.h
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
 * Header file to describe the programmable state
 * of the 14-bit instruction word Microchip PIC microcontrollers,
 * such as the PIC 12F675 or the PIC 16F684.
 */

#ifndef __PIC14_H__
#define __PIC14_H__

#include <stdio.h>
#include "common.h"


/* one storage location in the EEPROM has this type */
typedef unsigned char pic14_ee;

/* one instruction has this type (but only low 14 bits are used) */
typedef unsigned short pic14_inst;

/* one data or program memory reference has this type */
typedef unsigned short pic14_word;

/* a program address has this type */
typedef unsigned int pic14_addr;


/*
 * program state for pic14-series microcontroller.
 * contains two distinct regions: program memory region (composed of
 * 14-bit word instructions) and EEPROM data memory region (composed
 * of 8-bit bytes).
 */
typedef struct
{
  /* JEB - changed from 0x0fff to 0x01fff to go up to 8K for newer
     devices */
#define PIC14_INST_LEN 0x01fff /* up to 8192 words of program */

  /* regular program memory runs from 0x0000 to 0x0fff. */
  pic14_addr inst_len;
  pic14_word inst[PIC14_INST_LEN];
  pic14_addr max_prog; /* number of program words to write */

  /* JEB - computed checksum from memory buffer stored here */
  pic14_word instchecksum;

#define PIC14_EE_LEN 256 /* 256 bytes of EEPROM */
  /*
   * EEPROM data is available at offsets 0-127 or 255.
   * only the low 8 bits are actually stored.
   */
  pic14_addr ee_len;
  pic14_word ee[PIC14_EE_LEN];
  pic14_addr max_ee; /* number of data bytes to write */

} pic14_program;


/*
 * hard configuration state for pic14 microprocessor
 */
typedef struct
{
  /* oscilator calibration word, stored at 0x3ff. */
  pic14_word osccal;

#define PIC14_ID_LEN 4
  /*
   * special configuration memory "User ID" words, at
   * configuration address 0x2000-0x2003.
   * supposedly, only the low 7 bits are usable.
   */
  pic14_word id[PIC14_ID_LEN];

  /* special configuration word, at 0x2007 */
  pic14_word config;

  /* save OSCCAL, or not? */
  bool save_osccal;

  /* JEB - config word mask value for computing checksum */
  pic14_word configmask;

  /* JEB - read program checksum from "S" command stored here.
     must be and'ed with masked config value */
  pic14_word pgmchecksum;

  /* JEB - read EE data checksum stored here */
  byte eechecksum;

} pic14_config;


/*
 * All programmable states on a pic14.
 */
typedef struct
{
  pic14_program program;
  pic14_config config;

} pic14_state;


/* initialize this state to a reasonable power-up value. */
void pic14_state_init (pic14_state *p);


/*
 * a 14-bit instruction PIC device info structure.
 * it stores device's ID, name and constant parameters such as
 * program memory lenght, EEPROM lenght, configuration word mask
 * and if the OSCCAL byte must be saved before being erased.
 */
typedef struct
{
  /* device ID word */
  pic14_word device_id;

  /* a human-readable string for the device name */
  const char *device_name;

  /*
   * device's architecture related parameters:
   * program memory lenght, EE data memory lenght,
   * do we need to save OSCCAL and CONFIG word's mask
   */
  pic14_addr inst_len;
  pic14_addr ee_len;

  unsigned char save_osccal;
  pic14_word configmask;

} pic14_device_info;

/* list of supported devices by the programmer -- those are
   initialized at compile time */
extern const pic14_device_info __devices[];

/* get a device info, given a device ID */
const pic14_device_info *pic14_get_device (pic14_word id);


/*
 * a 14-bit instruction PIC device structure.
 * this holds constant information about the device (in a
 * pic14_device_info structure) and other infos proper to the device
 * (like revision number).
 */
typedef struct
{
  /* constant infos about the device */
  const pic14_device_info *dinfo;

  /* device revision number */
  pic14_word rev;

  /* device state */
  pic14_state state;

} pic14_device;


/* read this program from a .hex file.  returns non-zero
   value on success. */
int pic14_hex_read (pic14_state *p, FILE *src);

/* write this program to a .hex file */
void pic14_hex_write (pic14_state *p, FILE *dest);

#endif /* __PIC14_H__ */
