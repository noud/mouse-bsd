#ifndef _PPPOE_H_09df6aa5_
#define _PPPOE_H_09df6aa5_

/* version constants */
#define PPPOE_VER 1
#define PPPOE_TYPE 1

/* code values */
#define PPPOE_CODE_PADI 0x09
#define PPPOE_CODE_PADO 0x07
#define PPPOE_CODE_PADR 0x19
#define PPPOE_CODE_PADS 0x65
#define PPPOE_CODE_PADT 0xa7
#define PPPOE_CODE_DATA 0x00

/* tag values */
#define PPPOE_TAG_END      0x0000 /* End-Of-List */
#define PPPOE_TAG_SERV     0x0101 /* Service-Name */
#define PPPOE_TAG_ACNAME   0x0102 /* AC-Name */
#define PPPOE_TAG_HUNIQ    0x0103 /* Host-Uniq */
#define PPPOE_TAG_ACCOOK   0x0104 /* AC-Cookie */
#define PPPOE_TAG_VENDSPEC 0x0105 /* Vendor-Specific */
#define PPPOE_TAG_RELAYID  0x0110 /* Relay-Session-Id */
#define PPPOE_TAG_SNAMEERR 0x0201 /* Service-Name-Error */
#define PPPOE_TAG_ACSYSERR 0x0202 /* AC-System-Error */
#define PPPOE_TAG_GENERR   0x0203 /* Generic-Error */

#endif
