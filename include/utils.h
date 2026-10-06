/* =====================================================================
 * SDAMS - utils.h
 * Owner: M1 - Du Haoze
 *
 * Generic helpers used by every module:
 *   - validated console input (numbers, choices, dates, amounts)
 *   - string helpers (trim / case-insensitive compare / search)
 *   - identifier generation  (S001, I001, C001, ...)
 *   - date & time helpers (validation, parsing, hour difference)
 *   - screen helpers (pause, progress bar) and the activity log
 *   - the security prompt ported from the previous project
 * ===================================================================== */

#ifndef UTILS_H
#define UTILS_H

#include "../include/common.h"
#include <time.h>        /* struct tm - needed by parse_datetime() */

/* ---------------- console input ---------------- */
int  read_line(const char *prompt, char *out, size_t size);   /* 0 ok, -1 EOF */
int  read_int(const char *prompt, int min, int max, int *out);
int  read_double(const char *prompt, double min, double max, double *out);
int  read_choice(const char *prompt, const char *allowed, char *out);
int  confirm_action(const char *prompt);
void pause_screen(void);
void show_progress_bar(void);

/* ---------------- string helpers ---------------- */
void  trim_newline(char *s);
char *trim_whitespace(char *s);
void  to_lower(char *s);
int   equals_ci(const char *a, const char *b);
int   contains_ci(const char *haystack, const char *needle);
int   is_empty(const char *s);
int   validate_non_empty(const char *s);

/* ---------------- field validators ---------------- */
int validate_date(const char *s);                 /* "YYYY-MM-DD"            */
int validate_datetime(const char *s);             /* "YYYY-MM-DD HH:MM"      */
int validate_id(const char *s, const char *prefix);
int validate_contact(const char *s);
int validate_amount(const char *s, double *out);

/* ---------------- identifiers ---------------- */
int  id_number(const char *id, const char *prefix);   /* "S007" -> 7, else 0 */
void make_id(const char *prefix, int number, char *out, size_t size);

/* ---------------- date & time ---------------- */
void   current_datetime(char *out, size_t size, int with_time);
int    parse_datetime(const char *s, struct tm *out);
double hours_between(const char *from, const char *to);   /* to - from */
int    is_future_datetime(const char *s);
void   datetime_add_days(const char *base, int days, char *out, size_t size);

/* ---------------- enum <-> text ---------------- */
const char  *role_name(Role role);
const char  *status_to_string(Status s);
Status       status_from_string(const char *s);
const char  *booking_status_to_string(BookingStatus s);
BookingStatus booking_status_from_string(const char *s);
const char  *payment_type_to_string(PaymentType t);
PaymentType  payment_type_from_string(const char *s);
const char  *issue_status_to_string(IssueStatus s);
IssueStatus  issue_status_from_string(const char *s);
const char  *attendance_status_to_string(AttendanceStatus s);
AttendanceStatus attendance_status_from_string(const char *s);

/* ---------------- activity log ---------------- */
void log_event(const char *fmt, ...);

/* ---------------- security (ported from the previous project) ------- */
int check_password(const char *expected, int attempts);

#endif /* UTILS_H */
