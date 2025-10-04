/*
 * usb_pickit.h
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
 * Header for a USB interface to the Microchip(tm) PICkit(tm) 1 FLASH
 * Starter Kit device programmer and breadboard.
 *
 * These functions deal directly with the PICkit programmer.
 */

#ifndef __USB_PICKIT_H__
#define __USB_PICKIT_H__

#include "pic14.h"

typedef struct usb_dev_handle usb_pickit;

/* open the pickit as a usb device.  aborts on errors */
usb_pickit *usb_pickit_open ();

/* close the usb pickit device */
void usb_pickit_close (usb_pickit *d);


/* turn the device on */
void usb_pickit_on (usb_pickit *d);

/* turn the device off */
void usb_pickit_off (usb_pickit *d);

/* turn the 2.5 kHz osc on */
void usb_pickit_osc_on (usb_pickit *d);

/* turn the 2.5 kHz osc off */
void usb_pickit_osc_off (usb_pickit *d);


/* read device type */
int usb_pickit_get_device (usb_pickit *d, pic14_device *dev);


/* JEB - generate checksum from what was read into memory */
void usb_pickit_calc_checksum (pic14_state *s);

/* JEB - read checksum direct from programmer using "S" function */
void usb_pickit_read_checksum (usb_pickit *d, pic14_state *s);


/* read current EEPROM Data memory from the device. */
void usb_pickit_read_eeprom (usb_pickit *d, pic14_program *p);

/* read current program memory from the device. */
void usb_pickit_read_program (usb_pickit *d, pic14_program *p);

/* read current configuration from the device. */
void usb_pickit_read_config (usb_pickit *d, pic14_config *c);

/* fill out this state with the contents of the device */
void usb_pickit_read (usb_pickit *d, pic14_state *s);


/* write program data to device's EEPROM (requires reset first) */
void usb_pickit_write_eeprom (usb_pickit *d, pic14_program *p);

/* write program instructions to the device (requires reset first) */
void usb_pickit_write_program (usb_pickit *d, pic14_program *p);

/* send off this config.  WARNING: do not reset OSCCAL and BG bits! */
void usb_pickit_write_config (usb_pickit *d, pic14_config *c);

/* write this state.  if keepOld is set (RECOMMENDED), will
   preserve old OSCCAL and BG bits */
void usb_pickit_write (usb_pickit *d, pic14_state *s, bool keepOld);


/* JEB - erase device.  Preserve OSCCAL and BG bits if needed */
void usb_pickit_erase (usb_pickit *d, pic14_state *s);

/* do a hard chip reset. You *must* preserve config first.
   This clears both program and config, you must then
   call write_program and merge_config (or write_config) */
void usb_pickit_reset (usb_pickit *d, bool keepEeprom);

/* send off this configuration (requires reset first).
   Copies OSCCAL, ID, and BG bits from oldconfig, everything
   else from newconfig. (these are the preserved bits) */
void usb_pickit_merge_config (usb_pickit *d,
	pic14_config *oldconfig, pic14_config *newconfig);


/* JEB - set bandgap bits.  for 629, 675, 630 and 676 only */
void usb_pickit_set_bandgap (usb_pickit *d, pic14_state *s, int bgarg);

/* JEB - regenerate OSCCAL using autocal.hex file.  for 629, 675, 630
   and 676 only */
void usb_pickit_osccal_regen (usb_pickit *d,pic14_state *s);


/* print the device memory map, display all program and data memory values */
void usb_pickit_memory_map (usb_pickit *d, pic14_state *s);

/* print the whole configuration set (osccal, id, and config word).
   JEB - added state as function input to enhance config output */
void usb_pickit_print_config (usb_pickit *d, pic14_state *s);


/* JEB - .hex file to device verify operation */
int usb_pickit_verify (pic14_state *file, pic14_state *dev);

/* JEB - device blank check */
int usb_pickit_blank_check (pic14_state *s);

#endif /* __USB_PICKIT_H__ */
