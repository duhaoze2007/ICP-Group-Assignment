/* =====================================================================
 * SDAMS - reports.h
 * Owner: M1 - Du Haoze (generic report helpers used by M2 and M5)
 * ===================================================================== */

#ifndef REPORTS_H
#define REPORTS_H

#include "../include/common.h"

/* ---------------- shared table formatting ---------------- */
void print_table_border(int width);
void print_heading(const char *title);

/* ---------------- manager reports (called by manager.c) ---------------- */
void   generate_staff_report(void);        /* filter by role / name, count per role */
void   generate_revenue_report(void);      /* filter by month / year                */

/* ---------------- aggregation helpers ---------------- */
double calculate_total_revenue(void);                 /* fees only            */
double calculate_total_penalties(void);               /* late-cancel penalties */
int    count_payments_in_period(int month, int year); /* month/year, 0 = any   */
int    count_staff_by_role(const char *role_filter);  /* "all"/"admin"/"instructor" */

/* ---------------- facility officer report (called by facility_officer.c) */
void   generate_stock_report(void);        /* filter by equipment ID / name */

#endif /* REPORTS_H */
