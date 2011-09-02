#ifndef _BLOWFISH_H_dd6fff1b_
#define _BLOWFISH_H_dd6fff1b_

/* porting defines: BLOWFISH__U32 should be unsigned integer type with
   at least 32 bits.  It can be wider, but the extra space will be wasted.
   The code assumes 8-bit chars; if chars are wider, only the low 8 bits
   of each will be used. */

typedef unsigned int BLOWFISH__U32;

/* Opaque structures */

/* BLOWFISH_KEY is a key schedule, filled in by blowfish_setkey. */

typedef struct {
	  BLOWFISH__U32 BLOWFISH__P[18];
	  BLOWFISH__U32 BLOWFISH__S[4][256];
	  } BLOWFISH_KEY;

/* Routine interfaces */

/* blowfish_setkey fills in a BLOWFISH_KEY given the key. */
extern void blowfish_setkey(BLOWFISH_KEY *, const void *, int);

/* blowfish_encrypt does encryption, blowfish_decrypt decryption. */
extern void blowfish_encrypt(const BLOWFISH_KEY *, const void *, void *);
extern void blowfish_decrypt(const BLOWFISH_KEY *, const void *, void *);

#endif
