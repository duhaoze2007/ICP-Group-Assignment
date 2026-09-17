/* =====================================================================
 * SDAMS - facility_officer.c
 * Owner: M5 - Rehan Ali (Facilities & Logistics Lead)
 *
 * The previous (FitZone) project had no facilities role, so this module
 * reuses the same CRUD + filtering pattern that admin.py used for
 * trainers / members and adds the two reports required by the brief:
 * a stock report (low-stock warnings below the threshold) and the
 * facility issue workflow (pending -> fixed).
 *
 * All file access goes through the file handler.
 * ===================================================================== */

#include "../include/facility_officer.h"
#include "../include/auth.h"
#include "../include/utils.h"
#include "../include/reports.h"
#include "../include/file_handler.h"

#include <stdio.h>
#include <string.h>

static void next_id_for(const char *prefix, char (*ids)[ID_LEN], size_t count,
                        char *out, size_t size) {
    size_t i;
    int    best = 0;

    for (i = 0; i < count; i++) {
        int n = id_number(ids[i], prefix);
        if (n > best) best = n;
    }
    make_id(prefix, best + 1, out, size);
}

static void collect_equipment_ids(char (*ids)[ID_LEN]) {
    size_t i;
    for (i = 0; i < g_data.equipment.count; i++) snprintf(ids[i], ID_LEN, "%s", g_data.equipment.items[i].equip_id);
}

static int find_equipment_index(const char *equip_id) {
    size_t i;
    if (equip_id == NULL) return -1;
    for (i = 0; i < g_data.equipment.count; i++) {
        if (equals_ci(g_data.equipment.items[i].equip_id, equip_id)) return (int)i;
    }
    return -1;
}

static int find_issue_index(const char *issue_id) {
    size_t i;
    if (issue_id == NULL) return -1;
    for (i = 0; i < g_data.issues.count; i++) {
        if (equals_ci(g_data.issues.items[i].issue_id, issue_id)) return (int)i;
    }
    return -1;
}

/* =====================================================================
 * 1. Equipment inventory
 * ===================================================================== */

int add_equipment(void) {
    char      name[NAME_LEN];
    char      category[SHORT_LEN];
    int       quantity = 0, threshold = 0;
    Equipment record;
    char      ids[MAX_RECORDS][ID_LEN];

    print_heading("ADD EQUIPMENT");

    if (g_data.equipment.count >= MAX_RECORDS) {
        printf("ERROR: Equipment storage is full (%d records).\n", MAX_RECORDS);
        return 0;
    }

    if (read_line("Equipment name (e.g. Judo Mat, Punching Bag): ", name, sizeof name) != 0) return 0;
    trim_whitespace(name);
    if (!validate_non_empty(name)) { printf("ERROR: Name cannot be empty.\n"); return 0; }

    if (read_line("Category (e.g. Mats, Gloves, Training Gear): ", category, sizeof category) != 0) return 0;
    trim_whitespace(category);
    if (!validate_non_empty(category)) { printf("ERROR: Category cannot be empty.\n"); return 0; }

    if (read_int("Quantity in stock (0-10000): ", 0, MAX_EQUIP_QTY, &quantity) != 0) return 0;
    if (read_int("Low-stock threshold (0-10000): ", 0, MAX_EQUIP_QTY, &threshold) != 0) return 0;

    if (threshold > quantity) {
        printf("NOTE: The threshold is higher than the quantity, so this item is already low stock.\n");
    }

    collect_equipment_ids(ids);
    next_id_for("E", ids, g_data.equipment.count, record.equip_id, sizeof record.equip_id);
    snprintf(record.name, sizeof record.name, "%s", name);
    snprintf(record.category, sizeof record.category, "%s", category);
    record.quantity  = quantity;
    record.threshold = threshold;
    current_datetime(record.last_updated, sizeof record.last_updated, 1);

    g_data.equipment.items[g_data.equipment.count++] = record;
    if (save_equipment() != 0) {
        printf("ERROR: Could not write %s.\n", EQUIPMENT_FILE);
        g_data.equipment.count--;
        return 0;
    }
    printf("Added equipment %s (%s, quantity %d, threshold %d).\n", record.equip_id,
           name, quantity, threshold);
    log_event("Add equipment %s (%s) quantity %d", record.equip_id, name, quantity);
    return 1;
}

void view_all_equipment(void) {
    size_t i;
    int    low = 0;

    print_heading("EQUIPMENT INVENTORY");
    printf("%-6s %-24s %-14s %8s %10s %-18s %s\n", "ID", "NAME", "CATEGORY",
           "QUANTITY", "THRESHOLD", "LAST UPDATED", "STOCK");
    print_table_border(100);

    for (i = 0; i < g_data.equipment.count; i++) {
        const Equipment *e = &g_data.equipment.items[i];
        int is_low = (e->quantity <= e->threshold) ? 1 : 0;
        printf("%-6s %-24s %-14s %8d %10d %-18s %s\n", e->equip_id, e->name, e->category,
               e->quantity, e->threshold, e->last_updated,
               is_low ? "*** LOW ***" : "OK");
        if (is_low) low++;
    }
    if (g_data.equipment.count == 0) printf("(no equipment recorded)\n");
    print_table_border(100);
    printf("Total: %d item(s), %d low stock\n", (int)g_data.equipment.count, low);
    log_event("Facility officer viewed the equipment inventory");
}

void search_equipment(void) {
    char   key[NAME_LEN];
    size_t i;
    int    found = 0;

    print_heading("SEARCH EQUIPMENT");
    if (read_line("Search by ID, name or category: ", key, sizeof key) != 0) return;
    trim_whitespace(key);
    if (!validate_non_empty(key)) { printf("ERROR: Search text cannot be empty.\n"); return; }

    printf("\n%-6s %-24s %-14s %8s %10s %s\n", "ID", "NAME", "CATEGORY",
           "QUANTITY", "THRESHOLD", "STOCK");
    print_table_border(80);
    for (i = 0; i < g_data.equipment.count; i++) {
        const Equipment *e = &g_data.equipment.items[i];
        if (contains_ci(e->equip_id, key) || contains_ci(e->name, key) ||
            contains_ci(e->category, key)) {
            int is_low = (e->quantity <= e->threshold) ? 1 : 0;
            printf("%-6s %-24s %-14s %8d %10d %s\n", e->equip_id, e->name, e->category,
                   e->quantity, e->threshold, is_low ? "*** LOW ***" : "OK");
            found++;
        }
    }
    print_table_border(80);
    if (found > 0) printf("%d match(es) found.\n", found);
    else           printf("No matching equipment found.\n");
    log_event("Facility officer searched equipment for '%s' (%d match)", key, found);
}

int update_equipment(void) {
    char equip_id[ID_LEN];
    char buffer[NAME_LEN];
    int  index, threshold = 0, delta = 0;

    print_heading("UPDATE EQUIPMENT");
    if (read_line("Equipment ID: ", equip_id, sizeof equip_id) != 0) return 0;
    trim_whitespace(equip_id);

    index = find_equipment_index(equip_id);
    if (index < 0) {
        printf("ERROR: Equipment '%s' does not exist.\n", equip_id);
        return 0;
    }

    printf("Current: %s | %s | quantity %d | threshold %d\n",
           g_data.equipment.items[index].name, g_data.equipment.items[index].category,
           g_data.equipment.items[index].quantity, g_data.equipment.items[index].threshold);
    printf("How is the quantity changing? (0 = keep, positive = added, negative = used)\n");

    if (read_int("Quantity change (-10000 to 10000): ", -MAX_EQUIP_QTY, MAX_EQUIP_QTY, &delta) != 0) return 0;
    if (delta != 0) {
        int new_quantity = g_data.equipment.items[index].quantity + delta;
        if (new_quantity < 0 || new_quantity > MAX_EQUIP_QTY) {
            printf("ERROR: The resulting quantity would be out of range (%d).\n", new_quantity);
            return 0;
        }
        g_data.equipment.items[index].quantity = new_quantity;
    }

    if (read_int("New threshold (0 = keep): ", 0, MAX_EQUIP_QTY, &threshold) != 0) return 0;
    if (threshold > 0) g_data.equipment.items[index].threshold = threshold;

    if (read_line("New category (Enter = keep): ", buffer, sizeof buffer) != 0) return 0;
    trim_whitespace(buffer);
    if (buffer[0] != '\0') snprintf(g_data.equipment.items[index].category, SHORT_LEN, "%s", buffer);

    current_datetime(g_data.equipment.items[index].last_updated,
                     sizeof g_data.equipment.items[index].last_updated, 1);

    if (save_equipment() != 0) {
        printf("ERROR: Could not write %s.\n", EQUIPMENT_FILE);
        return 0;
    }
    printf("Equipment %s updated (quantity %d, threshold %d).\n", equip_id,
           g_data.equipment.items[index].quantity, g_data.equipment.items[index].threshold);
    if (g_data.equipment.items[index].quantity <= g_data.equipment.items[index].threshold) {
        printf("WARNING: %s is now below its threshold - restock needed.\n",
               g_data.equipment.items[index].name);
    }
    log_event("Update equipment %s (quantity %d threshold %d)", equip_id,
              g_data.equipment.items[index].quantity, g_data.equipment.items[index].threshold);
    return 1;
}

int delete_equipment(void) {
    char   equip_id[ID_LEN];
    int    index;
    size_t i;

    print_heading("DELETE EQUIPMENT");
    if (read_line("Equipment ID: ", equip_id, sizeof equip_id) != 0) return 0;
    trim_whitespace(equip_id);

    index = find_equipment_index(equip_id);
    if (index < 0) {
        printf("ERROR: Equipment '%s' does not exist.\n", equip_id);
        return 0;
    }
    if (g_data.equipment.items[index].quantity > 0) {
        printf("NOTE: %d unit(s) are still in stock.\n", g_data.equipment.items[index].quantity);
    }
    if (!confirm_action("Delete this equipment record?")) {
        printf("Cancelled.\n");
        return 0;
    }

    for (i = (size_t)index; i + 1 < g_data.equipment.count; i++) {
        g_data.equipment.items[i] = g_data.equipment.items[i + 1];
    }
    g_data.equipment.count--;
    if (save_equipment() != 0) {
        printf("ERROR: Could not write %s.\n", EQUIPMENT_FILE);
        return 0;
    }
    printf("Deleted equipment %s.\n", equip_id);
    log_event("Delete equipment %s", equip_id);
    return 1;
}

/* =====================================================================
 * 2. Facility issues
 * ===================================================================== */

void view_all_issues(void) {
    size_t i;
    int    pending = 0;

    print_heading("ALL FACILITY ISSUES");
    printf("%-6s %-6s %-18s %-30s %-9s %s\n", "ID", "BY", "LOCATION",
           "DESCRIPTION", "STATUS", "REPORTED");
    print_table_border(96);

    for (i = 0; i < g_data.issues.count; i++) {
        const FacilityIssue *issue = &g_data.issues.items[i];
        printf("%-6s %-6s %-18s %-30s %-9s %s\n", issue->issue_id, issue->instructor_id,
               issue->location, issue->description, issue_status_to_string(issue->status),
               issue->reported_date);
        if (issue->status == IS_PENDING) pending++;
    }
    if (g_data.issues.count == 0) printf("(no facility issues reported)\n");
    print_table_border(96);
    printf("Total: %d issue(s), %d pending\n", (int)g_data.issues.count, pending);
    log_event("Facility officer viewed all facility issues");
}

void search_issue(void) {
    char   key[NAME_LEN];
    size_t i;
    int    found = 0;

    print_heading("SEARCH FACILITY ISSUE");
    if (read_line("Search by issue ID, location or instructor ID: ", key, sizeof key) != 0) return;
    trim_whitespace(key);
    if (!validate_non_empty(key)) { printf("ERROR: Search text cannot be empty.\n"); return; }

    printf("\n%-6s %-6s %-18s %-30s %-9s %s\n", "ID", "BY", "LOCATION",
           "DESCRIPTION", "STATUS", "REPORTED");
    print_table_border(96);
    for (i = 0; i < g_data.issues.count; i++) {
        const FacilityIssue *issue = &g_data.issues.items[i];
        if (contains_ci(issue->issue_id, key) || contains_ci(issue->location, key) ||
            contains_ci(issue->instructor_id, key)) {
            printf("%-6s %-6s %-18s %-30s %-9s %s\n", issue->issue_id, issue->instructor_id,
                   issue->location, issue->description,
                   issue_status_to_string(issue->status), issue->reported_date);
            found++;
        }
    }
    print_table_border(96);
    if (found > 0) printf("%d match(es) found.\n", found);
    else           printf("No matching issue found.\n");
    log_event("Facility officer searched issues for '%s' (%d match)", key, found);
}

/* Status transition pending -> fixed.  An issue that is already fixed
 * cannot be closed twice. */
int update_issue_status(void) {
    char issue_id[ID_LEN];
    int  index, choice = 0;

    print_heading("UPDATE FACILITY ISSUE STATUS");
    if (read_line("Issue ID: ", issue_id, sizeof issue_id) != 0) return 0;
    trim_whitespace(issue_id);

    index = find_issue_index(issue_id);
    if (index < 0) {
        printf("ERROR: Issue '%s' does not exist.\n", issue_id);
        return 0;
    }
    if (g_data.issues.items[index].status == IS_FIXED) {
        printf("ERROR: Issue %s is already fixed.\n", issue_id);
        return 0;
    }

    printf("Issue %s at %s: %s\n", issue_id, g_data.issues.items[index].location,
           g_data.issues.items[index].description);
    if (read_int("Mark as (1 = fixed, 0 = still pending): ", 0, 1, &choice) != 0) return 0;

    if (choice == 0) {
        printf("Issue %s stays pending.\n", issue_id);
        return 0;
    }

    g_data.issues.items[index].status = IS_FIXED;
    if (save_facility_issues() != 0) {
        printf("ERROR: Could not write %s.\n", ISSUE_FILE);
        return 0;
    }
    printf("Issue %s marked as fixed.\n", issue_id);
    log_event("Update facility issue %s to fixed", issue_id);
    return 1;
}

/* Stock report is generated by reports.c (M1) and only triggered here. */
void facility_stock_report(void) {
    generate_stock_report();
}

/* =====================================================================
 * 3. Facility officer sub-menu
 * ===================================================================== */

void facility_officer_menu(void) {
    char choice;

    while (1) {
        printf("\n[Facility Officer Subsystem Menu] %s - %s\n", g_session.user_name,
               g_session.user_id);
        printf("1. Add equipment\n");
        printf("2. View all equipment\n");
        printf("3. Search equipment\n");
        printf("4. Update equipment (quantity / threshold)\n");
        printf("5. Delete equipment\n");
        printf("6. View all facility issues\n");
        printf("7. Search facility issue\n");
        printf("8. Update facility issue status\n");
        printf("9. Stock report (low-stock warnings)\n");
        printf("0. Logout and back\n");

        if (read_choice("Enter choice: ", "1234567890", &choice) != 0) return;

        switch (choice) {
            case '1': add_equipment(); pause_screen(); break;
            case '2': view_all_equipment(); pause_screen(); break;
            case '3': search_equipment(); pause_screen(); break;
            case '4': update_equipment(); pause_screen(); break;
            case '5': delete_equipment(); pause_screen(); break;
            case '6': view_all_issues(); pause_screen(); break;
            case '7': search_issue(); pause_screen(); break;
            case '8': update_issue_status(); pause_screen(); break;
            case '9': facility_stock_report(); pause_screen(); break;
            case '0':
                auth_logout();
                printf("Logged out of the facility officer subsystem.\n");
                return;
            default:
                printf("Invalid option.\n");
        }
    }
}
