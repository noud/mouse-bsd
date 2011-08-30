#include <string.h>

#include "internal.h"

int __crypt_tst64stringp(const void *sarg)
{
 const unsigned char *s;
 static unsigned char v[] = {   0,  0,  0,  0,  0,192,255,  3, /*  0.. 63*/
			      254,255,255,  7,254,255,255,  7, /* 64..127*/
			      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /*128..255*/
			      };

 for (s=sarg;*s;s++) if (! (v[*s/8] & (1<<(*s%8)))) return(0);
 return(1);
}

char *crypt(const char *key, const char *salt)
{
 int saltlen;

 saltlen = strlen(salt);
 if ( ( ((saltlen == 13) || (saltlen == 2)) &&
	__crypt_tst64stringp(salt) ) ||
      ( (salt[0] == '_') &&
	((saltlen == 20) || (saltlen == 9)) &&
	__crypt_tst64stringp(salt+1) ) )
  { return(des_crypt(key,salt));
  }
 return(md5_crypt(key,salt));
}
