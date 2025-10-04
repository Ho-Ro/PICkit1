/*
 * devices.c
 *
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
 * Supported devices by the PICKit1 programmer.
 */

#include "pic14.h"

/* list of supported devices by the programmer */
const pic14_device_info __devices[] = {

  /* begin OscCal devices */
  {
    .device_id = 0x0f80,
    .device_name = "12F629",
    .inst_len = 0x03ff,
    .ee_len = 128,
    .save_osccal = 1,
    .configmask = 0x1ff
  },
  {
    .device_id = 0x0fc0,
    .device_name = "12F675",
    .inst_len = 0x03ff,
    .ee_len = 128,
    .save_osccal = 1,
    .configmask = 0x1ff
  },
  {
    .device_id = 0x10c0,
    .device_name = "16F630",
    .inst_len = 0x03ff,
    .ee_len = 128,
    .save_osccal = 1,
    .configmask = 0x1ff
  },
  {
    .device_id = 0x10e0,
    .device_name = "16F676",
    .inst_len = 0x03ff,
    .ee_len = 128,
    .save_osccal = 1,
    .configmask = 0x1ff
  },
  /* end OscCal devices */
  {
    .device_id = 0x0fa0,
    .device_name = "16F635",
    .inst_len = 0x0400,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0x1fff
  },
  {
    .device_id = 0x0460,
    .device_name = "16F683",
    .inst_len = 0x0800,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0xfff
  },
  {
    .device_id = 0x10a0,
    .device_name = "16F636/639",
    .inst_len = 0x0800,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0x1fff
  },
  {
    .device_id = 0x1080,
    .device_name = "16F684",
    .inst_len = 0x0800,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0xfff
  },
  {
    .device_id = 0x04a0,
    .device_name = "16F685",
    .inst_len = 0x1000,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0xfff
  },
  {
    .device_id = 0x1320,
    .device_name = "16F687",
    .inst_len = 0x0800,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0xfff
  },
  {
    .device_id = 0x1180,
    .device_name = "16F688",
    .inst_len = 0x1000,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0xfff
  },
  {
    .device_id = 0x1340,
    .device_name = "16F689",
    .inst_len = 0x1000,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0xfff
  },
  {
    .device_id = 0x1400,
    .device_name = "16F690",
    .inst_len = 0x1000,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0xfff
  },
  /*
   * JEB - added more devices based on 2.0.2 firmware
   * JEB - Note, some devices require an adapter.
   * JEB - I soldered up a simple 14 pin to 18 pin adapter.
   * JEB - See Microchip TB079 for more info.
   */
  {
    .device_id = 0x1140,
    .device_name = "16F716",
    .inst_len = 0x0800,
    .ee_len = 0,
    .save_osccal = 0,
    .configmask = 0x0cf
  },
  {
    /* note: only A version works with PICkit */
    .device_id = 0x1040,
    .device_name = "16F627A",
    .inst_len = 0x0400,
    .ee_len = 128,
    .save_osccal = 0,
    .configmask = 0x0ff
  },
  {
    /* note: only A version works with PICkit */
    .device_id = 0x1060,
    .device_name = "16F628A",
    .inst_len = 0x0800,
    .ee_len = 128,
    .save_osccal = 0,
    .configmask = 0x0ff
  },
  {
    /* note: only A version works with PICkit */
    .device_id = 0x1100,
    .device_name = "16F648A",
    .inst_len = 0x1000,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0x0ff
  },
  {
    .device_id = 0x1200,
    .device_name = "16F785",
    .inst_len = 0x0800,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0x0fff
  },
  {
    .device_id = 0x0e20,
    .device_name = "16F877A",
    .inst_len = 0x2000,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0x2fc7
  },
  {
    .device_id = 0x13e0,
    .device_name = "16F913",
    .inst_len = 0x1000,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0x1fff
  },
  {
    .device_id = 0x13c0,
    .device_name = "16F914",
    .inst_len = 0x1000,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0x1fff
  },
  {
    .device_id = 0x1380,
    .device_name = "16F917",
    .inst_len = 0x2000,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0x1fff
  },
  {
    .device_id = 0x13a0,
    .device_name = "16F916",
    .inst_len = 0x2000,
    .ee_len = 256,
    .save_osccal = 0,
    .configmask = 0x1fff
  },
  /* do not insert any device info after this line! */
  {
    .device_id = 0xffff,
    .device_name = "Last Device Entry",
  },
};
