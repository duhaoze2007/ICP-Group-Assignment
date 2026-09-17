/* =====================================================================
 * SDAMS - utils.c
 * Owner: M1 - Du Haoze
 *
 * Ported and extended from the previous (FitZone) project's
 * utils.py + cli_utils.py + file_utils.py helper layer:
 *   cli_utils.valid_input()        -> read_choice()
 *   cli_utils.pause()              -> pause_screen()
 *   cli_utils.show_progress_bar()  -> show_progress_bar()
 *   utils.log()                    -> log_event()
 *   utils.passwd()                 -> check_password()
 *   utils.generate_booking_id()    -> make_id() / id_number()
 *
 * This module never calls fopen(): the activity log is appended through
 * append_line() from the file handler, the single owner of file I/O.
 * ===================================================================== */

#include "../include/utils.h"
#include "../include/file_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>

/* =====================================================================
 * 1. String helpers
 * ===================================================================== */

/* Remove the trailing newline left behind by fgets(). */
void trim_newline(char *s) {
    size_t len;
    if (!s) return;
    len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

/* Remove the newline plus any leading / trailing blanks (in place). */
char *trim_whitespace(char *s) {
    char  *start;
    size_t len;

    if (!s) return s;
    trim_newline(s);
    start = s;
    while (*start != '\0' && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    return s;
}

void to_lower(char *s) {
    if (!s) return;
    for (; *s != '\0'; s++) *s = (char)tolower((unsigned char)*s);
}

/* Case-insensitive equality (portable replacement for strcasecmp). */
int equals_ci(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0') ? 1 : 0;
}

/* Case-insensitive "needle in haystack" search used by every search feature. */
int contains_ci(const char *haystack, const char *needle) {
    size_t nlen;

    if (!haystack || !needle) return 0;
    nlen = strlen(needle);
    if (nlen == 0) return 1;
    for (; *haystack != '\0'; haystack++) {
        size_t i;
        for (i = 0; i < nlen; i++) {
            if (haystack[i] == '\0' ||
                tolower((unsigned char)haystack[i]) != tolower((unsigned char)needle[i])) {
                break;
            }
        }
        if (i == nlen) return 1;
    }
    return 0;
}

int is_empty(const char *s) {
    if (!s) return 1;
    while (*s != '\0') {
        if (!isspace((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

int validate_non_empty(const char *s) {
    return is_empty(s) ? 0 : 1;
}

/* =====================================================================
 * 2. Validated console input
 *    Every prompt loops until the user types something acceptable, so an
 *    invalid entry can never reach the business logic.
 * ===================================================================== */

int read_line(const char *prompt, char *out, size_t size) {
    if (!out || size == 0) return -1;
    if (prompt) {
        fputs(prompt, stdout);
        fflush(stdout);
    }
    if (fgets(out, (int)size, stdin) == NULL) {
        out[0] = '\0';
        clearerr(stdin);
        return -1;               /* EOF - also how scripted test runs end */
    }
    trim_newline(out);
    return 0;
}

int read_int(const char *prompt, int min, int max, int *out) {
    char  buf[64];
    char *end;
    long  value;

    for (;;) {
        if (read_line(prompt, buf, sizeof buf) != 0) return -1;
        trim_whitespace(buf);
        if (buf[0] == '\0') {
            printf("ERROR: Input cannot be empty.\n");
            continue;
        }
        errno = 0;
        value = strtol(buf, &end, 10);
        if (errno == ERANGE || end == buf) {
            printf("ERROR: '%s' is not a whole number.\n", buf);
            continue;
        }
        while (*end == ' ' || *end == '\t') end++;
        if (*end != '\0') {
            printf("ERROR: '%s' is not a whole number.\n", buf);
            continue;
        }
        if (value < (long)min || value > (long)max) {
            printf("ERROR: Value must be between %d and %d.\n", min, max);
            continue;
        }
        *out = (int)value;
        return 0;
    }
}

int read_double(const char *prompt, double min, double max, double *out) {
    char   buf[64];
    char  *end;
    double value;

    for (;;) {
        if (read_line(prompt, buf, sizeof buf) != 0) return -1;
        trim_whitespace(buf);
        if (buf[0] == '\0') {
            printf("ERROR: Input cannot be empty.\n");
            continue;
        }
        errno = 0;
        value = strtod(buf, &end);
        if (errno == ERANGE || end == buf) {
            printf("ERROR: '%s' is not a number.\n", buf);
            continue;
        }
        while (*end == ' ' || *end == '\t') end++;
        if (*end != '\0') {
            printf("ERROR: '%s' is not a number.\n", buf);
            continue;
        }
        if (value < min || value > max) {
            printf("ERROR: Value must be between %.2f and %.2f.\n", min, max);
            continue;
        }
        *out = value;
        return 0;
    }
}

/* Port of cli_utils.valid_input(prompt, options): repeat until the typed
 * character is one of the characters listed in `allowed`. */
int read_choice(const char *prompt, const char *allowed, char *out) {
    char buf[64];

    for (;;) {
        if (read_line(prompt, buf, sizeof buf) != 0) return -1;
        trim_whitespace(buf);
        if (strlen(buf) == 1 && allowed != NULL && strchr(allowed, buf[0]) != NULL) {
            *out = buf[0];
            return 0;
        }
        if (buf[0] == '\0') printf("ERROR: (empty) is an invalid choice.\n");
        else                printf("ERROR: %s is an invalid choice.\n", buf);
    }
}

int confirm_action(const char *prompt) {
    char buf[16];

    printf("%s [y/n]: ", (prompt != NULL) ? prompt : "Confirm?");
    fflush(stdout);
    if (fgets(buf, sizeof buf, stdin) == NULL) {
        clearerr(stdin);
        return 0;
    }
    trim_whitespace(buf);
    to_lower(buf);
    return (strcmp(buf, "y") == 0 || strcmp(buf, "yes") == 0) ? 1 : 0;
}

/* Port of cli_utils.pause(). */
void pause_screen(void) {
    char buf[32];

    printf("\nPress Enter to continue...");
    fflush(stdout);
    if (fgets(buf, sizeof buf, stdin) == NULL) clearerr(stdin);
}

/* Busy wait so the progress bar animates without depending on any
 * OS specific sleep function - keeps this module portable ANSI C. */
static void short_delay(void) {
    clock_t start = clock();
    while ((clock() - start) < (CLOCKS_PER_SEC / 25)) {
        /* intentionally empty */
    }
}

/* Port of cli_utils.show_progress_bar(). */
void show_progress_bar(void) {
    const int total = 52;
    int       i, j, filled;
    double    percent;

    for (i = 0; i <= total; i++) {
        percent = (double)i * 100.0 / (double)total;
        filled  = (int)(percent / 100.0 * (double)total);
        printf("\r[");
        for (j = 0; j < total; j++) putchar(j < filled ? '#' : ' ');
        printf("] %5.1f%%", percent);
        fflush(stdout);
        short_delay();
    }
    putchar('\n');
}

/* =====================================================================
 * 3. Field validators
 * ===================================================================== */

/* "YYYY-MM-DD" including a real calendar check (month lengths + leap year). */
int validate_date(const char *s) {
    static const int days_in_month[12] = { 31, 28, 31, 30, 31, 30,
                                           31, 31, 30, 31, 30, 31 };
    int year, month, day, i, limit;

    if (!s || strlen(s) != 10) return 0;
    for (i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            if (s[i] != '-') return 0;
        } else if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }
    if (sscanf(s, "%d-%d-%d", &year, &month, &day) != 3) return 0;
    if (month < 1 || month > 12) return 0;
    limit = days_in_month[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) limit = 29;
    if (day < 1 || day > limit) return 0;
    return 1;
}

/* "YYYY-MM-DD HH:MM" */
int validate_datetime(const char *s) {
    char date_part[16];
    int  hour, minute, i;

    if (!s || strlen(s) != 16) return 0;
    if (s[10] != ' ' || s[13] != ':') return 0;
    memcpy(date_part, s, 10);
    date_part[10] = '\0';
    if (!validate_date(date_part)) return 0;
    for (i = 11; i < 16; i++) {
        if (i == 13) continue;
        if (!isdigit((unsigned char)s[i])) return 0;
    }
    if (sscanf(s + 11, "%d:%d", &hour, &minute) != 2) return 0;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return 0;
    return 1;
}

/* Identifier check: expected prefix followed by digits only ("S001"). */
int validate_id(const char *s, const char *prefix) {
    size_t      plen;
    const char *p;

    if (!s || !prefix) return 0;
    plen = strlen(prefix);
    if (strncmp(s, prefix, plen) != 0) return 0;
    if (strlen(s) <= plen) return 0;
    for (p = s + plen; *p != '\0'; p++) {
        if (!isdigit((unsigned char)*p)) return 0;
    }
    return 1;
}

/* Phone number: digits plus spaces / '-' and an optional leading '+'. */
int validate_contact(const char *s) {
    size_t len, i, digits = 0;

    if (!s) return 0;
    len = strlen(s);
    if (len < 7 || len > 20) return 0;
    for (i = 0; i < len; i++) {
        if (isdigit((unsigned char)s[i])) { digits++; continue; }
        if (s[i] == ' ' || s[i] == '-') continue;
        if (s[i] == '+' && i == 0) continue;
        return 0;
    }
    return (digits >= 7) ? 1 : 0;
}

/* Positive money amount. */
int validate_amount(const char *s, double *out) {
    char  *end;
    double value;

    if (!s || !out) return 0;
    errno = 0;
    value = strtod(s, &end);
    if (errno == ERANGE || end == s || *end != '\0') return 0;
    if (value <= 0.0 || value > MAX_PAYMENT) return 0;
    *out = value;
    return 1;
}

/* =====================================================================
 * 4. Identifier generation  (README: generateID() -> A001, S001, C001)
 * ===================================================================== */

int id_number(const char *id, const char *prefix) {
    size_t      plen;
    const char *digits;
    char       *end;
    long        value;

    if (!id || !prefix) return 0;
    plen = strlen(prefix);
    if (strncmp(id, prefix, plen) != 0) return 0;
    digits = id + plen;
    if (*digits == '\0') return 0;
    value = strtol(digits, &end, 10);
    if (end == digits || *end != '\0') return 0;
    if (value <= 0 || value > 99999) return 0;
    return (int)value;
}

void make_id(const char *prefix, int number, char *out, size_t size) {
    if (!out || size == 0) return;
    snprintf(out, size, "%s%03d", (prefix != NULL) ? prefix : "X",
             (number < 1) ? 1 : number);
}

/* =====================================================================
 * 5. Date & time helpers (strftime / strptime / timedelta equivalents)
 * ===================================================================== */

void current_datetime(char *out, size_t size, int with_time) {
    time_t     now;
    struct tm *tm_info;

    if (!out || size == 0) return;
    now     = time(NULL);
    tm_info = localtime(&now);
    if (tm_info == NULL) {
        out[0] = '\0';
        return;
    }
    strftime(out, size, with_time ? "%Y-%m-%d %H:%M" : "%Y-%m-%d", tm_info);
}

int parse_datetime(const char *s, struct tm *out) {
    int year = 0, month = 0, day = 0, hour = 0, minute = 0;

    if (!s || !out) return -1;
    if (!validate_datetime(s)) return -1;
    if (sscanf(s, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute) != 5) return -1;
    memset(out, 0, sizeof *out);
    out->tm_year  = year - 1900;
    out->tm_mon   = month - 1;
    out->tm_mday  = day;
    out->tm_hour  = hour;
    out->tm_min   = minute;
    out->tm_sec   = 0;
    out->tm_isdst = -1;
    if (mktime(out) == (time_t)-1) return -1;
    return 0;
}

/* Difference in hours (positive when `to` is later than `from`).
 * Used for the "cancel less than 5 hours before the class" penalty rule. */
double hours_between(const char *from, const char *to) {
    struct tm tm_from, tm_to;
    time_t    t_from, t_to;

    if (parse_datetime(from, &tm_from) != 0) return 0.0;
    if (parse_datetime(to, &tm_to) != 0) return 0.0;
    t_from = mktime(&tm_from);
    t_to   = mktime(&tm_to);
    if (t_from == (time_t)-1 || t_to == (time_t)-1) return 0.0;
    return difftime(t_to, t_from) / 3600.0;
}

int is_future_datetime(const char *s) {
    struct tm tm_value;
    time_t    then, now;

    if (parse_datetime(s, &tm_value) != 0) return 0;
    then = mktime(&tm_value);
    now  = time(NULL);
    if (then == (time_t)-1) return 0;
    return (difftime(then, now) > 0.0) ? 1 : 0;
}

/* Port of (datetime.now() + timedelta(days=n)).strftime("%Y-%m-%d"). */
void datetime_add_days(const char *base, int days, char *out, size_t size) {
    struct tm tm_value;
    time_t    stamp;

    if (!out || size == 0) return;
    if (parse_datetime(base, &tm_value) != 0) return;
    tm_value.tm_mday += days;
    tm_value.tm_isdst = -1;
    stamp = mktime(&tm_value);
    if (stamp == (time_t)-1) return;
    strftime(out, size, "%Y-%m-%d", localtime(&stamp));
}

/* =====================================================================
 * 6. Enum <-> text conversion (used when parsing / writing data files)
 * ===================================================================== */

const char *role_name(Role role) {
    switch (role) {
        case ROLE_MANAGER:    return "Manager";
        case ROLE_ADMIN:      return "Administrator";
        case ROLE_STUDENT:    return "Student";
        case ROLE_INSTRUCTOR: return "Instructor";
        case ROLE_FACILITY:   return "Facility Officer";
        default:              return "Unknown";
    }
}

const char *status_to_string(Status s) {
    return (s == ST_ACTIVE) ? "active" : "not active";
}

Status status_from_string(const char *s) {
    if (s != NULL && (equals_ci(s, "active") || equals_ci(s, "1"))) return ST_ACTIVE;
    return ST_INACTIVE;
}

const char *booking_status_to_string(BookingStatus s) {
    switch (s) {
        case BK_CANCELLED: return "cancelled";
        case BK_ATTENDED:  return "attended";
        default:           return "booked";
    }
}

BookingStatus booking_status_from_string(const char *s) {
    if (!s) return BK_BOOKED;
    if (equals_ci(s, "cancelled") || equals_ci(s, "canceled")) return BK_CANCELLED;
    if (equals_ci(s, "attended")) return BK_ATTENDED;
    return BK_BOOKED;
}

const char *payment_type_to_string(PaymentType t) {
    return (t == PAY_PENALTY) ? "penalty" : "payment";
}

PaymentType payment_type_from_string(const char *s) {
    return (s != NULL && equals_ci(s, "penalty")) ? PAY_PENALTY : PAY_FEE;
}

const char *issue_status_to_string(IssueStatus s) {
    return (s == IS_FIXED) ? "fixed" : "pending";
}

IssueStatus issue_status_from_string(const char *s) {
    return (s != NULL && (equals_ci(s, "fixed") || equals_ci(s, "resolved")))
           ? IS_FIXED : IS_PENDING;
}

const char *attendance_status_to_string(AttendanceStatus s) {
    return (s == AT_PRESENT) ? "present" : "absent";
}

AttendanceStatus attendance_status_from_string(const char *s) {
    return (s != NULL && equals_ci(s, "present")) ? AT_PRESENT : AT_ABSENT;
}

/* =====================================================================
 * 7. Activity log - port of utils.py log()
 *    Python: append_file("system.log", f"[{t}] {msg}")
 * ===================================================================== */

void log_event(const char *fmt, ...) {
    char      stamp[32];
    char      message[TEXT_LEN * 2];
    char      line[TEXT_LEN * 3];
    char      path[64];
    time_t    now;
    struct tm *tm_info;
    va_list   args;

    if (fmt == NULL) return;

    now     = time(NULL);
    tm_info = localtime(&now);
    if (tm_info != NULL) strftime(stamp, sizeof stamp, "%Y/%m/%d %H:%M:%S", tm_info);
    else                 snprintf(stamp, sizeof stamp, "unknown-time");

    va_start(args, fmt);
    vsnprintf(message, sizeof message, fmt, args);
    va_end(args);

    snprintf(line, sizeof line, "[%s] %s", stamp, message);
    snprintf(path, sizeof path, "%s/%s", DATA_DIR, LOG_FILE);

    if (append_line(path, line) != 0) {
        /* Logging must never abort the program - warn only. */
        fprintf(stderr, "Warning: could not write to %s\n", path);
    }
}

/* =====================================================================
 * 8. Security prompt - port of utils.py passwd(123456, 5)
 * ===================================================================== */

int check_password(const char *expected, int attempts) {
    char entered[PASS_LEN];

    if (expected == NULL) return 0;
    if (attempts <= 0) attempts = 1;

    printf("=== SDAMS Security System ===\n");
    while (attempts > 0) {
        if (read_line("Enter password: ", entered, sizeof entered) != 0) return 0;
        if (equals_ci(entered, expected)) {
            printf("Password correct.\n");
            return 1;
        }
        attempts--;
        if (attempts == 0) {
            printf("ERROR: Too many attempts.\n");
            return 0;
        }
        printf("ERROR: Incorrect password. %d attempt(s) left.\n", attempts);
    }
    return 0;
}
