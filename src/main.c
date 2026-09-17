/* =====================================================================
 * SDAMS - main.c
 * Owner: M1 - Du Haoze (Lead Architect & Integrator)
 *
 * Program entry point.  Ported from the previous (FitZone) project's
 * main.py, which printed a start-up banner, ran a progress bar, logged
 * the initialisation steps, showed a role menu and called the subsystem
 * menu the user picked.
 *
 * In SDAMS the password prompt of each subsystem is replaced by a real
 * login (auth.c) and the data files are read into memory once at start
 * (load_all_data) and written back on exit (save_all_data).
 * ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/file_handler.h"
#include "../include/utils.h"
#include "../include/auth.h"
#include "../include/manager.h"
#include "../include/admin.h"
#include "../include/student.h"
#include "../include/instructor.h"
#include "../include/facility_officer.h"
#include "../include/reports.h"

#define VERSION "V1.0"

/* =====================================================================
 * 1. Start-up (port of initsystem() from the previous project)
 * ===================================================================== */

static void print_banner(void) {
    printf("\n");
    print_table_border(78);
    printf("  Self Defence Academy Management System (SDAMS)  %s\n", VERSION);
    printf("  CT018-3-1-ICP Group Assignment\n");
    print_table_border(78);
}

static void init_system(void) {
    printf("\nSystem initializing and starting is in progress......\n");
    show_progress_bar();
    log_event("System memory initialized");
    log_event("File Management Utilities initialized");
    log_event("Command Line Interface Utilities initialized");
    log_event("System Self-Check passed");
    log_event("System started Normal");
    printf("System started.\n");
}

/* =====================================================================
 * 2. File Resource Manager sub-menu (all raw file access in one place)
 * ===================================================================== */

static void view_file_menu(void) {
    char   fname[128];
    char   path[256];
    char **lines = NULL;
    size_t n = 0, i;

    if (read_line("Enter filename (relative to data/, e.g. students.txt): ", fname, sizeof fname) != 0) return;
    trim_whitespace(fname);
    if (fname[0] == '\0') {
        printf("ERROR: Filename cannot be empty.\n");
        return;
    }
    snprintf(path, sizeof path, "%s/%s", get_data_dir(), fname);

    if (read_all_lines(path, &lines, &n) != 0) {
        printf("ERROR: Could not read %s.\n", path);
        return;
    }
    printf("\nContents of %s (%d line(s)):\n", path, (int)n);
    for (i = 0; i < n; i++) printf("%4d: %s\n", (int)(i + 1), lines[i]);
    if (n == 0) printf("(empty)\n");
    free_lines(lines, n);
}

static void append_line_menu(void) {
    char fname[128];
    char line[LINE_BUF_SIZE];
    char path[256];

    if (read_line("Enter filename (relative to data/): ", fname, sizeof fname) != 0) return;
    trim_whitespace(fname);
    if (fname[0] == '\0') { printf("ERROR: Filename cannot be empty.\n"); return; }

    if (read_line("Enter the line to append: ", line, sizeof line) != 0) return;
    if (line[0] == '\0') { printf("ERROR: Line cannot be empty.\n"); return; }

    snprintf(path, sizeof path, "%s/%s", get_data_dir(), fname);
    if (append_line(path, line) != 0) {
        printf("ERROR: Could not append to %s.\n", path);
        return;
    }
    printf("Appended to %s. Reloading the database...\n", path);
    reload_all_data();
    log_event("File Resource Manager: appended a line to %s", fname);
}

static void overwrite_file_menu(void) {
    char  fname[128];
    char  path[256];
    char  buffer[LINE_BUF_SIZE];
    char **lines = NULL;
    size_t cap = 0, count = 0;

    if (read_line("Enter filename (relative to data/): ", fname, sizeof fname) != 0) return;
    trim_whitespace(fname);
    if (fname[0] == '\0') { printf("ERROR: Filename cannot be empty.\n"); return; }

    printf("Enter the new content, one line at a time.\n");
    printf("Finish with a single '.' on its own line.\n");

    for (;;) {
        if (read_line("> ", buffer, sizeof buffer) != 0) break;
        if (strcmp(buffer, ".") == 0) break;
        if (count >= cap) {
            size_t new_cap = (cap == 0) ? 8 : cap * 2;
            char **tmp = realloc(lines, new_cap * sizeof(char *));
            if (!tmp) {
                free_lines(lines, count);
                printf("ERROR: Out of memory.\n");
                return;
            }
            lines = tmp;
            cap = new_cap;
        }
        {
            size_t len = strlen(buffer);
            lines[count] = malloc(len + 1);
            if (!lines[count]) {
                free_lines(lines, count);
                printf("ERROR: Out of memory.\n");
                return;
            }
            memcpy(lines[count], buffer, len + 1);
        }
        count++;
    }

    snprintf(path, sizeof path, "%s/%s", get_data_dir(), fname);
    if (write_all_lines(path, lines, count) != 0) {
        printf("ERROR: Could not write %s.\n", path);
    } else {
        printf("Wrote %d line(s) to %s. Reloading the database...\n", (int)count, path);
        reload_all_data();
        log_event("File Resource Manager: overwrote %s (%d lines)", fname, (int)count);
    }
    free_lines(lines, count);
}

static void list_files_menu(void) {
    char **files = NULL;
    size_t n = 0, i;

    if (list_files_in_dir(get_data_dir(), &files, &n) != 0) {
        printf("ERROR: Could not list %s.\n", get_data_dir());
        return;
    }
    printf("\nFiles in %s/:\n", get_data_dir());
    for (i = 0; i < n; i++) printf("  %2d. %s\n", (int)(i + 1), files[i]);
    if (n == 0) printf("  (no files)\n");
    free_lines(files, n);
}

static void file_manager_menu(void) {
    char choice;

    while (1) {
        printf("\n--- File Resource Manager ---\n");
        printf("1. List data files\n");
        printf("2. View file contents\n");
        printf("3. Append a line to a file\n");
        printf("4. Overwrite a file\n");
        printf("5. Show record counts\n");
        printf("0. Back to main menu\n");

        if (read_choice("Choose: ", "123450", &choice) != 0) return;

        switch (choice) {
            case '1': list_files_menu(); pause_screen(); break;
            case '2': view_file_menu(); pause_screen(); break;
            case '3': append_line_menu(); pause_screen(); break;
            case '4': overwrite_file_menu(); pause_screen(); break;
            case '5':
                printf("\nIn-memory record counts:\n");
                printf("  admins           : %d\n", (int)g_data.admins.count);
                printf("  instructors      : %d\n", (int)g_data.instructors.count);
                printf("  students         : %d\n", (int)g_data.students.count);
                printf("  classes          : %d\n", (int)g_data.classes.count);
                printf("  bookings         : %d\n", (int)g_data.bookings.count);
                printf("  payments         : %d\n", (int)g_data.payments.count);
                printf("  ratings          : %d\n", (int)g_data.ratings.count);
                printf("  facility issues  : %d\n", (int)g_data.issues.count);
                printf("  equipment        : %d\n", (int)g_data.equipment.count);
                printf("  attendance       : %d\n", (int)g_data.attendance.count);
                pause_screen();
                break;
            case '0': return;
            default:  printf("Invalid option.\n");
        }
    }
}

/* =====================================================================
 * 3. Role routing
 * ===================================================================== */

static void route_role(Role role) {
    if (!auth_login(role)) {
        printf("Login failed - returning to the main menu.\n");
        return;
    }
    switch (role) {
        case ROLE_MANAGER:    manager_menu();            break;
        case ROLE_ADMIN:      admin_menu();              break;
        case ROLE_INSTRUCTOR: instructor_menu();         break;
        case ROLE_STUDENT:    student_menu();            break;
        case ROLE_FACILITY:   facility_officer_menu();   break;
        default:              printf("Unknown role.\n"); break;
    }
}

/* =====================================================================
 * 4. main()
 * ===================================================================== */

int main(int argc, char *argv[]) {
    char  choice;
    char  now[SHORT_LEN + 8];
    int   data_ready;

    (void)argc;
    print_banner();

    /* Locate the data folder BEFORE the first log entry, so the program also
     * works when the IDE starts it from its build directory. */
    data_ready = resolve_data_dir((argc > 0) ? argv[0] : NULL);
    printf("\nData directory: %s\n", get_data_dir());
    if (data_ready > 0) {
        printf("(no existing data folder was found, so an empty one was created)\n");
    } else if (data_ready < 0) {
        fprintf(stderr, "ERROR: no usable data folder was found or created.\n");
        fprintf(stderr, "Run the program from the project folder (the one containing data/).\n");
    }

    init_system();

    if (load_all_data() != 0) {
        fprintf(stderr, "Warning: some data files could not be read. Continuing with empty data.\n");
    }
    log_event("Data files loaded from %s", get_data_dir());

    while (1) {
        current_datetime(now, sizeof now, 1);
        printf("\nCurrent time: %s\n", now);
        printf("==== SDAMS %s Main Menu ====\n", VERSION);
        printf("1. Manager\n");
        printf("2. Administrator\n");
        printf("3. Instructor\n");
        printf("4. Student\n");
        printf("5. Facility Officer\n");
        printf("6. File Resource Manager\n");
        printf("7. Save & Exit\n");

        if (read_choice("Select: ", "1234567", &choice) != 0) {
            /* End of input (Ctrl-D): behave like Save & Exit. */
            printf("\nInput closed - saving and exiting.\n");
            save_all_data();
            log_event("System shutdown (input stream closed)");
            return 0;
        }

        switch (choice) {
            case '1': route_role(ROLE_MANAGER);    break;
            case '2': route_role(ROLE_ADMIN);      break;
            case '3': route_role(ROLE_INSTRUCTOR); break;
            case '4': route_role(ROLE_STUDENT);    break;
            case '5': route_role(ROLE_FACILITY);   break;
            case '6': file_manager_menu();         break;
            case '7':
                if (save_all_data() != 0) {
                    fprintf(stderr, "Warning: some data files could not be saved.\n");
                } else {
                    printf("\nAll data saved to file.\n");
                }
                log_event("System shutdown");
                printf("Thank you for using SDAMS %s. Goodbye!\n", VERSION);
                printf("System shutting down......\n");
                show_progress_bar();
                printf("System successfully shut down.\n");
                return 0;
            default:
                printf("Invalid option.\n");
        }
    }
}
