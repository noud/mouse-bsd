#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <net/if_ether.h>
#include <netinet/if_inarp.h>

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#include "interface.h"
#include "addrtoname.h"
#include "ethertype.h"
#include "extract.h"			/* XXX must come after interface.h */

#include "pppoe.h"

extern u_short extracted_ethertype; /* XXX belongs in a .h */

static void print_tag_data_bin(const u_char *bp, int len, int caplen)
{
 int i;

 for (i=0;(i<len)&&(i<caplen);i++) printf("%02x",bp[i]);
 printf(" ");
}

static void print_tag_data_txt(const u_char *bp, int len, int caplen)
{
 print_tag_data_bin(bp,len,caplen);
}

void pppoe_print(const u_char *bp, u_int length, u_int caplen)
{
 int code;
 int sid;
 int len;
 int destype;
 int o;
 int tagt;
 int tagl;

 if (caplen < 6)
  { printf("[|pppoe]");
    return;
  }
 printf("PPPoE ");
 if ( ((bp[0] & 0xf0) != (PPPOE_VER << 4)) ||
      ((bp[0] & 0x0f) != PPPOE_TYPE) )
  { printf("v%d t%d ",bp[0]>>4,bp[0]&0x0f);
    return;
  }
 code = bp[1];
 sid = (bp[2] << 8) + bp[3];
 len = (bp[4] << 8) + bp[5];
 destype = ETHERTYPE_PPPOEDISC;
 switch (code)
  { case PPPOE_CODE_PADI:
       printf("PADI ");
       break;
    case PPPOE_CODE_PADO:
       printf("PADO ");
       break;
    case PPPOE_CODE_PADR:
       printf("PADR ");
       break;
    case PPPOE_CODE_PADS:
       printf("PADS ");
       break;
    case PPPOE_CODE_PADT:
       printf("PADT ");
       break;
    case PPPOE_CODE_DATA:
       printf("DATA ");
       destype = ETHERTYPE_PPPOE;
       break;
    default:
       printf("code%d (sid %04x len %d) ",code,sid,len);
       return;
       break;
  }
 if (destype != extracted_ethertype) printf("(!)");
 if (sid) printf("sid %04x ",sid);
 if (len+6 < caplen) caplen = len + 6;
 switch (code)
  { case PPPOE_CODE_PADI:
    case PPPOE_CODE_PADO:
    case PPPOE_CODE_PADR:
    case PPPOE_CODE_PADS:
    case PPPOE_CODE_PADT:
       o = 6;
       while (1)
	{ if (o == caplen) break;
	  if (o+4 > caplen)
	   { printf("[|pppoe]");
	     break;
	   }
	  tagt = (bp[o] << 8) + bp[o+1];
	  tagl = (bp[o+2] << 8) + bp[o+3];
	  if (o+4+tagl > caplen)
	   { printf("[|pppoe]");
	     break;
	   }
	  switch (tagt)
	   { case PPPOE_TAG_END:
		printf("end ");
		print_tag_data_bin(bp+o+4,tagl,caplen-(o+4));
		break;
	     case PPPOE_TAG_SERV:
		printf("serv ");
		print_tag_data_txt(bp+o+4,tagl,caplen-(o+4));
		break;
	     case PPPOE_TAG_ACNAME:
		printf("acn ");
		print_tag_data_txt(bp+o+4,tagl,caplen-(o+4));
		break;
	     case PPPOE_TAG_HUNIQ:
		printf("uniq ");
		print_tag_data_bin(bp+o+4,tagl,caplen-(o+4));
		break;
	     case PPPOE_TAG_ACCOOK:
		printf("acck ");
		print_tag_data_bin(bp+o+4,tagl,caplen-(o+4));
		break;
	     case PPPOE_TAG_VENDSPEC:
		printf("vend ");
		print_tag_data_bin(bp+o+4,tagl,caplen-(o+4));
		break;
	     case PPPOE_TAG_RELAYID:
		printf("rly ");
		print_tag_data_bin(bp+o+4,tagl,caplen-(o+4));
		break;
	     case PPPOE_TAG_SNAMEERR:
		printf("snerr ");
		print_tag_data_txt(bp+o+4,tagl,caplen-(o+4));
		break;
	     case PPPOE_TAG_ACSYSERR:
		printf("acerr ");
		print_tag_data_txt(bp+o+4,tagl,caplen-(o+4));
		break;
	     case PPPOE_TAG_GENERR:
		printf("gerr ");
		print_tag_data_txt(bp+o+4,tagl,caplen-(o+4));
		break;
	     default:
		printf("%04x ",tagt);
		print_tag_data_bin(bp+o+4,tagl,caplen-(o+4));
		break;
	   }
	  o += 4 + tagl;
	}
       break;
    case PPPOE_CODE_DATA:
       ppp_contents_print(bp+8,len-2,caplen-8,(bp[6]<<8)+bp[7]);
       break;
  }
}
