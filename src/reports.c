/* =====================================================================
 * SDAMS - reports.c
 * Owner: M1 - Du Haoze
 *
 * Aggregation helpers ported from the previous (FitZone) project:
 *   accountant.py  income_report()  -> calculate_total_revenue()
 *   accountant.py  unpaid_members() -> revenue/staff summaries
 *   admin.py       view_all()       -> table printing helpers
 * plus the new manager reports required by the assignment brief.
 * ===================================================================== */

#include "../include/reports.h"
#include "../include/utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* =====================================================================
 * 1. Table formatting helpers
 * ===================================================================== */

void print_table_border(int width) {
    int i;
    for (i = 0; i < width; i++) putchar('-');
    putchar('\n');
}

void print_heading(const char *title) {
    printf("\n");
    print_table_border(78);
    printf("%s\n", (title != NULL) ? title : "");
    print_table_border(78);
}

/* =====================================================================
 * 2. Aggregation helpers
 * ===================================================================== */

/* Sum of every class fee (penalties are excluded, exactly like the
 * previous project's income_report() which skipped malformed rows). */
double calculate_total_revenue(void) {
    size_t i;
    double total = 0.0;

    for (i = 0; i < g_data.payments.count; i++) {
        if (g_data.payments.items[i].type == PAY_FEE) {
            total += g_data.payments.items[i].amount;
        }
    }
    return total;
}

double calculate_total_penalties(void) {
    size_t i;
    double total = 0.0;

    for (i = 0; i < g_data.payments.count; i++) {
        if (g_data.payments.items[i].type == PAY_PENALTY) {
            total += g_data.payments.items[i].amount;
        }
    }
    return total;
}

/* month = 0 or year = 0 means "any month / any year". */
int count_payments_in_period(int month, int year) {
    size_t i;
    int    count = 0, y, m, d;

    for (i = 0; i < g_data.payments.count; i++) {
        const char *date = g_data.payments.items[i].payment_date;
        if (g_data.payments.items[i].type != PAY_FEE) continue;
        if (sscanf(date, "%d-%d-%d", &y, &m, &d) != 3) continue;
        if (month != 0 && m != month) continue;
        if (year != 0 && y != year) continue;
        count++;
    }
    return count;
}

int count_staff_by_role(const char *role_filter) {
    if (role_filter == NULL || equals_ci(role_filter, "all")) {
        return (int)(g_data.admins.count + g_data.instructors.count);
    }
    if (equals_ci(role_filter, "admin") || equals_ci(role_filter, "administrator")) {
        return (int)g_data.admins.count;
    }
    if (equals_ci(role_filter, "instructor")) {
        return (int)g_data.instructors.count;
    }
    return 0;
}

/* =====================================================================
 * 3. Manager reports
 * ===================================================================== */

/* Staff report: all staff accounts plus all instructors, optionally
 * filtered by group and by a name substring (case-insensitive). */
void generate_staff_report(void) {
    char  name_filter[NAME_LEN];
    char  group[TEXT_LEN];
    char  c;
    size_t i;
    int   shown_admins = 0, shown_instructors = 0;

    print_heading("STAFF REPORT");

    printf("Filter by group:\n");
    printf("  1. All staff\n");
    printf("  2. Manager / Administrator accounts only\n");
    printf("  3. Instructors only\n");
    if (read_choice("Select (1-3): ", "123", &c) != 0) return;

    if (c == '1')      snprintf(group, sizeof group, "all");
    else if (c == '2') snprintf(group, sizeof group, "admin");
    else               snprintf(group, sizeof group, "instructor");

    if (read_line("Filter by name (Enter = no filter): ", name_filter, sizeof name_filter) != 0) return;
    trim_whitespace(name_filter);

    if (c == '1' || c == '2') {
        printf("\n[Manager / Administrator accounts]\n");
        printf("%-6s %-24s %-12s\n", "ID", "NAME", "STATUS");
        print_table_border(46);
        for (i = 0; i < g_data.admins.count; i++) {
            const UserAccount *a = &g_data.admins.items[i];
            if (name_filter[0] != '\0' &&
                !contains_ci(a->name, name_filter) && !contains_ci(a->id, name_filter)) continue;
            printf("%-6s %-24s %-12s\n", a->id, a->name, status_to_string(a->status));
            shown_admins++;
        }
        if (shown_admins == 0) printf("(no matching account)\n");
    }

    if (c == '1' || c == '3') {
        printf("\n[Instructors]\n");
        printf("%-6s %-24s %-14s %-12s %s\n", "ID", "NAME", "SPECIALITY", "STATUS", "RATING");
        print_table_border(70);
        for (i = 0; i < g_data.instructors.count; i++) {
            const Instructor *ins = &g_data.instructors.items[i];
            if (name_filter[0] != '\0' &&
                !contains_ci(ins->name, name_filter) && !contains_ci(ins->id, name_filter)) continue;
            printf("%-6s %-24s %-14s %-12s %.2f\n", ins->id, ins->name, ins->role,
                   status_to_string(ins->status), ins->avg_rating);
            shown_instructors++;
        }
        if (shown_instructors == 0) printf("(no matching instructor)\n");
    }

    print_table_border(70);
    printf("Totals  : %d staff record(s) shown\n", shown_admins + shown_instructors);
    printf("By group: accounts=%d  instructors=%d  overall=%d\n",
           (int)g_data.admins.count, (int)g_data.instructors.count,
           count_staff_by_role("all"));
    log_event("Report: staff report generated (group=%s filter=%s)", group,
              (name_filter[0] != '\0') ? name_filter : "-");
}

/* Revenue report: month / year filter, number of registrations and the
 * total money collected in that period. */
void generate_revenue_report(void) {
    int    month = 0, year = 0;
    size_t i;
    int    registrations = 0, penalties = 0;
    double revenue = 0.0, penalty_total = 0.0;

    print_heading("REVENUE REPORT");

    printf("Period filter:\n");
    printf("  0 = every month / every year\n");
    if (read_int("Month (0-12): ", 0, 12, &month) != 0) return;
    if (read_int("Year (2015-2100): ", 2015, 2100, &year) != 0) return;

    printf("\n%-6s %-8s %-8s %-6s %10s %s\n", "ID", "STUDENT", "CLASS", "TYPE", "AMOUNT", "DATE");
    print_table_border(70);

    for (i = 0; i < g_data.payments.count; i++) {
        const Payment *p = &g_data.payments.items[i];
        int y = 0, m = 0, d = 0;

        if (sscanf(p->payment_date, "%d-%d-%d", &y, &m, &d) != 3) continue;
        if (month != 0 && m != month) continue;
        if (year != 0 && y != year) continue;

        printf("%-6s %-8s %-8s %-6s %10.2f %s\n", p->payment_id, p->student_id,
               p->class_id, payment_type_to_string(p->type), p->amount, p->payment_date);

        if (p->type == PAY_FEE) { revenue += p->amount; registrations++; }
        else                    { penalty_total += p->amount; penalties++; }
    }

    print_table_border(70);
    printf("Class registrations : %d\n", registrations);
    printf("Penalty records     : %d\n", penalties);
    printf("Total revenue (RM)  : %.2f\n", revenue);
    printf("Total penalties (RM): %.2f\n", penalty_total);
    printf("Grand total (RM)    : %.2f\n", revenue + penalty_total);
    printf("\nAll-time revenue (RM): %.2f\n", calculate_total_revenue());
    log_event("Report: revenue report generated (month=%d year=%d revenue=%.2f)",
              month, year, revenue);
}

/* =====================================================================
 * 4. Facility officer report
 * ===================================================================== */

/* Stock report: filter by equipment ID or name and highlight every item
 * whose quantity has dropped to (or below) its threshold. */
void generate_stock_report(void) {
    char   filter[NAME_LEN];
    size_t i;
    int    shown = 0, low = 0;

    print_heading("STOCK REPORT");

    if (read_line("Filter by equipment ID or name (Enter = all): ", filter, sizeof filter) != 0) return;
    trim_whitespace(filter);

    printf("\n%-6s %-24s %-14s %8s %10s %s\n", "ID", "NAME", "CATEGORY",
           "QUANTITY", "THRESHOLD", "STATUS");
    print_table_border(78);

    for (i = 0; i < g_data.equipment.count; i++) {
        const Equipment *e = &g_data.equipment.items[i];
        int is_low;

        if (filter[0] != '\0' &&
            !contains_ci(e->name, filter) && !contains_ci(e->equip_id, filter) &&
            !contains_ci(e->category, filter)) continue;

        is_low = (e->quantity <= e->threshold) ? 1 : 0;
        printf("%-6s %-24s %-14s %8d %10d %s\n", e->equip_id, e->name, e->category,
               e->quantity, e->threshold, is_low ? "*** LOW STOCK ***" : "OK");
        shown++;
        if (is_low) low++;
    }

    if (shown == 0) printf("(no matching equipment)\n");
    print_table_border(78);
    printf("Items shown : %d of %d\n", shown, (int)g_data.equipment.count);
    printf("Low stock   : %d item(s) need restocking\n", low);
    log_event("Report: stock report generated (filter=%s low=%d)",
              (filter[0] != '\0') ? filter : "all", low);
}
