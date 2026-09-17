/* =====================================================================
 * SDAMS - facility_officer.h
 * Owner: M5 - Rehan Ali (Facilities & Logistics Lead)
 * ===================================================================== */

#ifndef FACILITY_OFFICER_H
#define FACILITY_OFFICER_H

#include "../include/common.h"

/* Facility officer sub-menu (login is performed by main.c first). */
void facility_officer_menu(void);

/* Equipment inventory (equipment.txt) */
int  add_equipment(void);
void view_all_equipment(void);
void search_equipment(void);
int  update_equipment(void);
int  delete_equipment(void);

/* Facility issues reported by the instructors (facility_issues.txt) */
void view_all_issues(void);
void search_issue(void);
int  update_issue_status(void);

/* Stock report lives in reports.c (owned by M1) */
void facility_stock_report(void);

#endif /* FACILITY_OFFICER_H */
