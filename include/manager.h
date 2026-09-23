/* =====================================================================
 * SDAMS - manager.h
 * Owner: M2 - Justin Loo (Authentication & Management Lead)
 * ===================================================================== */

#ifndef MANAGER_H
#define MANAGER_H

#include "../include/common.h"

/* Manager sub-menu (login is performed by main.c before this is called). */
void manager_menu(void);

/* Staff account management (admins.txt) */
int  add_admin_account(void);
void view_all_admins(void);
void search_admin_by_name_or_id(void);
int  update_admin_status(void);
int  delete_admin_account(void);

/* Reports live in reports.c (owned by M1) and are only called from here. */
void manager_staff_report(void);
void manager_revenue_report(void);

#endif /* MANAGER_H */
