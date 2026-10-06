/* =====================================================================
 * SDAMS - admin.c
 * Owner: M3 - Htoo Aung Htet (Administrative Control Lead)
 *
 * Ported from the previous (FitZone) project's admin.py, where the
 * administrator could add / delete trainers and members and view all
 * data.  Here the same add / delete / view pattern is applied to the
 * SDAMS entities the administrator owns:
 *
 *   add_trainer()    -> add_instructor()      add_member()  -> add_student()
 *   delete_trainer() -> delete_instructor()   delete_member() -> delete_student()
 *   view_all()       -> view_all_instructors() / view_all_students()
 *
 * Plus the new C requirements: search by ID / name / contact, update
 * details, toggle status, and view a student's payment records.
 * All file access goes through the file handler (no fopen here).
 * ===================================================================== */

#include "../include/admin.h"
#include "../include/auth.h"
#include "../include/utils.h"
#include "../include/reports.h"
#include "../include/file_handler.h"

#include <stdio.h>
#include <string.h>

/* Highest existing number + 1 for the given prefix and list. */
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

static void collect_instructor_ids(char (*ids)[ID_LEN]) {
    size_t i;
    for (i = 0; i < g_data.instructors.count; i++) {
        snprintf(ids[i], ID_LEN, "%s", g_data.instructors.items[i].id);
    }
}

static void collect_student_ids(char (*ids)[ID_LEN]) {
    size_t i;
    for (i = 0; i < g_data.students.count; i++) {
        snprintf(ids[i], ID_LEN, "%s", g_data.students.items[i].id);
    }
}

/* =====================================================================
 * 1. INSTRUCTOR MANAGEMENT
 * ===================================================================== */

int add_instructor(void) {
    char       id[ID_LEN];
    char       name[NAME_LEN];
    char       role[SHORT_LEN];
    char       contact[CONTACT_LEN];
    char       ids[MAX_RECORDS][ID_LEN];
    Instructor record;

    print_heading("ADD INSTRUCTOR");

    if (g_data.instructors.count >= MAX_RECORDS) {
        printf("ERROR: Instructor storage is full (%d records).\n", MAX_RECORDS);
        return 0;
    }

    collect_instructor_ids(ids);
    next_id_for("I", ids, g_data.instructors.count, id, sizeof id);
    printf("New instructor ID: %s\n", id);

    if (read_line("Name: ", name, sizeof name) != 0) return 0;
    trim_whitespace(name);
    if (!validate_non_empty(name)) { printf("ERROR: Name cannot be empty.\n"); return 0; }

    if (read_line("Speciality (e.g. Karate, Judo, Taekwondo, MMA): ", role, sizeof role) != 0) return 0;
    trim_whitespace(role);
    if (!validate_non_empty(role)) { printf("ERROR: Speciality cannot be empty.\n"); return 0; }

    if (read_line("Contact number (e.g. 012-3456789): ", contact, sizeof contact) != 0) return 0;
    trim_whitespace(contact);
    if (!validate_contact(contact)) {
        printf("ERROR: Contact must be 7-20 digits and may contain spaces, '-' or a leading '+'.\n");
        return 0;
    }

    memset(&record, 0, sizeof record);
    snprintf(record.id, sizeof record.id, "%s", id);
    snprintf(record.name, sizeof record.name, "%s", name);
    snprintf(record.role, sizeof record.role, "%s", role);
    snprintf(record.contact, sizeof record.contact, "%s", contact);
    record.status     = ST_ACTIVE;
    record.avg_rating = 0.0;

    g_data.instructors.items[g_data.instructors.count++] = record;
    if (save_instructors() != 0) {
        printf("ERROR: Could not write %s.\n", INSTRUCTOR_FILE);
        g_data.instructors.count--;
        return 0;
    }
    printf("Added instructor %s (%s, %s).\n", id, name, role);
    log_event("Add instructor %s (%s)", id, name);
    return 1;
}

void view_all_instructors(void) {
    size_t i;

    print_heading("ALL INSTRUCTORS");
    printf("%-6s %-24s %-14s %-16s %-12s %s\n", "ID", "NAME", "SPECIALITY",
           "CONTACT", "STATUS", "RATING");
    print_table_border(90);
    for (i = 0; i < g_data.instructors.count; i++) {
        const Instructor *ins = &g_data.instructors.items[i];
        printf("%-6s %-24s %-14s %-16s %-12s %.2f\n", ins->id, ins->name, ins->role,
               ins->contact, status_to_string(ins->status), ins->avg_rating);
    }
    if (g_data.instructors.count == 0) printf("(no instructors)\n");
    print_table_border(90);
    printf("Total: %d instructor(s)\n", (int)g_data.instructors.count);
    log_event("Admin viewed all instructors");
}

void search_instructor(void) {
    char   key[NAME_LEN];
    size_t i;
    int    found = 0;

    print_heading("SEARCH INSTRUCTOR");
    if (read_line("Enter ID, name or contact to search: ", key, sizeof key) != 0) return;
    trim_whitespace(key);
    if (!validate_non_empty(key)) { printf("ERROR: Search text cannot be empty.\n"); return; }

    printf("\n%-6s %-24s %-14s %-16s %-12s\n", "ID", "NAME", "SPECIALITY", "CONTACT", "STATUS");
    print_table_border(78);
    for (i = 0; i < g_data.instructors.count; i++) {
        const Instructor *ins = &g_data.instructors.items[i];
        if (contains_ci(ins->id, key) || contains_ci(ins->name, key) ||
            contains_ci(ins->contact, key)) {
            printf("%-6s %-24s %-14s %-16s %-12s\n", ins->id, ins->name, ins->role,
                   ins->contact, status_to_string(ins->status));
            found++;
        }
    }
    print_table_border(78);
    if (found > 0) printf("%d match(es) found.\n", found);
    else           printf("No matching instructor found.\n");
    log_event("Admin searched instructors for '%s' (%d match)", key, found);
}

int update_instructor(void) {
    char key[ID_LEN];
    char buf[NAME_LEN];
    int  index;

    print_heading("UPDATE INSTRUCTOR DETAILS");
    if (read_line("Instructor ID: ", key, sizeof key) != 0) return 0;
    trim_whitespace(key);

    index = find_instructor_index(key);
    if (index < 0) {
        printf("ERROR: Instructor '%s' does not exist.\n", key);
        return 0;
    }

    printf("Leave a field empty to keep the current value.\n");

    if (read_line("New name: ", buf, sizeof buf) != 0) return 0;
    trim_whitespace(buf);
    if (buf[0] != '\0') snprintf(g_data.instructors.items[index].name, NAME_LEN, "%s", buf);

    if (read_line("New speciality: ", buf, sizeof buf) != 0) return 0;
    trim_whitespace(buf);
    if (buf[0] != '\0') snprintf(g_data.instructors.items[index].role, SHORT_LEN, "%s", buf);

    if (read_line("New contact: ", buf, sizeof buf) != 0) return 0;
    trim_whitespace(buf);
    if (buf[0] != '\0') {
        if (!validate_contact(buf)) {
            printf("ERROR: Invalid contact format - details were not saved.\n");
            return 0;
        }
        snprintf(g_data.instructors.items[index].contact, CONTACT_LEN, "%s", buf);
    }

    if (save_instructors() != 0) {
        printf("ERROR: Could not write %s.\n", INSTRUCTOR_FILE);
        return 0;
    }
    printf("Instructor %s updated.\n", g_data.instructors.items[index].id);
    log_event("Update instructor %s details", g_data.instructors.items[index].id);
    return 1;
}

int update_instructor_status(void) {
    char key[ID_LEN];
    int  index, choice = 0;

    print_heading("UPDATE INSTRUCTOR STATUS");
    if (read_line("Instructor ID: ", key, sizeof key) != 0) return 0;
    trim_whitespace(key);

    index = find_instructor_index(key);
    if (index < 0) {
        printf("ERROR: Instructor '%s' does not exist.\n", key);
        return 0;
    }

    printf("Instructor %s (%s) is currently %s.\n", g_data.instructors.items[index].id,
           g_data.instructors.items[index].name,
           status_to_string(g_data.instructors.items[index].status));
    if (read_int("New status (1 = active, 0 = not active): ", 0, 1, &choice) != 0) return 0;

    g_data.instructors.items[index].status = (choice == 1) ? ST_ACTIVE : ST_INACTIVE;
    if (save_instructors() != 0) {
        printf("ERROR: Could not write %s.\n", INSTRUCTOR_FILE);
        return 0;
    }
    printf("Status of %s updated to %s.\n", g_data.instructors.items[index].id,
           status_to_string(g_data.instructors.items[index].status));
    log_event("Update instructor %s status to %s", g_data.instructors.items[index].id,
              status_to_string(g_data.instructors.items[index].status));
    return 1;
}

/* Deleting an instructor is only allowed when no active class still
 * belongs to them (referential integrity - a new SDAMS requirement). */
int delete_instructor(void) {
    char   key[ID_LEN];
    int    index;
    size_t i;

    print_heading("DELETE INSTRUCTOR");
    if (read_line("Instructor ID: ", key, sizeof key) != 0) return 0;
    trim_whitespace(key);

    index = find_instructor_index(key);
    if (index < 0) {
        printf("ERROR: Instructor '%s' does not exist.\n", key);
        return 0;
    }

    for (i = 0; i < g_data.classes.count; i++) {
        if (equals_ci(g_data.classes.items[i].instructor_id, key) &&
            g_data.classes.items[i].status == ST_ACTIVE) {
            printf("ERROR: %s still teaches active class %s. Deactivate the class first.\n",
                   key, g_data.classes.items[i].class_id);
            return 0;
        }
    }

    if (!confirm_action("Delete this instructor (ratings stay in history)?")) {
        printf("Cancelled.\n");
        return 0;
    }

    for (i = (size_t)index; i + 1 < g_data.instructors.count; i++) {
        g_data.instructors.items[i] = g_data.instructors.items[i + 1];
    }
    g_data.instructors.count--;
    if (save_instructors() != 0) {
        printf("ERROR: Could not write %s.\n", INSTRUCTOR_FILE);
        return 0;
    }
    printf("Deleted instructor %s.\n", key);
    log_event("Delete instructor %s", key);
    return 1;
}

/* =====================================================================
 * 2. STUDENT MANAGEMENT
 * ===================================================================== */

int add_student(void) {
    char    id[ID_LEN];
    char    name[NAME_LEN];
    char    contact[CONTACT_LEN];
    char    ids[MAX_RECORDS][ID_LEN];
    Student record;

    print_heading("ADD STUDENT");

    if (g_data.students.count >= MAX_RECORDS) {
        printf("ERROR: Student storage is full (%d records).\n", MAX_RECORDS);
        return 0;
    }

    collect_student_ids(ids);
    next_id_for("S", ids, g_data.students.count, id, sizeof id);
    printf("New student ID: %s\n", id);

    if (read_line("Name: ", name, sizeof name) != 0) return 0;
    trim_whitespace(name);
    if (!validate_non_empty(name)) { printf("ERROR: Name cannot be empty.\n"); return 0; }

    if (read_line("Contact number (e.g. 012-3456789): ", contact, sizeof contact) != 0) return 0;
    trim_whitespace(contact);
    if (!validate_contact(contact)) {
        printf("ERROR: Contact must be 7-20 digits and may contain spaces, '-' or a leading '+'.\n");
        return 0;
    }

    memset(&record, 0, sizeof record);
    snprintf(record.id, sizeof record.id, "%s", id);
    snprintf(record.name, sizeof record.name, "%s", name);
    snprintf(record.contact, sizeof record.contact, "%s", contact);
    record.status = ST_ACTIVE;

    g_data.students.items[g_data.students.count++] = record;
    if (save_students() != 0) {
        printf("ERROR: Could not write %s.\n", STUDENT_FILE);
        g_data.students.count--;
        return 0;
    }
    printf("Added student %s (%s). Default password: %s\n", id, name, DEFAULT_PASSWORD);
    log_event("Add student %s (%s)", id, name);
    return 1;
}

void view_all_students(void) {
    size_t i;

    print_heading("ALL STUDENTS");
    printf("%-6s %-24s %-18s %s\n", "ID", "NAME", "CONTACT", "STATUS");
    print_table_border(64);
    for (i = 0; i < g_data.students.count; i++) {
        const Student *s = &g_data.students.items[i];
        printf("%-6s %-24s %-18s %s\n", s->id, s->name, s->contact,
               status_to_string(s->status));
    }
    if (g_data.students.count == 0) printf("(no students)\n");
    print_table_border(64);
    printf("Total: %d student(s)\n", (int)g_data.students.count);
    log_event("Admin viewed all students");
}

void search_student(void) {
    char   key[NAME_LEN];
    size_t i;
    int    found = 0;

    print_heading("SEARCH STUDENT");
    if (read_line("Enter ID, name or contact to search: ", key, sizeof key) != 0) return;
    trim_whitespace(key);
    if (!validate_non_empty(key)) { printf("ERROR: Search text cannot be empty.\n"); return; }

    printf("\n%-6s %-24s %-18s %s\n", "ID", "NAME", "CONTACT", "STATUS");
    print_table_border(64);
    for (i = 0; i < g_data.students.count; i++) {
        const Student *s = &g_data.students.items[i];
        if (contains_ci(s->id, key) || contains_ci(s->name, key) ||
            contains_ci(s->contact, key)) {
            printf("%-6s %-24s %-18s %s\n", s->id, s->name, s->contact,
                   status_to_string(s->status));
            found++;
        }
    }
    print_table_border(64);
    if (found > 0) printf("%d match(es) found.\n", found);
    else           printf("No matching student found.\n");
    log_event("Admin searched students for '%s' (%d match)", key, found);
}

int update_student(void) {
    char key[ID_LEN];
    char buf[NAME_LEN];
    int  index;

    print_heading("UPDATE STUDENT DETAILS");
    if (read_line("Student ID: ", key, sizeof key) != 0) return 0;
    trim_whitespace(key);

    index = find_student_index(key);
    if (index < 0) {
        printf("ERROR: Student '%s' does not exist.\n", key);
        return 0;
    }

    printf("Leave a field empty to keep the current value.\n");

    if (read_line("New name: ", buf, sizeof buf) != 0) return 0;
    trim_whitespace(buf);
    if (buf[0] != '\0') snprintf(g_data.students.items[index].name, NAME_LEN, "%s", buf);

    if (read_line("New contact: ", buf, sizeof buf) != 0) return 0;
    trim_whitespace(buf);
    if (buf[0] != '\0') {
        if (!validate_contact(buf)) {
            printf("ERROR: Invalid contact format - details were not saved.\n");
            return 0;
        }
        snprintf(g_data.students.items[index].contact, CONTACT_LEN, "%s", buf);
    }

    if (save_students() != 0) {
        printf("ERROR: Could not write %s.\n", STUDENT_FILE);
        return 0;
    }
    printf("Student %s updated.\n", g_data.students.items[index].id);
    log_event("Update student %s details", g_data.students.items[index].id);
    return 1;
}

int update_student_status(void) {
    char key[ID_LEN];
    int  index, choice = 0;

    print_heading("UPDATE STUDENT STATUS");
    if (read_line("Student ID: ", key, sizeof key) != 0) return 0;
    trim_whitespace(key);

    index = find_student_index(key);
    if (index < 0) {
        printf("ERROR: Student '%s' does not exist.\n", key);
        return 0;
    }

    printf("Student %s (%s) is currently %s.\n", g_data.students.items[index].id,
           g_data.students.items[index].name,
           status_to_string(g_data.students.items[index].status));
    if (read_int("New status (1 = active, 0 = not active): ", 0, 1, &choice) != 0) return 0;

    g_data.students.items[index].status = (choice == 1) ? ST_ACTIVE : ST_INACTIVE;
    if (save_students() != 0) {
        printf("ERROR: Could not write %s.\n", STUDENT_FILE);
        return 0;
    }
    printf("Status of %s updated to %s.\n", g_data.students.items[index].id,
           status_to_string(g_data.students.items[index].status));
    log_event("Update student %s status to %s", g_data.students.items[index].id,
              status_to_string(g_data.students.items[index].status));
    return 1;
}

/* A student with unpaid penalties cannot be removed - the payment record
 * must stay for the revenue report. */
int delete_student(void) {
    char   key[ID_LEN];
    int    index;
    size_t i;

    print_heading("DELETE STUDENT");
    if (read_line("Student ID: ", key, sizeof key) != 0) return 0;
    trim_whitespace(key);

    index = find_student_index(key);
    if (index < 0) {
        printf("ERROR: Student '%s' does not exist.\n", key);
        return 0;
    }

    for (i = 0; i < g_data.payments.count; i++) {
        if (equals_ci(g_data.payments.items[i].student_id, key) &&
            g_data.payments.items[i].type == PAY_PENALTY) {
            printf("ERROR: %s has penalty record %s. Set the student to 'not active' instead.\n",
                   key, g_data.payments.items[i].payment_id);
            return 0;
        }
    }

    if (!confirm_action("Delete this student and all their bookings?")) {
        printf("Cancelled.\n");
        return 0;
    }

    /* Remove the student record. */
    for (i = (size_t)index; i + 1 < g_data.students.count; i++) {
        g_data.students.items[i] = g_data.students.items[i + 1];
    }
    g_data.students.count--;

    /* Compact the booking list (same filtering style as the Python
     * project's write_file([x for x in lines if ...])). */
    {
        size_t write = 0;
        for (i = 0; i < g_data.bookings.count; i++) {
            if (!equals_ci(g_data.bookings.items[i].student_id, key)) {
                g_data.bookings.items[write++] = g_data.bookings.items[i];
            }
        }
        g_data.bookings.count = write;
    }

    save_students();
    save_bookings();
    printf("Deleted student %s and their bookings.\n", key);
    log_event("Delete student %s", key);
    return 1;
}

/* =====================================================================
 * 3. PAYMENT RECORDS (read-only)
 * ===================================================================== */

void view_student_payments(void) {
    char   key[ID_LEN];
    size_t i;
    int    found = 0;
    double total = 0.0, penalties = 0.0;

    print_heading("STUDENT PAYMENT RECORDS");
    if (read_line("Student ID (Enter = all payments): ", key, sizeof key) != 0) return;
    trim_whitespace(key);

    printf("\n%-6s %-8s %-8s %-8s %10s %s\n", "PAYID", "STUDENT", "CLASS",
           "TYPE", "AMOUNT", "DATE");
    print_table_border(66);
    for (i = 0; i < g_data.payments.count; i++) {
        const Payment *p = &g_data.payments.items[i];
        if (key[0] != '\0' && !equals_ci(p->student_id, key)) continue;
        printf("%-6s %-8s %-8s %-8s %10.2f %s\n", p->payment_id, p->student_id,
               p->class_id, payment_type_to_string(p->type), p->amount, p->payment_date);
        found++;
        if (p->type == PAY_FEE) total += p->amount;
        else                    penalties += p->amount;
    }
    if (found == 0) printf("(no payment records)\n");
    print_table_border(66);
    printf("Records: %d   Fees: RM%.2f   Penalties: RM%.2f   Total: RM%.2f\n",
           found, total, penalties, total + penalties);
    log_event("Admin viewed payment records (filter=%s)", (key[0] != '\0') ? key : "all");
}

/* =====================================================================
 * 4. Sub-menus
 * ===================================================================== */

static void instructor_management_menu(void) {
    char choice;

    while (1) {
        printf("\n[Instructor Management]\n");
        printf("1. Add instructor\n");
        printf("2. View all instructors\n");
        printf("3. Search instructor (ID / name / contact)\n");
        printf("4. Update instructor details\n");
        printf("5. Update instructor status\n");
        printf("6. Delete instructor\n");
        printf("0. Back\n");

        if (read_choice("Enter choice: ", "1234560", &choice) != 0) return;

        switch (choice) {
            case '1': add_instructor(); pause_screen(); break;
            case '2': view_all_instructors(); pause_screen(); break;
            case '3': search_instructor(); pause_screen(); break;
            case '4': update_instructor(); pause_screen(); break;
            case '5': update_instructor_status(); pause_screen(); break;
            case '6': delete_instructor(); pause_screen(); break;
            case '0': return;
            default:  printf("Invalid option.\n");
        }
    }
}

static void student_management_menu(void) {
    char choice;

    while (1) {
        printf("\n[Student Management]\n");
        printf("1. Add student\n");
        printf("2. View all students\n");
        printf("3. Search student (ID / name / contact)\n");
        printf("4. Update student details\n");
        printf("5. Update student status\n");
        printf("6. Delete student\n");
        printf("0. Back\n");

        if (read_choice("Enter choice: ", "1234560", &choice) != 0) return;

        switch (choice) {
            case '1': add_student(); pause_screen(); break;
            case '2': view_all_students(); pause_screen(); break;
            case '3': search_student(); pause_screen(); break;
            case '4': update_student(); pause_screen(); break;
            case '5': update_student_status(); pause_screen(); break;
            case '6': delete_student(); pause_screen(); break;
            case '0': return;
            default:  printf("Invalid option.\n");
        }
    }
}

void admin_menu(void) {
    char choice;

    while (1) {
        printf("\n[Administrator Subsystem Menu] %s - %s\n", g_session.user_name, g_session.user_id);
        printf("1. Instructor management\n");
        printf("2. Student management\n");
        printf("3. View student payment records\n");
        printf("0. Logout and back\n");

        if (read_choice("Enter choice: ", "1230", &choice) != 0) return;

        switch (choice) {
            case '1': instructor_management_menu(); break;
            case '2': student_management_menu(); break;
            case '3': view_student_payments(); pause_screen(); break;
            case '0':
                auth_logout();
                printf("Logged out of the administrator subsystem.\n");
                return;
            default:
                printf("Invalid option.\n");
        }
    }
}
