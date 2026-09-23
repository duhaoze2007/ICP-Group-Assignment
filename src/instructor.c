/* =====================================================================
 * SDAMS - instructor.c
 * Owner: M4 - Lhaksam Tiempey Geltsan (Operations & Training Lead)
 *
 * Ported from the previous (FitZone) project's trainer.py:
 *
 *   report()             -> view_my_classes() / view_attendance_report()
 *   record_attendance()  -> mark_attendance()
 *   (admin.py add_class + delete_class) -> add_class() / toggle_class_status()
 *
 * plus the SDAMS specific requirements: manage the class schedule
 * (add / view / search / update), report facility issues, and read the
 * ratings received from students.
 *
 * Every class and issue listed here is filtered by the instructor ID of
 * the logged-in session, so one instructor can never see or edit the
 * schedule of another (role boundary check required by AGENTS.md).
 * ===================================================================== */

#include "../include/instructor.h"
#include "../include/auth.h"
#include "../include/student.h"
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

static void collect_class_ids(char (*ids)[ID_LEN]) {
    size_t i;
    for (i = 0; i < g_data.classes.count; i++) snprintf(ids[i], ID_LEN, "%s", g_data.classes.items[i].class_id);
}

static void collect_issue_ids(char (*ids)[ID_LEN]) {
    size_t i;
    for (i = 0; i < g_data.issues.count; i++) snprintf(ids[i], ID_LEN, "%s", g_data.issues.items[i].issue_id);
}

/* Does this class belong to the logged-in instructor? */
static int owns_class(const ClassRecord *c) {
    return equals_ci(c->instructor_id, g_session.user_id);
}

/* =====================================================================
 * 1. Class schedule management
 * ===================================================================== */

int add_class(void) {
    char        martial_art[SHORT_LEN];
    char        datetime[DATETIME_LEN];
    int         capacity = 0;
    ClassRecord record;
    char        ids[MAX_RECORDS][ID_LEN];

    print_heading("ADD A CLASS");

    if (g_data.classes.count >= MAX_RECORDS) {
        printf("ERROR: Class storage is full (%d records).\n", MAX_RECORDS);
        return 0;
    }

    if (read_line("Martial art (e.g. Karate, Judo, Taekwondo, MMA): ", martial_art, sizeof martial_art) != 0) return 0;
    trim_whitespace(martial_art);
    if (!validate_non_empty(martial_art)) {
        printf("ERROR: Martial art cannot be empty.\n");
        return 0;
    }

    if (read_line("Date & time (YYYY-MM-DD HH:MM): ", datetime, sizeof datetime) != 0) return 0;
    trim_whitespace(datetime);
    if (!validate_datetime(datetime)) {
        printf("ERROR: Use the format YYYY-MM-DD HH:MM (example: 2026-10-05 18:00).\n");
        return 0;
    }
    if (!is_future_datetime(datetime)) {
        printf("ERROR: A new class must be scheduled in the future.\n");
        return 0;
    }

    if (read_int("Capacity (1-200): ", 1, MAX_CAPACITY, &capacity) != 0) return 0;

    collect_class_ids(ids);
    next_id_for("C", ids, g_data.classes.count, record.class_id, sizeof record.class_id);
    snprintf(record.instructor_id, sizeof record.instructor_id, "%s", g_session.user_id);
    snprintf(record.martial_art, sizeof record.martial_art, "%s", martial_art);
    snprintf(record.datetime, sizeof record.datetime, "%s", datetime);
    record.capacity     = capacity;
    record.booked_count = 0;
    record.status       = ST_ACTIVE;

    g_data.classes.items[g_data.classes.count++] = record;
    if (save_classes() != 0) {
        printf("ERROR: Could not write %s.\n", CLASS_FILE);
        g_data.classes.count--;
        return 0;
    }
    printf("Class %s (%s) added for %s, capacity %d.\n", record.class_id,
           record.martial_art, record.datetime, capacity);
    log_event("Instructor %s added class %s (%s)", g_session.user_id, record.class_id, datetime);
    return 1;
}

void view_my_classes(void) {
    size_t i;
    int    found = 0;

    print_heading("MY CLASS SCHEDULE");
    printf("%-6s %-14s %-18s %-9s %-9s %s\n", "ID", "MARTIAL ART", "DATE & TIME",
           "CAPACITY", "BOOKED", "STATUS");
    print_table_border(78);

    for (i = 0; i < g_data.classes.count; i++) {
        const ClassRecord *c = &g_data.classes.items[i];
        if (!owns_class(c)) continue;
        printf("%-6s %-14s %-18s %-9d %-9d %s\n", c->class_id, c->martial_art,
               c->datetime, c->capacity, c->booked_count, status_to_string(c->status));
        found++;
    }
    if (found == 0) printf("(you have no classes yet)\n");
    print_table_border(78);
    printf("Total: %d class(es)\n", found);
    log_event("Instructor %s viewed their class schedule", g_session.user_id);
}

void search_class(void) {
    char   key[NAME_LEN];
    size_t i;
    int    found = 0;

    print_heading("SEARCH MY CLASSES");
    if (read_line("Search by class ID, martial art or date (YYYY-MM-DD): ", key, sizeof key) != 0) return;
    trim_whitespace(key);
    if (!validate_non_empty(key)) { printf("ERROR: Search text cannot be empty.\n"); return; }

    printf("\n%-6s %-14s %-18s %-9s %-9s %s\n", "ID", "MARTIAL ART", "DATE & TIME",
           "CAPACITY", "BOOKED", "STATUS");
    print_table_border(78);
    for (i = 0; i < g_data.classes.count; i++) {
        const ClassRecord *c = &g_data.classes.items[i];
        if (!owns_class(c)) continue;
        if (contains_ci(c->class_id, key) || contains_ci(c->martial_art, key) ||
            contains_ci(c->datetime, key)) {
            printf("%-6s %-14s %-18s %-9d %-9d %s\n", c->class_id, c->martial_art,
                   c->datetime, c->capacity, c->booked_count, status_to_string(c->status));
            found++;
        }
    }
    print_table_border(78);
    if (found > 0) printf("%d match(es) found.\n", found);
    else           printf("No matching class found.\n");
    log_event("Instructor %s searched own classes for '%s'", g_session.user_id, key);
}

int update_class(void) {
    char class_id[ID_LEN];
    char buffer[DATETIME_LEN];
    int  index, capacity = 0;

    print_heading("UPDATE A CLASS");
    if (read_line("Class ID: ", class_id, sizeof class_id) != 0) return 0;
    trim_whitespace(class_id);

    index = find_class_index(class_id);
    if (index < 0) {
        printf("ERROR: Class '%s' does not exist.\n", class_id);
        return 0;
    }
    if (!owns_class(&g_data.classes.items[index])) {
        printf("ERROR: Class %s is not yours - you may only edit your own classes.\n", class_id);
        return 0;
    }

    printf("Leave a field empty to keep the current value.\n");

    if (read_line("New date & time (YYYY-MM-DD HH:MM): ", buffer, sizeof buffer) != 0) return 0;
    trim_whitespace(buffer);
    if (buffer[0] != '\0') {
        if (!validate_datetime(buffer)) {
            printf("ERROR: Invalid date format - nothing was changed.\n");
            return 0;
        }
        if (!is_future_datetime(buffer)) {
            printf("ERROR: The new date is in the past - nothing was changed.\n");
            return 0;
        }
        snprintf(g_data.classes.items[index].datetime, DATETIME_LEN, "%s", buffer);
    }

    if (read_line("New martial art: ", buffer, sizeof buffer) != 0) return 0;
    trim_whitespace(buffer);
    if (buffer[0] != '\0') {
        snprintf(g_data.classes.items[index].martial_art, SHORT_LEN, "%s", buffer);
    }

    if (read_int("New capacity (0 = keep, must be >= booked count): ", 0, MAX_CAPACITY, &capacity) != 0) return 0;
    if (capacity > 0) {
        if (capacity < g_data.classes.items[index].booked_count) {
            printf("ERROR: Capacity cannot be smaller than the %d booked seat(s).\n",
                   g_data.classes.items[index].booked_count);
            return 0;
        }
        g_data.classes.items[index].capacity = capacity;
    }

    if (save_classes() != 0) {
        printf("ERROR: Could not write %s.\n", CLASS_FILE);
        return 0;
    }
    printf("Class %s updated.\n", class_id);
    log_event("Instructor %s updated class %s", g_session.user_id, class_id);
    return 1;
}

int toggle_class_status(void) {
    char class_id[ID_LEN];
    int  index, choice = 0;

    print_heading("ACTIVATE / DEACTIVATE A CLASS");
    if (read_line("Class ID: ", class_id, sizeof class_id) != 0) return 0;
    trim_whitespace(class_id);

    index = find_class_index(class_id);
    if (index < 0) {
        printf("ERROR: Class '%s' does not exist.\n", class_id);
        return 0;
    }
    if (!owns_class(&g_data.classes.items[index])) {
        printf("ERROR: Class %s is not yours.\n", class_id);
        return 0;
    }

    printf("Class %s is currently %s.\n", class_id,
           status_to_string(g_data.classes.items[index].status));
    if (read_int("New status (1 = active, 0 = not active): ", 0, 1, &choice) != 0) return 0;

    g_data.classes.items[index].status = (choice == 1) ? ST_ACTIVE : ST_INACTIVE;
    if (save_classes() != 0) {
        printf("ERROR: Could not write %s.\n", CLASS_FILE);
        return 0;
    }
    printf("Class %s is now %s.\n", class_id, status_to_string(g_data.classes.items[index].status));
    log_event("Instructor %s set class %s to %s", g_session.user_id, class_id,
              status_to_string(g_data.classes.items[index].status));
    return 1;
}

/* =====================================================================
 * 2. Facility issues
 * ===================================================================== */

int submit_facility_issue(void) {
    char          location[TEXT_LEN];
    char          description[TEXT_LEN];
    FacilityIssue issue;
    char          ids[MAX_RECORDS][ID_LEN];

    print_heading("SUBMIT A FACILITY ISSUE");

    if (g_data.issues.count >= MAX_RECORDS) {
        printf("ERROR: Issue storage is full (%d records).\n", MAX_RECORDS);
        return 0;
    }

    if (read_line("Location (e.g. Dojo Hall A, Equipment Room): ", location, sizeof location) != 0) return 0;
    trim_whitespace(location);
    if (!validate_non_empty(location)) {
        printf("ERROR: Location cannot be empty.\n");
        return 0;
    }

    if (read_line("Description (e.g. damaged mats, broken punching bag): ", description, sizeof description) != 0) return 0;
    trim_whitespace(description);
    if (!validate_non_empty(description)) {
        printf("ERROR: Description cannot be empty.\n");
        return 0;
    }

    collect_issue_ids(ids);
    next_id_for("F", ids, g_data.issues.count, issue.issue_id, sizeof issue.issue_id);
    snprintf(issue.instructor_id, sizeof issue.instructor_id, "%s", g_session.user_id);
    snprintf(issue.location, sizeof issue.location, "%s", location);
    snprintf(issue.description, sizeof issue.description, "%s", description);
    issue.status = IS_PENDING;                       /* every new issue starts as "pending" */
    current_datetime(issue.reported_date, sizeof issue.reported_date, 0);

    g_data.issues.items[g_data.issues.count++] = issue;
    if (save_facility_issues() != 0) {
        printf("ERROR: Could not write %s.\n", ISSUE_FILE);
        g_data.issues.count--;
        return 0;
    }
    printf("Issue %s reported (%s, status: pending).\n", issue.issue_id, location);
    log_event("Instructor %s reported facility issue %s at %s", g_session.user_id,
              issue.issue_id, location);
    return 1;
}

void view_my_issues(void) {
    size_t i;
    int    found = 0, pending = 0;

    print_heading("MY FACILITY ISSUES");
    printf("%-6s %-18s %-30s %-10s %s\n", "ID", "LOCATION", "DESCRIPTION", "STATUS", "REPORTED");
    print_table_border(88);

    for (i = 0; i < g_data.issues.count; i++) {
        const FacilityIssue *issue = &g_data.issues.items[i];
        if (!equals_ci(issue->instructor_id, g_session.user_id)) continue;
        printf("%-6s %-18s %-30s %-10s %s\n", issue->issue_id, issue->location,
               issue->description, issue_status_to_string(issue->status),
               issue->reported_date);
        found++;
        if (issue->status == IS_PENDING) pending++;
    }
    if (found == 0) printf("(you have not reported any issue)\n");
    print_table_border(88);
    printf("Total: %d issue(s), %d still pending\n", found, pending);
    log_event("Instructor %s viewed their facility issues", g_session.user_id);
}

/* =====================================================================
 * 3. Ratings received
 * ===================================================================== */

void view_my_overall_rating(void) {
    size_t i;
    int    count = 0;
    double sum = 0.0, best = 0.0;

    print_heading("MY RATING SCORES");
    printf("%-6s %-8s %-6s %-18s %s\n", "RATID", "STUDENT", "SCORE", "DATE", "COMMENT");
    print_table_border(72);

    for (i = 0; i < g_data.ratings.count; i++) {
        const Rating *r = &g_data.ratings.items[i];
        if (!equals_ci(r->instructor_id, g_session.user_id)) continue;
        printf("%-6s %-8s %-6d %-18s %s\n", r->rating_id, r->student_id, r->score,
               r->date, r->comment);
        sum += r->score;
        count++;
    }
    if (count == 0) printf("(no ratings received yet)\n");
    print_table_border(72);

    best = (count > 0) ? sum / count : 0.0;
    printf("Instructor %s (%s)\n", g_session.user_name, g_session.user_id);
    printf("Ratings received: %d\n", count);
    printf("Average score   : %.2f / %d\n", best, MAX_RATING);
    if (count > 0) {
        if (best >= 4.5)      printf("Feedback        : Excellent\n");
        else if (best >= 3.5) printf("Feedback        : Good\n");
        else if (best >= 2.5) printf("Feedback        : Average\n");
        else                  printf("Feedback        : Needs improvement\n");
    }
    log_event("Instructor %s viewed their ratings (average %.2f)", g_session.user_id, best);
}

/* =====================================================================
 * 4. Extra feature: class attendance
 *    Port of trainer.py record_attendance() + report(), now tied to the
 *    list of students who actually booked the class.
 * ===================================================================== */

int mark_attendance(void) {
    char   class_id[ID_LEN];
    char   today[SHORT_LEN];
    char   answer[16];
    int    index;
    size_t i;
    int    marked = 0, present = 0;

    print_heading("MARK CLASS ATTENDANCE");
    view_my_classes();
    if (read_line("Class ID: ", class_id, sizeof class_id) != 0) return 0;
    trim_whitespace(class_id);

    index = find_class_index(class_id);
    if (index < 0) {
        printf("ERROR: Class '%s' does not exist.\n", class_id);
        return 0;
    }
    if (!owns_class(&g_data.classes.items[index])) {
        printf("ERROR: Class %s is not yours.\n", class_id);
        return 0;
    }

    current_datetime(today, sizeof today, 0);
    printf("\nClass %s (%s) on %s - capacity %d, booked %d\n", class_id,
           g_data.classes.items[index].martial_art, g_data.classes.items[index].datetime,
           g_data.classes.items[index].capacity, g_data.classes.items[index].booked_count);

    for (i = 0; i < g_data.bookings.count; i++) {
        const Booking *b = &g_data.bookings.items[i];
        int   student_index;
        Attendance record;
        int   duplicate_index = -1;
        size_t j;

        if (!equals_ci(b->class_id, class_id)) continue;
        if (b->status == BK_CANCELLED) continue;

        student_index = find_student_index(b->student_id);

        /* Skip a student already marked for this class today. */
        for (j = 0; j < g_data.attendance.count; j++) {
            if (equals_ci(g_data.attendance.items[j].class_id, class_id) &&
                equals_ci(g_data.attendance.items[j].student_id, b->student_id) &&
                equals_ci(g_data.attendance.items[j].date, today)) {
                duplicate_index = (int)j;
                break;
            }
        }
        if (duplicate_index >= 0) {
            printf("Booking %s: %s already marked today (skipped).\n", b->booking_id, b->student_id);
            continue;
        }

        printf("Booking %s -> %s (%s): present? ", b->booking_id, b->student_id,
               (student_index >= 0) ? g_data.students.items[student_index].name : "unknown");
        fflush(stdout);
        if (fgets(answer, sizeof answer, stdin) == NULL) {
            clearerr(stdin);
            break;
        }
        trim_whitespace(answer);
        to_lower(answer);

        snprintf(record.class_id, sizeof record.class_id, "%s", class_id);
        snprintf(record.student_id, sizeof record.student_id, "%s", b->student_id);
        snprintf(record.date, sizeof record.date, "%s", today);
        record.status = (answer[0] == 'y') ? AT_PRESENT : AT_ABSENT;

        if (g_data.attendance.count >= MAX_RECORDS) {
            printf("ERROR: Attendance storage is full.\n");
            break;
        }
        g_data.attendance.items[g_data.attendance.count++] = record;
        marked++;
        if (record.status == AT_PRESENT) present++;
    }

    if (marked == 0) {
        printf("Nothing to mark for class %s today.\n", class_id);
        return 0;
    }

    if (save_attendance() != 0) {
        printf("ERROR: Could not write %s.\n", ATTENDANCE_FILE);
        return 0;
    }
    printf("Attendance saved: %d record(s), %d present.\n", marked, present);
    log_event("Attendance marked for class %s (%d present / %d records)",
              class_id, present, marked);
    return 1;
}

void view_attendance_report(void) {
    char   class_id[ID_LEN];
    size_t i;
    int    present = 0, total = 0;

    print_heading("ATTENDANCE REPORT");
    if (read_line("Class ID (Enter = all my classes): ", class_id, sizeof class_id) != 0) return;
    trim_whitespace(class_id);

    printf("\n%-6s %-8s %-18s %s\n", "CLASS", "STUDENT", "DATE", "STATUS");
    print_table_border(48);

    for (i = 0; i < g_data.attendance.count; i++) {
        const Attendance *a = &g_data.attendance.items[i];
        int class_index;

        if (class_id[0] != '\0' && !equals_ci(a->class_id, class_id)) continue;
        class_index = find_class_index(a->class_id);
        if (class_index < 0) continue;
        if (!owns_class(&g_data.classes.items[class_index])) continue;  /* only my classes */

        printf("%-6s %-8s %-18s %s\n", a->class_id, a->student_id, a->date,
               attendance_status_to_string(a->status));
        total++;
        if (a->status == AT_PRESENT) present++;
    }
    if (total == 0) printf("(no attendance records)\n");
    print_table_border(48);
    printf("Records: %d   Present: %d   Absent: %d\n", total, present, total - present);
    if (total > 0) {
        printf("Attendance rate: %.1f%%\n", (double)present * 100.0 / (double)total);
    }
    log_event("Instructor %s viewed the attendance report", g_session.user_id);
}

/* =====================================================================
 * 5. Instructor sub-menu
 * ===================================================================== */

void instructor_menu(void) {
    char choice;

    while (1) {
        printf("\n[Instructor Subsystem Menu] %s - %s\n", g_session.user_name, g_session.user_id);
        printf("1. Add a class\n");
        printf("2. View my class schedule\n");
        printf("3. Search my classes\n");
        printf("4. Update a class\n");
        printf("5. Activate / deactivate a class\n");
        printf("6. Submit a facility issue\n");
        printf("7. View my facility issues\n");
        printf("8. View my rating scores\n");
        printf("9. Mark class attendance (extra)\n");
        printf("A. Attendance report (extra)\n");
        printf("0. Logout and back\n");

        if (read_choice("Enter choice: ", "123456789A0", &choice) != 0) return;

        switch (choice) {
            case '1': add_class(); pause_screen(); break;
            case '2': view_my_classes(); pause_screen(); break;
            case '3': search_class(); pause_screen(); break;
            case '4': update_class(); pause_screen(); break;
            case '5': toggle_class_status(); pause_screen(); break;
            case '6': submit_facility_issue(); pause_screen(); break;
            case '7': view_my_issues(); pause_screen(); break;
            case '8': view_my_overall_rating(); pause_screen(); break;
            case '9': mark_attendance(); pause_screen(); break;
            case 'A': view_attendance_report(); pause_screen(); break;
            case '0':
                auth_logout();
                printf("Logged out of the instructor subsystem.\n");
                return;
            default:
                printf("Invalid option.\n");
        }
    }
}
