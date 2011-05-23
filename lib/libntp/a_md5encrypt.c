/*	$NetBSD: a_md5encrypt.c,v 1.6 1999/07/03 12:30:28 simonb Exp $	*/

/*
 *  md5crypt - MD5 based authentication routines
 */

#include "ntp_types.h"
#include "ntp_string.h"
#include "md5.h"
#include "ntp_stdlib.h"

extern u_int32 cache_keyid;
extern char *cache_key;
extern int cache_keylen;

#ifndef HAVE_MEMMOVE
extern void *memmove P((void *, const void *, size_t));
#endif

/*
 * Stat counters, imported from data base module
 */
extern u_int32 authencryptions;
extern u_int32 authdecryptions;
extern u_int32 authkeyuncached;
extern u_int32 authnokey;

/*
 * For our purposes an NTP packet looks like:
 *
 *	a variable amount of encrypted data, multiple of 8 bytes, followed by:
 *	NOCRYPT_OCTETS worth of unencrypted data, followed by:
 *	BLOCK_OCTETS worth of ciphered checksum.
 */ 
#define	NOCRYPT_OCTETS	4
#define	BLOCK_OCTETS	16

#define	NOCRYPT_int32S	((NOCRYPT_OCTETS)/sizeof(u_int32))
#define	BLOCK_int32S	((BLOCK_OCTETS)/sizeof(u_int32))


int
MD5authencrypt(keyno, pkt, length)
    u_int32 keyno;
    u_int32 *pkt;
    int length;		/* length of encrypted portion of packet */
{
    MD5_CTX ctx;
    int len;		/* in 4 byte quantities */
#ifdef __NetBSD__
    unsigned char hash[16];
#endif

    authencryptions++;

    if (keyno != cache_keyid) {
	authkeyuncached++;
	if (!authhavekey(keyno)) {
	    authnokey++;
	    return 0;
	}
    }

    len = length / sizeof(u_int32);

    /*
     *  Generate the authenticator.
     */
    MD5Init(&ctx);
    MD5Update(&ctx, (unsigned const char *)cache_key, cache_keylen);
    MD5Update(&ctx, (unsigned const char *)pkt, length);
#ifdef __NetBSD__
    MD5Final(hash, &ctx);
#else
    MD5Final(&ctx);
#endif

    memmove((char *)&pkt[NOCRYPT_int32S + len],
#ifdef __NetBSD__
	    (char *) hash,
#else
	    (char *) ctx.digest,
#endif
	    BLOCK_OCTETS);
    return (4 + BLOCK_OCTETS);	/* return size of key and MAC  */
}
