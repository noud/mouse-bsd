#include <sys/device.h>

#define KMMUX_NRXRING 512
#define KMMUX_NTXRING 32

struct kmmux_mux;
struct zs_chanstate;

struct kmmux_softc {
  struct device dev;
  unsigned int swflags;
#define KMSF_CONSOLE 0x00000001
#define KMSF_ABORT_1 0x00000002
#define KMSF_TX_BUSY 0x00000004
#define KMSF_HOLDOWN 0x00000008
  unsigned int intflags;
#define KMIF_OVERRUN 0x00000001
#define KMIF_TXEMPTY 0x00000002
#define KMIF_STATCHG 0x00000004
  struct zs_chanstate *zcs;
  unsigned int baud;
  struct kmmux_mux *mux;
  unsigned int tput;
  unsigned int tget;
  unsigned char tring[KMMUX_NTXRING];
  unsigned int rput;
  unsigned int rget;
  unsigned short int rring[KMMUX_NRXRING];
#define KMMUX_RRINGINC(v) do { if (++v >= KMMUX_NRXRING) v = 0; } while (0)
#define KMMUX_TRINGINC(v) do { if (++v >= KMMUX_NTXRING) v = 0; } while (0)
  } ;

struct kmmux_attach_args {
  int console;
  } ;
