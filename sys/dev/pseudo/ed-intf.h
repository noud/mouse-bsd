#ifndef _ED_INTF_H_e40b75c5_
#define _ED_INTF_H_e40b75c5_

/* On a write to the control device, the write data is a command.
   The first byte is a command; the rest is command-specific.
   The commands and their additional data are: */

/* Set the encryption key for a unit.  Data:
   affected unit number (one byte) and key data (rest of write). */
#define ED_CMD_SETKEY 1

/* Set a unit to prompt on the console for its encryption key.  Data:
   affected unit number (one byte). */
#define ED_CMD_SETPROMPT 2

/* Set the underlying device for a unit.  Data:
   affected unit number (one byte) and fd open on the underlying device
   (sizeof(int) bytes).  The fd will be closed. */
#define ED_CMD_SETDEV 3

/* Reset a unit.  This clears any key and/or underlying device info.  Data:
   affected unit number (one byte). */
#define ED_CMD_RESET 4

#endif
