#include "opt_ddb.h"

#if defined(DDB)

#include <ddb/db_command.h>
#include <ddb/db_interface.h>

#include <machine/db_machdep.h>

extern void rtc_init(void); /* rtc.c */
extern void poweroff(void); /* rtc.c */

static void db_poweroff_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
 rtc_init();
 poweroff();
}

struct db_command next68k_db_command_table[]
 = { { "poweroff", db_poweroff_cmd, 0, 0 },
     { 0 } };

void db_machine_init(void)
{
 db_machine_commands_install(next68k_db_command_table);
}

#endif /* DDB */
