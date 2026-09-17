/* =====================================================================
 * SDAMS - manager.c
 * Owner: M2 - Justin Loo (Authentication & Management Lead)
 *
 * Ported from the previous (FitZone) project's admin.py:
 *   admin.add_member()   -> add_admin_account()
 *   admin.delete_member() -> delete_admin_account()
 *   admin.view_all()     -> view_all_admins() + the two reports
 * The missing manager features of the previous project (staff report and
 * revenue report) are implemented through reports.c.
 *
 * No fopen() here: data lives in g_data and is written back with
 * save_admins() from the file handler.
 * ===================================================================== */

#include "../include/manager.h"
#include "../include/auth.h"
#include "../include/utils.h"
#include "../include/reports.h"
#include "../include/file_handler.h"

#include <stdio.h>
#include <string.h>

/* Highest existing account number + 1 (so IDs are never reused). */
static void next_account_id(char *out, size_t size) {
    size_t i;
    int    best = 0;

    for (i = 0; i < g_data.admins.count; i++) {
        int n = id_number(g_data.admins.items[i].id, "A");
        if (n > best) best = n;
    }
    make_id("A", best + 1, out, size);
}

/* ---------------------------------------------------------------------
 * 1. Add a staff account
 * ------------------------------------------------------------------- */
int add_admin_account(void) {
    char        id[ID_LEN];
    char        name[NAME_LEN];
    char        password[PASS_LEN];
    UserAccount account;

    print_heading("ADD STAFF ACCOUNT");

    if (g_data.admins.count >= MAX_RECORDS) {
        printf("ERROR: Account storage is full (%d records).\n", MAX_RECORDS);
        return 0;
    }

    next_account_id(id, sizeof id);
    printf("New account ID: %s\n", id);

    if (read_line("Name: ", name, sizeof name) != 0) return 0;
    trim_whitespace(name);
    if (!validate_non_empty(name)) {
        printf("ERROR: Name cannot be empty.\n");
        return 0;
    }

    if (read_line("Password (Enter = default 123456): ", password, sizeof password) != 0) return 0;
    trim_whitespace(password);
    if (password[0] == '\0') snprintf(password, sizeof password, "%s", DEFAULT_PASSWORD);
    if (strlen(password) < 4 || strlen(password) > 16) {
        printf("ERROR: Password must be 4 to 16 characters long.\n");
        return 0;
    }

    memset(&account, 0, sizeof account);
    snprintf(account.id, sizeof account.id, "%s", id);
    snprintf(account.name, sizeof account.name, "%s", name);
    snprintf(account.password, sizeof account.password, "%s", password);
    account.status = ST_ACTIVE;

    if (!confirm_action("Save this staff account?")) {
        printf("Cancelled.\n");
        return 0;
    }

    g_data.admins.items[g_data.admins.count++] = account;
    if (save_admins() != 0) {
        printf("ERROR: Could not write %s.\n", ADMIN_FILE);
        g_data.admins.count--;
        return 0;
    }
    printf("Added staff account %s (%s).\n", id, name);
    log_event("Add staff account %s (%s)", id, name);
    return 1;
}

/* ---------------------------------------------------------------------
 * 2. View every staff account
 * ------------------------------------------------------------------- */
void view_all_admins(void) {
    size_t i;

    print_heading("ALL STAFF ACCOUNTS");
    printf("%-6s %-24s %-14s %s\n", "ID", "NAME", "STATUS", "PASSWORD");
    print_table_border(70);
    for (i = 0; i < g_data.admins.count; i++) {
        const UserAccount *a = &g_data.admins.items[i];
        printf("%-6s %-24s %-14s %s\n", a->id, a->name, status_to_string(a->status), "******");
    }
    if (g_data.admins.count == 0) printf("(no staff accounts)\n");
    print_table_border(70);
    printf("Total: %d account(s)\n", (int)g_data.admins.count);
    printf("Note   : passwords are not displayed (new staff accounts default to %s).\n",
           DEFAULT_PASSWORD);
    log_event("Manager viewed all staff accounts");
}

/* ---------------------------------------------------------------------
 * 3. Search by name or ID (case-insensitive substring, like the new
 *    "search" requirement in the C brief; the Python version only
 *    compared full names).
 * ------------------------------------------------------------------- */
void search_admin_by_name_or_id(void) {
    char   key[NAME_LEN];
    size_t i;
    int    found = 0;

    print_heading("SEARCH STAFF ACCOUNT");
    if (read_line("Enter part of the name or the ID: ", key, sizeof key) != 0) return;
    trim_whitespace(key);
    if (!validate_non_empty(key)) {
        printf("ERROR: Search text cannot be empty.\n");
        return;
    }

    printf("\n%-6s %-24s %-14s\n", "ID", "NAME", "STATUS");
    print_table_border(46);
    for (i = 0; i < g_data.admins.count; i++) {
        const UserAccount *a = &g_data.admins.items[i];
        if (contains_ci(a->name, key) || contains_ci(a->id, key)) {
            printf("%-6s %-24s %-14s\n", a->id, a->name, status_to_string(a->status));
            found++;
        }
    }
    print_table_border(46);
    if (found > 0) printf("%d match(es) found.\n", found);
    else           printf("No matching account found.\n");
    log_event("Manager searched accounts for '%s' (%d match)", key, found);
}

/* ---------------------------------------------------------------------
 * 4. Toggle account status (active <-> not active)
 * ------------------------------------------------------------------- */
int update_admin_status(void) {
    char key[ID_LEN];
    int  index;
    int  choice = 0;

    print_heading("UPDATE STAFF ACCOUNT STATUS");
    if (read_line("Account ID: ", key, sizeof key) != 0) return 0;
    trim_whitespace(key);

    index = find_account_index(key);
    if (index < 0) {
        printf("ERROR: Account '%s' does not exist.\n", key);
        return 0;
    }
    if (strcmp(g_data.admins.items[index].id, g_session.user_id) == 0) {
        printf("ERROR: You cannot change the status of the account you are logged in with.\n");
        return 0;
    }

    printf("Account %s (%s) is currently %s.\n", g_data.admins.items[index].id,
           g_data.admins.items[index].name, status_to_string(g_data.admins.items[index].status));
    if (read_int("New status (1 = active, 0 = not active): ", 0, 1, &choice) != 0) return 0;

    g_data.admins.items[index].status = (choice == 1) ? ST_ACTIVE : ST_INACTIVE;
    if (save_admins() != 0) {
        printf("ERROR: Could not write %s.\n", ADMIN_FILE);
        return 0;
    }
    printf("Status of %s updated to %s.\n", g_data.admins.items[index].id,
           status_to_string(g_data.admins.items[index].status));
    log_event("Update staff account %s status to %s", g_data.admins.items[index].id,
              status_to_string(g_data.admins.items[index].status));
    return 1;
}

/* ---------------------------------------------------------------------
 * 5. Delete an account (port of admin.delete_member())
 * ------------------------------------------------------------------- */
int delete_admin_account(void) {
    char key[ID_LEN];
    int  index;
    char confirm[8];
    size_t i;

    print_heading("DELETE STAFF ACCOUNT");
    if (read_line("Account ID: ", key, sizeof key) != 0) return 0;
    trim_whitespace(key);

    index = find_account_index(key);
    if (index < 0) {
        printf("ERROR: Account '%s' does not exist.\n", key);
        return 0;
    }
    if (strcmp(g_data.admins.items[index].id, g_session.user_id) == 0) {
        printf("ERROR: You cannot delete the account you are logged in with.\n");
        return 0;
    }
    if (g_data.admins.count <= 1) {
        printf("ERROR: At least one staff account must remain in the system.\n");
        return 0;
    }

    printf("About to delete %s (%s). Type DELETE to confirm: ", g_data.admins.items[index].id,
           g_data.admins.items[index].name);
    fflush(stdout);
    if (fgets(confirm, sizeof confirm, stdin) == NULL) {
        clearerr(stdin);
        return 0;
    }
    trim_whitespace(confirm);
    if (strcmp(confirm, "DELETE") != 0) {
        printf("Cancelled - account not deleted.\n");
        return 0;
    }

    for (i = (size_t)index; i + 1 < g_data.admins.count; i++) {
        g_data.admins.items[i] = g_data.admins.items[i + 1];
    }
    g_data.admins.count--;
    if (save_admins() != 0) {
        printf("ERROR: Could not write %s.\n", ADMIN_FILE);
        return 0;
    }
    printf("Deleted staff account %s.\n", key);
    log_event("Delete staff account %s", key);
    return 1;
}

/* ---------------------------------------------------------------------
 * Reports - implemented in reports.c (M1) and called from here
 * ------------------------------------------------------------------- */
void manager_staff_report(void) {
    generate_staff_report();
    log_event("Manager generated the staff report");
}

void manager_revenue_report(void) {
    generate_revenue_report();
    log_event("Manager generated the revenue report");
}

/* ---------------------------------------------------------------------
 * Manager sub-menu
 * ------------------------------------------------------------------- */
void manager_menu(void) {
    char choice;

    while (1) {
        printf("\n[Manager Subsystem Menu] %s - %s\n", g_session.user_name, g_session.user_id);
        printf("1. Add staff account\n");
        printf("2. View all staff accounts\n");
        printf("3. Search staff account by name or ID\n");
        printf("4. Update staff account status\n");
        printf("5. Delete staff account\n");
        printf("6. Staff report\n");
        printf("7. Revenue report (month / year)\n");
        printf("0. Logout and back\n");

        if (read_choice("Enter choice: ", "12345670", &choice) != 0) return;

        switch (choice) {
            case '1':
                if (add_admin_account()) pause_screen();
                break;
            case '2': view_all_admins(); pause_screen(); break;
            case '3': search_admin_by_name_or_id(); pause_screen(); break;
            case '4': update_admin_status(); pause_screen(); break;
            case '5': delete_admin_account(); pause_screen(); break;
            case '6': manager_staff_report(); pause_screen(); break;
            case '7': manager_revenue_report(); pause_screen(); break;
            case '0':
                auth_logout();
                printf("Logged out of the manager subsystem.\n");
                return;
            default:
                printf("Invalid option.\n");
        }
    }
}
