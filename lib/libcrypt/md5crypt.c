#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/time.h>

#include "internal.h"

#define VERSION_MD5 1

#define DEF_VERSION VERSION_MD5

#define MIN_ROUNDS 2
#define DEF_ROUNDS 512
#define MAX_ROUNDS 99999

/* MD5 stuff, mostly straight out of the RFC */

typedef unsigned long int U32; /* unsigned, 32 bits */

/* T[i] = floor((1<<32)*abs(sin(i+1))), where sin arg is in radians */
static U32 T[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };

typedef struct state STATE;
struct state {
  U32 A;
  U32 B;
  U32 C;
  U32 D;
  U32 X[16];
  int xfill; /* # of bytes added since last crunch_block() */
  U32 bitlen_l;
  U32 bitlen_h;
  } ;

#define F(X,Y,Z) (((X)&(Y))|((~(X))&(Z)))
#define G(X,Y,Z) (((X)&(Z))|((Y)&(~(Z))))
#define H(X,Y,Z) ((X)^(Y)^(Z))
#define I(X,Y,Z) ((Y)^((X)|(~(Z))))

/* rotate v left nbits bits */
__inline__ static U32 ROTLEFT(U32 v, unsigned int nbits)
{
 return((v<<nbits)|(v>>(32-nbits)));
}

/*
 * A block of 64 input bytes has been accumulated in X[]; crunch it
 *  through the hash machine.  Essentially a transcription of the RFC
 *  algorithm into C.
 */
static void crunch_block(STATE *state)
{
 U32 A;
 U32 B;
 U32 C;
 U32 D;
 U32 *X;

 A = state->A;
 B = state->B;
 C = state->C;
 D = state->D;
 X = &state->X[0];
#define OP(a,b,c,d,k,s,i) a = b + ROTLEFT(a+F(b,c,d)+X[k]+T[i-1],s)
 OP(A,B,C,D, 0, 7, 1);OP(D,A,B,C, 1,12, 2);OP(C,D,A,B, 2,17, 3);OP(B,C,D,A, 3,22, 4);
 OP(A,B,C,D, 4, 7, 5);OP(D,A,B,C, 5,12, 6);OP(C,D,A,B, 6,17, 7);OP(B,C,D,A, 7,22, 8);
 OP(A,B,C,D, 8, 7, 9);OP(D,A,B,C, 9,12,10);OP(C,D,A,B,10,17,11);OP(B,C,D,A,11,22,12);
 OP(A,B,C,D,12, 7,13);OP(D,A,B,C,13,12,14);OP(C,D,A,B,14,17,15);OP(B,C,D,A,15,22,16);
#undef OP
#define OP(a,b,c,d,k,s,i) a = b + ROTLEFT(a+G(b,c,d)+X[k]+T[i-1],s)
 OP(A,B,C,D, 1, 5,17);OP(D,A,B,C, 6, 9,18);OP(C,D,A,B,11,14,19);OP(B,C,D,A, 0,20,20);
 OP(A,B,C,D, 5, 5,21);OP(D,A,B,C,10, 9,22);OP(C,D,A,B,15,14,23);OP(B,C,D,A, 4,20,24);
 OP(A,B,C,D, 9, 5,25);OP(D,A,B,C,14, 9,26);OP(C,D,A,B, 3,14,27);OP(B,C,D,A, 8,20,28);
 OP(A,B,C,D,13, 5,29);OP(D,A,B,C, 2, 9,30);OP(C,D,A,B, 7,14,31);OP(B,C,D,A,12,20,32);
#undef OP
#define OP(a,b,c,d,k,s,i) a = b + ROTLEFT(a+H(b,c,d)+X[k]+T[i-1],s)
 OP(A,B,C,D, 5, 4,33);OP(D,A,B,C, 8,11,34);OP(C,D,A,B,11,16,35);OP(B,C,D,A,14,23,36);
 OP(A,B,C,D, 1, 4,37);OP(D,A,B,C, 4,11,38);OP(C,D,A,B, 7,16,39);OP(B,C,D,A,10,23,40);
 OP(A,B,C,D,13, 4,41);OP(D,A,B,C, 0,11,42);OP(C,D,A,B, 3,16,43);OP(B,C,D,A, 6,23,44);
 OP(A,B,C,D, 9, 4,45);OP(D,A,B,C,12,11,46);OP(C,D,A,B,15,16,47);OP(B,C,D,A, 2,23,48);
#undef OP
#define OP(a,b,c,d,k,s,i) a = b + ROTLEFT(a+I(b,c,d)+X[k]+T[i-1],s)
 OP(A,B,C,D, 0, 6,49);OP(D,A,B,C, 7,10,50);OP(C,D,A,B,14,15,51);OP(B,C,D,A, 5,21,52);
 OP(A,B,C,D,12, 6,53);OP(D,A,B,C, 3,10,54);OP(C,D,A,B,10,15,55);OP(B,C,D,A, 1,21,56);
 OP(A,B,C,D, 8, 6,57);OP(D,A,B,C,15,10,58);OP(C,D,A,B, 6,15,59);OP(B,C,D,A,13,21,60);
 OP(A,B,C,D, 4, 6,61);OP(D,A,B,C,11,10,62);OP(C,D,A,B, 2,15,63);OP(B,C,D,A, 9,21,64);
#undef OP
 state->A += A;
 state->B += B;
 state->C += C;
 state->D += D;
}

/*
 * Feed some bytes to MD5.  Accumulates them into X[], calling
 *  crunch_block() at each 64-byte boundary to process them.
 */
static void crunch_bytes(STATE *state, const unsigned char *buf, unsigned int nbytes)
{
 int xfill;
 U32 *X;

 xfill = state->xfill;
 X = &state->X[0];
 for (;nbytes>0;nbytes--)
  { switch (xfill % 4)
     { case 0:
	  X[xfill/4] = *buf;
	  break;
       case 1:
	  X[xfill/4] |= ((U32)*buf) << 8;
	  break;
       case 2:
	  X[xfill/4] |= ((U32)*buf) << 16;
	  break;
       case 3:
	  X[xfill/4] |= ((U32)*buf) << 24;
	  break;
     }
    buf ++;
    xfill ++;
    if (xfill >= 64)
     { crunch_block(state);
       xfill = 0;
     }
  }
 state->xfill = xfill;
}

/*
 * Initialize the state, per the RFC.
 */
static void md5_init(STATE *s)
{
 s->A = 0x67452301;
 s->B = 0xefcdab89;
 s->C = 0x98badcfe;
 s->D = 0x10325476;
 s->xfill = 0;
 s->bitlen_l = 0;
 s->bitlen_h = 0;
}

/*
 * Process some bytes.  Feed them to crunch_bytes() and also update the
 *  bitlen values to reflect the number of bits fed in.
 */
static void md5_process_bytes(STATE *s, const void *vbuf, unsigned int nbytes)
{
 U32 ltmp;
 U32 lsum;

 crunch_bytes(s,vbuf,nbytes);
 s->bitlen_h += ((U32)nbytes) >> 29;
 ltmp = ((U32)nbytes) << 3;
 lsum = ltmp + s->bitlen_l;
 if ( (ltmp & s->bitlen_l & 0x80000000) ||
      ( ((ltmp|s->bitlen_l) & 0x80000000) &&
	!(lsum & 0x80000000) ) )
  { s->bitlen_h ++;
  }
 s->bitlen_l = lsum;
#undef s
}

/*
 * Finish the MD5 algorithm; insert padding and the bitstring length,
 *  then extract the result into the supplied 16-byte buffer.
 */
static void md5_result(STATE *s, unsigned char *result)
{
 unsigned char lenbytes[8];

 crunch_bytes(s,"\200",1);
 while (s->xfill != 56) crunch_bytes(s,"\0",1);
 lenbytes[0] =  s->bitlen_l        & 0xff;
 lenbytes[1] = (s->bitlen_l >>  8) & 0xff;
 lenbytes[2] = (s->bitlen_l >> 16) & 0xff;
 lenbytes[3] = (s->bitlen_l >> 24) & 0xff;
 lenbytes[4] =  s->bitlen_h        & 0xff;
 lenbytes[5] = (s->bitlen_h >>  8) & 0xff;
 lenbytes[6] = (s->bitlen_h >> 16) & 0xff;
 lenbytes[7] = (s->bitlen_h >> 24) & 0xff;
 crunch_bytes(s,&lenbytes[0],8);
 result[ 0] =  s->A        & 0xff;
 result[ 1] = (s->A >>  8) & 0xff;
 result[ 2] = (s->A >> 16) & 0xff;
 result[ 3] = (s->A >> 24) & 0xff;
 result[ 4] =  s->B        & 0xff;
 result[ 5] = (s->B >>  8) & 0xff;
 result[ 6] = (s->B >> 16) & 0xff;
 result[ 7] = (s->B >> 24) & 0xff;
 result[ 8] =  s->C        & 0xff;
 result[ 9] = (s->C >>  8) & 0xff;
 result[10] = (s->C >> 16) & 0xff;
 result[11] = (s->C >> 24) & 0xff;
 result[12] =  s->D        & 0xff;
 result[13] = (s->D >>  8) & 0xff;
 result[14] = (s->D >> 16) & 0xff;
 result[15] = (s->D >> 24) & 0xff;
}

/* End MD5 stuff */

/* Base 64 conversion */

static const char base64[64] = "./1234567890qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM";

/*
 * Convert "from" to base 64 and store it in "to".  Returns a pointer
 *  to just after the last byte of "to" that was modified.  (This will
 *  always be to+ceil((nfb*4)/3).)
 */
static char *to64(unsigned char *from, char *to, unsigned int nfb)
{
 unsigned long int cvbuf;
 int cvbits;

 cvbuf = 0;
 cvbits = 0;
 while (1)
  { if (cvbits < 6)
     { if (nfb < 1)
	{ if (cvbits > 0) *to++ = base64[cvbuf];
	  break;
	}
       cvbuf |= (*from++ << cvbits);
       cvbits += 8;
       nfb --;
     }
    *to++ = base64[cvbuf&63];
    cvbuf >>= 6;
    cvbits -= 6;
  }
 return(to);
}

/* End base 64 conversion */

/*
 * Publicized interface: make a salt string.  This returns something
 *  suitable to hand to crypt()'s second argument, when hashing a
 *  new password to store somewhere.
 */
char *crypt_makesalt(void)
{
 static char saltbuf[256];
 unsigned char tmp[256];
 struct timeval tv;
 STATE s;

 gettimeofday(&tv,0);
 sprintf(&tmp[0],"%lu %lu %d %d",
	(unsigned long int)tv.tv_sec,
	(unsigned long int)tv.tv_usec,
	(int)getpid(),
	(int)getppid());
 md5_init(&s);
 md5_process_bytes(&s,&tmp[0],strlen(&tmp[0]));
 md5_result(&s,&tmp[0]);
 saltbuf[0] = ':';
 to64(&tmp[0],&saltbuf[1],16);
 saltbuf[23] = '\0';
 return(&saltbuf[0]);
}

/*
 * Encrypt a password.  key is the cleartext password; hash is either
 *  the hashed password, from wherever it was stored, or some salt string,
 *  such as one returned by crypt_makesalt().
 *
 * Returned string has the format "=%d.%d.%s%s", where the first number
 *  is the format version (currently 1) and the second is the number of
 *  rounds.  The first string is the salt string, the second the hash
 *  string; both strings will always be 22 bytes long.  If the hash
 *  argument appears to match this format, it is assumed to be a hashed
 *  password; otherwise, it is taken as an arbitrary string and is
 *  MD5-hashed down to a 22-byte salt string that's used.
 */
char *md5_crypt(key,hash)
const char *key;
const char *hash;
{
 static char rvbuf[256];
 char saltbuf[22];
 unsigned char md5res[16];
 char databuf[22];
 const char *data;
 int datalen;
 int keylen;
 STATE s;
 int v;
 int r;
 int i;
 char *hp;
 char *salt;

 salt = 0;
 v = 0; /* shut up -Wuninitialized */
 r = 0; /* shut up -Wuninitialized */
 if (hash[0] == '=')
  { v = strtol(hash+1,&hp,10);
    if ((v == VERSION_MD5) && (*hp == '.'))
     { r = strtol(hp+1,&hp,10);
       if ((r >= MIN_ROUNDS) && (r <= MAX_ROUNDS) && (*hp == '.'))
	{ if ((strlen(hp) == 45) && __crypt_tst64stringp(hp+1))
	   { salt = hp + 1;
	   }
	}
     }
  }
 if (! salt)
  { unsigned char rnd[16];
    md5_init(&s);
    md5_process_bytes(&s,hash,strlen(hash));
    md5_result(&s,&md5res[0]);
    to64(&md5res[0],&saltbuf[0],16);
    md5_init(&s);
    md5_process_bytes(&s,&md5res[0],16);
    md5_process_bytes(&s,hash,strlen(hash));
    md5_process_bytes(&s,&md5res[0],16);
    md5_process_bytes(&s,hash,strlen(hash));
    md5_process_bytes(&s,&md5res[0],16);
    md5_result(&s,&rnd[0]);
    for (i=15;i>0;i--) rnd[0] ^= rnd[i];
    salt = &saltbuf[0];
    v = DEF_VERSION;
    i = DEF_ROUNDS >> 6;
    while (i & (i+1)) i |= i >> 1;
    r = DEF_ROUNDS + (rnd[i] & i) - (i >> 1);
  }
 keylen = strlen(key);
 data = key;
 datalen = keylen;
 for (i=0;i<r;i++)
  { md5_init(&s);
    md5_process_bytes(&s,data,datalen);
    md5_process_bytes(&s,salt,22);
    md5_process_bytes(&s,data,datalen);
    md5_process_bytes(&s,key,keylen);
    md5_process_bytes(&s,data,datalen);
    md5_result(&s,&md5res[0]);
    to64(&md5res[0],&databuf[0],16);
    data = &databuf[0];
    datalen = 22;
  }
 sprintf(&rvbuf[0],"=%d.%d.%.22s%.22s",v,r,salt,data);
 return(&rvbuf[0]);
}
