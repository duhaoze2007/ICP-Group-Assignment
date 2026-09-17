/* =====================================================================
 * SDAMS - auth.c
 * Owner: M2 - Justin Loo (Authentication & Management Lead)
 *
 * Replaces the "type 123456 to enter the subsystem" prompt of the
 * previous project with a real login:
 *
 *   Manager / Administrator / Facility Officer -> account in admins.txt
 *   Student                                   -> record in students.txt
 *   Instructor                                -> record in instructors.txt
 *
 * Assumption (documented in the report): student and instructor accounts
 * use the default password 123456 and must have status "active".  Staff
 * accounts keep the password stored in admins.txt.
 * ===================================================================== */

#include "../include/auth.h"
#include "../include/utils.h"
#include "../include/file_handler.h"

#include <stdio.h>
#include <string.h>

Session g_session;   /* zero-initialised: nobody is logged in at start */

/* ---------------------------------------------------------------------
 * Lookup helpers (used here and by manager.c / admin.c / *_menu routing)
 * ------------------------------------------------------------------- */

int find_account_index(const char *id) {
    size_t i;
    if (id == NULL) return -1;
    for (i = 0; i < g_data.admins.count; i++) {
        if (equals_ci(g_data.admins.items[i].id, id)) return (int)i;
    }
    return -1;
}

int find_student_index(const char *id) {
    size_t i;
    if (id == NULL) return -1;
    for (i = 0; i < g_data.students.count; i++) {
        if (equals_ci(g_data.students.items[i].id, id)) return (int)i;
    }
    return -1;
}

int find_instructor_index(const char *id) {
    size_t i;
    if (id == NULL) return -1;
    for (i = 0; i < g_data.instructors.count; i++) {
        if (equals_ci(g_data.instructors.items[i].id, id)) return (int)i;
    }
    return -1;
}

/* ---------------------------------------------------------------------
 * Internal credential check.
 * Fills `name_out` with the display name on success.
 * Returns 1 on success, 0 on failure.  `*reason` receives a short text.
 * ------------------------------------------------------------------- */
static int verify_credentials(Role role, const char *id, const char *password,
                              char *name_out, size_t name_size,
                              const char **reason) {
    int index;

    *reason = "unknown reason";

    switch (role) {
        case ROLE_MANAGER:
        case ROLE_ADMIN:
        case ROLE_FACILITY: {
            index = find_account_index(id);
            if (index < 0) { *reason = "account not found"; return 0; }
            if (g_data.admins.items[index].status != ST_ACTIVE) {
                *reason = "account is not active";
                return 0;
            }
            if (strcmp(g_data.admins.items[index].password, password) != 0) {
                *reason = "wrong password";
                return 0;
            }
            snprintf(name_out, name_size, "%s", g_data.admins.items[index].name);
            return 1;
        }
        case ROLE_STUDENT: {
            index = find_student_index(id);
            if (index < 0) { *reason = "student ID not found"; return 0; }
            if (g_data.students.items[index].status != ST_ACTIVE) {
                *reason = "student is not active";
                return 0;
            }
            if (!equals_ci(password, DEFAULT_PASSWORD)) {
                *reason = "wrong password (default is 123456)";
                return 0;
            }
            snprintf(name_out, name_size, "%s", g_data.students.items[index].name);
            return 1;
        }
        case ROLE_INSTRUCTOR: {
            index = find_instructor_index(id);
            if (index < 0) { *reason = "instructor ID not found"; return 0; }
            if (g_data.instructors.items[index].status != ST_ACTIVE) {
                *reason = "instructor is not active";
                return 0;
            }
            if (!equals_ci(password, DEFAULT_PASSWORD)) {
                *reason = "wrong password (default is 123456)";
                return 0;
            }
            snprintf(name_out, name_size, "%s", g_data.instructors.items[index].name);
            return 1;
        }
        default:
            *reason = "unknown role";
            return 0;
    }
}

/* ---------------------------------------------------------------------
 * auth_login(): the public entry point used by main.c before routing to
 * a role sub-menu.
 * ------------------------------------------------------------------- */
int auth_login(Role role) {
    char        id[ID_LEN];
    char        password[PASS_LEN];
    char        name[NAME_LEN];
    const char *reason = "";
    int         attempts = MAX_LOGIN_ATTEMPTS;

    printf("\n=== SDAMS Security System ===\n");
    printf("Role: %s\n", role_name(role));

    while (attempts > 0) {
        if (read_line("Enter ID (0 to cancel): ", id, sizeof id) != 0) return 0;
        trim_whitespace(id);
        if (strcmp(id, "0") == 0) {
            printf("Login cancelled.\n");
            return 0;
        }
        if (read_line("Enter Password (0 to cancel): ", password, sizeof password) != 0) return 0;
        trim_whitespace(password);
        if (strcmp(password, "0") == 0) {
            printf("Login cancelled.\n");
            return 0;
        }

        if (verify_credentials(role, id, password, name, sizeof name, &reason)) {
            g_session.role = role;
            snprintf(g_session.user_id, sizeof g_session.user_id, "%s", id);
            snprintf(g_session.user_name, sizeof g_session.user_name, "%s", name);
            g_session.logged_in = 1;
            printf("\nLogin successful. Welcome, %s (%s).\n", name, id);
            log_event("Login OK: role=%s id=%s name=%s", role_name(role), id, name);
            return 1;
        }

        attempts--;
        printf("ERROR: Login failed - %s.\n", reason);
        if (attempts > 0) printf("%d attempt(s) left.\n", attempts);
    }

    log_event("Security System: %s login failed for ID %s", role_name(role), id);
    printf("ERROR: Too many failed attempts. Returning to the main menu.\n");
    return 0;
}

void auth_logout(void) {
    if (g_session.logged_in) {
        log_event("Logout: role=%s id=%s", role_name(g_session.role), g_session.user_id);
    }
    memset(&g_session, 0, sizeof g_session);
    g_session.role = ROLE_NONE;
}

int auth_is_logged_in(void) {
    return g_session.logged_in;
}

const Session *get_logged_in_user(void) {
    return &g_session;
}
