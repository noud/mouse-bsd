#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <string.h>
#else
#include <lib/libkern/libkern.h>
#endif

#include <sys/systm.h>

#include <sys/blowfish.h>

typedef BLOWFISH__U32 u32;

#define P BLOWFISH__P
#define S BLOWFISH__S

extern u32 blowfish__pi[];

static __inline__ u32 F(const BLOWFISH_KEY *k, u32 v)
{
 return( ( ( k->S[0][(v>>24)&0xff] +
	     k->S[1][(v>>16)&0xff] ) ^
	   k->S[2][(v>>8)&0xff] ) +
	 k->S[3][v&0xff] );
}

void blowfish_setkey(BLOWFISH_KEY *ks, const void *key, int keylen)
{
 int i;
 int j;
 int x;
 unsigned char blk[8];
 u32 v;

 if (keylen < 1) return;
 if (keylen > 56) keylen = 56; /* why? because the spec says so. */
 x = 0;
 for (i=0;i<18;i++) ks->P[i] = blowfish__pi[x++];
 for (i=0;i<4;i++) for (j=0;j<256;j++) ks->S[i][j] = blowfish__pi[x++];
 x = 0;
 for (i=0;i<18;i++)
  { v = ((const unsigned char *)key)[x++];
    if (x >= keylen) x = 0;
    v = (v << 8) | ((const unsigned char *)key)[x++];
    if (x >= keylen) x = 0;
    v = (v << 8) | ((const unsigned char *)key)[x++];
    if (x >= keylen) x = 0;
    v = (v << 8) | ((const unsigned char *)key)[x++];
    if (x >= keylen) x = 0;
    ks->P[i] ^= v;
  }
 bzero(&blk[0],8);
 for (i=0;i<18;i+=2)
  { blowfish_encrypt(ks,&blk[0],&blk[0]);
    ks->P[i] = (blk[0] * 0x01000000) |
	       (blk[1] * 0x00010000) |
	       (blk[2] * 0x00000100) |
	       (blk[3] * 0x00000001);
    ks->P[i+1] = (blk[4] * 0x01000000) |
		 (blk[5] * 0x00010000) |
		 (blk[6] * 0x00000100) |
		 (blk[7] * 0x00000001);
  }
 for (i=0;i<4;i++)
  { for (j=0;j<256;j+=2)
     { blowfish_encrypt(ks,&blk[0],&blk[0]);
       ks->S[i][j] = (blk[0] * 0x01000000) |
		     (blk[1] * 0x00010000) |
		     (blk[2] * 0x00000100) |
		     (blk[3] * 0x00000001);
       ks->S[i][j+1] = (blk[4] * 0x01000000) |
		       (blk[5] * 0x00010000) |
		       (blk[6] * 0x00000100) |
		       (blk[7] * 0x00000001);
     }
  }
}

void blowfish_encrypt(const BLOWFISH_KEY *k, const void *blkin, void *blkout)
{
 u32 l;
 u32 r;
 int round;

 l = (((const unsigned char *)blkin)[0] * 0x01000000) |
     (((const unsigned char *)blkin)[1] * 0x00010000) |
     (((const unsigned char *)blkin)[2] * 0x00000100) |
     (((const unsigned char *)blkin)[3] * 0x00000001);
 r = (((const unsigned char *)blkin)[4] * 0x01000000) |
     (((const unsigned char *)blkin)[5] * 0x00010000) |
     (((const unsigned char *)blkin)[6] * 0x00000100) |
     (((const unsigned char *)blkin)[7] * 0x00000001);
 for (round=0;round<16;round+=2)
  { l ^= k->P[round];
    r ^= F(k,l) ^ k->P[round+1];
    l ^= F(k,r);
  }
 l ^= k->P[16];
 r ^= k->P[17];
 ((unsigned char *)blkout)[0] = (r >> 24) & 0xff;
 ((unsigned char *)blkout)[1] = (r >> 16) & 0xff;
 ((unsigned char *)blkout)[2] = (r >>  8) & 0xff;
 ((unsigned char *)blkout)[3] =  r        & 0xff;
 ((unsigned char *)blkout)[4] = (l >> 24) & 0xff;
 ((unsigned char *)blkout)[5] = (l >> 16) & 0xff;
 ((unsigned char *)blkout)[6] = (l >>  8) & 0xff;
 ((unsigned char *)blkout)[7] =  l        & 0xff;
}

void blowfish_decrypt(const BLOWFISH_KEY *k, const void *blkin, void *blkout)
{
 u32 l;
 u32 r;
 int round;

 l = (((const unsigned char *)blkin)[0] * 0x01000000) |
     (((const unsigned char *)blkin)[1] * 0x00010000) |
     (((const unsigned char *)blkin)[2] * 0x00000100) |
     (((const unsigned char *)blkin)[3] * 0x00000001);
 r = (((const unsigned char *)blkin)[4] * 0x01000000) |
     (((const unsigned char *)blkin)[5] * 0x00010000) |
     (((const unsigned char *)blkin)[6] * 0x00000100) |
     (((const unsigned char *)blkin)[7] * 0x00000001);
 for (round=17;round>=2;round-=2)
  { l ^= k->P[round];
    r ^= F(k,l) ^ k->P[round-1];
    l ^= F(k,r);
  }
 l ^= k->P[1];
 r ^= k->P[0];
 ((unsigned char *)blkout)[0] = (r >> 24) & 0xff;
 ((unsigned char *)blkout)[1] = (r >> 16) & 0xff;
 ((unsigned char *)blkout)[2] = (r >>  8) & 0xff;
 ((unsigned char *)blkout)[3] =  r        & 0xff;
 ((unsigned char *)blkout)[4] = (l >> 24) & 0xff;
 ((unsigned char *)blkout)[5] = (l >> 16) & 0xff;
 ((unsigned char *)blkout)[6] = (l >>  8) & 0xff;
 ((unsigned char *)blkout)[7] =  l        & 0xff;
}
