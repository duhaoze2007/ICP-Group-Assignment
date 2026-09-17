/* =====================================================================
 * SDAMS - file_handler.c   (FILE RESOURCE MANAGER)
 * Owner: M1 - Du Haoze
 *
 * THE ONLY MODULE IN THE PROJECT THAT CALLS fopen()/fprintf().
 * Ported from the previous (FitZone) project's file_utils.py, which
 * offered read_file() / write_file() / append_file().  Here those three
 * helpers become read_all_lines() / write_all_lines() / append_line(),
 * and a typed layer on top parses every record into a struct.
 *
 * Layout of this file:
 *   1. generic line layer        (fopen / fgets / fprintf)
 *   2. run-time line cache
 *   3. record parsing helpers    ("a|b|c"  ->  struct fields)
 *   4. load_xxx() / save_xxx()   (one pair per data file)
 *   5. load_all_data() / save_all_data()
 * ===================================================================== */

#include "../include/file_handler.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

/* The single in-memory database shared by every module (declared in common.h). */
DataStore g_data;

#define INITIAL_CAPACITY 32

/* =====================================================================
 * 0. Small internal helpers
 * ===================================================================== */

static void copy_field(char *dst, size_t size, const char *src) {
    size_t i = 0;
    if (size == 0) return;
    if (src == NULL) { dst[0] = '\0'; return; }
    while (src[i] != '\0' && i + 1 < size) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static char *dup_str(const char *src) {
    size_t len;
    char  *out;

    if (src == NULL) return NULL;
    len = strlen(src);
    out = malloc(len + 1);
    if (out == NULL) return NULL;
    memcpy(out, src, len + 1);
    return out;
}

/* Build "data/<filename>" in caller supplied buffer. */
static void data_path(char *out, size_t size, const char *filename) {
    snprintf(out, size, "%s/%s", DATA_DIR, filename);
}

static int ensure_array_capacity(char ***arr, size_t *cap, size_t needed) {
    size_t newcap;
    char **tmp;

    if (*cap >= needed) return 0;
    newcap = (*cap == 0) ? INITIAL_CAPACITY : (*cap * 2);
    while (newcap < needed) newcap *= 2;
    tmp = realloc(*arr, newcap * sizeof(char *));
    if (!tmp) return -1;
    *arr = tmp;
    *cap = newcap;
    return 0;
}

/* =====================================================================
 * 1. Generic line layer
 * ===================================================================== */

int read_all_lines(const char *path, char ***out_lines, size_t *out_count) {
    FILE  *f;
    char  *buf;
    char **arr = NULL;
    size_t cap = 0, cnt = 0;

    if (!path || !out_lines || !out_count) return -1;
    f = fopen(path, "r");
    if (!f) {
        *out_lines = NULL;   /* missing file = empty data (caller may create) */
        *out_count = 0;
        return 0;
    }
    buf = malloc(LINE_BUF_SIZE);
    if (!buf) { fclose(f); return -1; }

    while (fgets(buf, (int)LINE_BUF_SIZE, f)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';
        if (ensure_array_capacity(&arr, &cap, cnt + 1) != 0) {
            free(buf); fclose(f); free_lines(arr, cnt); return -1;
        }
        arr[cnt] = dup_str(buf);
        if (!arr[cnt]) { free(buf); fclose(f); free_lines(arr, cnt); return -1; }
        cnt++;
    }
    free(buf);
    fclose(f);
    *out_lines = arr;
    *out_count = cnt;
    return 0;
}

int write_all_lines(const char *path, char **lines, size_t count) {
    FILE  *f;
    size_t i;

    if (!path) return -1;
    f = fopen(path, "w");
    if (!f) return -1;
    for (i = 0; i < count; i++) {
        if (lines && lines[i]) fprintf(f, "%s\n", lines[i]);
        else                   fprintf(f, "\n");
    }
    if (fclose(f) != 0) return -1;
    return 0;
}

int append_line(const char *path, const char *line) {
    FILE *f;

    if (!path || !line) return -1;
    f = fopen(path, "a");
    if (!f) return -1;
    fprintf(f, "%s\n", line);
    if (fclose(f) != 0) return -1;
    return 0;
}

int list_files_in_dir(const char *dir, char ***out_files, size_t *out_count) {
    DIR           *d;
    struct dirent *ent;
    char         **arr = NULL;
    size_t         cap = 0, cnt = 0;

    if (!dir || !out_files || !out_count) return -1;
    d = opendir(dir);
    if (!d) {
        *out_files = NULL;
        *out_count = 0;
        return 0;
    }
    while ((ent = readdir(d)) != NULL) {
        char        pathbuf[4096];
        struct stat st;

        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        snprintf(pathbuf, sizeof pathbuf, "%s/%s", dir, ent->d_name);
        if (stat(pathbuf, &st) != 0) continue;
        if (!S_ISREG(st.st_mode)) continue;
        if (ensure_array_capacity(&arr, &cap, cnt + 1) != 0) {
            closedir(d); free_lines(arr, cnt); return -1;
        }
        arr[cnt] = dup_str(ent->d_name);
        if (!arr[cnt]) { closedir(d); free_lines(arr, cnt); return -1; }
        cnt++;
    }
    closedir(d);
    *out_files = arr;
    *out_count = cnt;
    return 0;
}

void free_lines(char **lines, size_t count) {
    size_t i;
    if (!lines) return;
    for (i = 0; i < count; i++) free(lines[i]);
    free(lines);
}

/* =====================================================================
 * 2. Run-time line cache (keeps main.c's File Resource Manager menu and
 *    the typed database consistent after every save).
 * ===================================================================== */

struct cache_entry {
    const char *name;
    char      **lines;
    size_t      count;
};

static const char *known_files[] = {
    ADMIN_FILE, INSTRUCTOR_FILE, STUDENT_FILE, CLASS_FILE, BOOKING_FILE,
    PAYMENT_FILE, RATING_FILE, ISSUE_FILE, EQUIPMENT_FILE, ATTENDANCE_FILE
};
static const size_t known_files_count = sizeof(known_files) / sizeof(known_files[0]);
static struct cache_entry cache[sizeof(known_files) / sizeof(known_files[0])];
static int  cache_initialized = 0;

static void cache_init(void) {
    size_t i;
    for (i = 0; i < known_files_count; i++) {
        cache[i].name  = known_files[i];
        cache[i].lines = NULL;
        cache[i].count = 0;
    }
    cache_initialized = 1;
}

int get_cached_lines(const char *filename, char ***out_lines, size_t *out_count) {
    size_t i;

    if (!cache_initialized || !filename || !out_lines || !out_count) return -1;
    for (i = 0; i < known_files_count; i++) {
        if (strcmp(cache[i].name, filename) == 0) {
            *out_lines = cache[i].lines;
            *out_count = cache[i].count;
            return 0;
        }
    }
    *out_lines = NULL;
    *out_count = 0;
    return -1;
}

int set_cached_lines(const char *filename, char **lines, size_t count) {
    size_t i;

    if (!cache_initialized || !filename) return -1;
    for (i = 0; i < known_files_count; i++) {
        if (strcmp(cache[i].name, filename) == 0) {
            free_lines(cache[i].lines, cache[i].count);
            cache[i].lines = lines;      /* ownership transferred to the cache */
            cache[i].count = count;
            return 0;
        }
    }
    return -1;
}

/* Write `lines` to data/<filename> and hand the array over to the cache. */
static int save_lines_to_file(const char *filename, char **lines, size_t count) {
    char path[64];
    int  rc;

    data_path(path, sizeof path, filename);
    rc = write_all_lines(path, lines, count);
    if (set_cached_lines(filename, lines, count) != 0) free_lines(lines, count);
    return rc;
}

/* =====================================================================
 * 3. Record parsing:  "field1|field2|field3"  ->  struct
 * =====================================================================
 * split_fields() copies each pipe separated field into the caller's
 * buffer.  Empty fields are allowed (""), extra fields are ignored.
 */
static int split_fields(const char *line, char fields[][TEXT_LEN], int max_fields) {
    const char *p = line;
    int         n = 0;

    if (!line || !fields || max_fields <= 0) return 0;
    while (n < max_fields) {
        size_t j = 0;
        while (*p != '\0' && *p != '|') {
            if (j + 1 < TEXT_LEN) fields[n][j++] = *p;
            p++;
        }
        fields[n][j] = '\0';
        n++;
        if (*p == '|') { p++; continue; }
        break;
    }
    return n;
}

/* A line is data only when it has content and is not a comment. */
static int is_data_line(const char *line) {
    return (line != NULL && line[0] != '\0' && line[0] != '#') ? 1 : 0;
}

static int parse_account(const char *line, UserAccount *a) {
    char f[4][TEXT_LEN];
    if (split_fields(line, f, 4) != 4) return -1;
    if (!validate_id(f[0], "A")) return -1;
    if (!validate_non_empty(f[1])) return -1;
    if (is_empty(f[2])) return -1;
    copy_field(a->id, sizeof a->id, f[0]);
    copy_field(a->name, sizeof a->name, f[1]);
    copy_field(a->password, sizeof a->password, f[2]);
    a->status = status_from_string(f[3]);
    return 0;
}

static int parse_instructor(const char *line, Instructor *i) {
    char f[6][TEXT_LEN];
    if (split_fields(line, f, 6) != 6) return -1;
    if (!validate_id(f[0], "I")) return -1;
    if (!validate_non_empty(f[1])) return -1;
    copy_field(i->id, sizeof i->id, f[0]);
    copy_field(i->name, sizeof i->name, f[1]);
    copy_field(i->role, sizeof i->role, f[2]);
    copy_field(i->contact, sizeof i->contact, f[3]);
    i->status     = status_from_string(f[4]);
    i->avg_rating = (f[5][0] != '\0') ? atof(f[5]) : 0.0;
    if (i->avg_rating < 0.0) i->avg_rating = 0.0;
    return 0;
}

static int parse_student(const char *line, Student *s) {
    char f[4][TEXT_LEN];
    if (split_fields(line, f, 4) != 4) return -1;
    if (!validate_id(f[0], "S")) return -1;
    if (!validate_non_empty(f[1])) return -1;
    copy_field(s->id, sizeof s->id, f[0]);
    copy_field(s->name, sizeof s->name, f[1]);
    copy_field(s->contact, sizeof s->contact, f[2]);
    s->status = status_from_string(f[3]);
    return 0;
}

static int parse_class(const char *line, ClassRecord *c) {
    char f[7][TEXT_LEN];
    if (split_fields(line, f, 7) != 7) return -1;
    if (!validate_id(f[0], "C")) return -1;
    if (!validate_id(f[1], "I")) return -1;
    if (!validate_non_empty(f[2])) return -1;
    if (!validate_datetime(f[3])) return -1;
    copy_field(c->class_id, sizeof c->class_id, f[0]);
    copy_field(c->instructor_id, sizeof c->instructor_id, f[1]);
    copy_field(c->martial_art, sizeof c->martial_art, f[2]);
    copy_field(c->datetime, sizeof c->datetime, f[3]);
    c->capacity     = atoi(f[4]);
    c->booked_count = atoi(f[5]);
    c->status       = status_from_string(f[6]);
    if (c->capacity <= 0 || c->capacity > MAX_CAPACITY) return -1;
    if (c->booked_count < 0 || c->booked_count > c->capacity) return -1;
    return 0;
}

static int parse_booking(const char *line, Booking *b) {
    char f[5][TEXT_LEN];
    if (split_fields(line, f, 5) != 5) return -1;
    if (!validate_id(f[0], "B")) return -1;
    if (!validate_id(f[1], "S")) return -1;
    if (!validate_id(f[2], "C")) return -1;
    if (!validate_datetime(f[3])) return -1;
    copy_field(b->booking_id, sizeof b->booking_id, f[0]);
    copy_field(b->student_id, sizeof b->student_id, f[1]);
    copy_field(b->class_id, sizeof b->class_id, f[2]);
    copy_field(b->booking_date, sizeof b->booking_date, f[3]);
    b->status = booking_status_from_string(f[4]);
    return 0;
}

static int parse_payment(const char *line, Payment *p) {
    char f[6][TEXT_LEN];
    if (split_fields(line, f, 6) != 6) return -1;
    if (!validate_id(f[0], "P")) return -1;
    if (!validate_id(f[1], "S")) return -1;
    if (!validate_non_empty(f[4])) return -1;
    copy_field(p->payment_id, sizeof p->payment_id, f[0]);
    copy_field(p->student_id, sizeof p->student_id, f[1]);
    copy_field(p->class_id, sizeof p->class_id, f[2]);
    p->amount = (f[3][0] != '\0') ? atof(f[3]) : 0.0;
    if (p->amount < 0.0 || p->amount > MAX_PAYMENT) return -1;
    copy_field(p->payment_date, sizeof p->payment_date, f[4]);
    p->type = payment_type_from_string(f[5]);
    return 0;
}

static int parse_rating(const char *line, Rating *r) {
    char f[6][TEXT_LEN];
    if (split_fields(line, f, 6) != 6) return -1;
    if (!validate_id(f[0], "R")) return -1;
    if (!validate_id(f[1], "S")) return -1;
    if (!validate_id(f[2], "I")) return -1;
    copy_field(r->rating_id, sizeof r->rating_id, f[0]);
    copy_field(r->student_id, sizeof r->student_id, f[1]);
    copy_field(r->instructor_id, sizeof r->instructor_id, f[2]);
    r->score = atoi(f[3]);
    if (r->score < 1 || r->score > MAX_RATING) return -1;
    copy_field(r->comment, sizeof r->comment, f[4]);
    copy_field(r->date, sizeof r->date, f[5]);
    return 0;
}

static int parse_issue(const char *line, FacilityIssue *i) {
    char f[6][TEXT_LEN];
    if (split_fields(line, f, 6) != 6) return -1;
    if (!validate_id(f[0], "F")) return -1;
    if (!validate_id(f[1], "I")) return -1;
    if (!validate_non_empty(f[2])) return -1;
    copy_field(i->issue_id, sizeof i->issue_id, f[0]);
    copy_field(i->instructor_id, sizeof i->instructor_id, f[1]);
    copy_field(i->location, sizeof i->location, f[2]);
    copy_field(i->description, sizeof i->description, f[3]);
    i->status = issue_status_from_string(f[4]);
    copy_field(i->reported_date, sizeof i->reported_date, f[5]);
    return 0;
}

static int parse_equipment(const char *line, Equipment *e) {
    char f[6][TEXT_LEN];
    if (split_fields(line, f, 6) != 6) return -1;
    if (!validate_id(f[0], "E")) return -1;
    if (!validate_non_empty(f[1])) return -1;
    copy_field(e->equip_id, sizeof e->equip_id, f[0]);
    copy_field(e->name, sizeof e->name, f[1]);
    copy_field(e->category, sizeof e->category, f[2]);
    e->quantity  = atoi(f[3]);
    e->threshold = atoi(f[4]);
    if (e->quantity < 0 || e->quantity > MAX_EQUIP_QTY) return -1;
    if (e->threshold < 0 || e->threshold > MAX_EQUIP_QTY) return -1;
    copy_field(e->last_updated, sizeof e->last_updated, f[5]);
    return 0;
}

static int parse_attendance(const char *line, Attendance *a) {
    char f[4][TEXT_LEN];
    if (split_fields(line, f, 4) != 4) return -1;
    if (!validate_id(f[0], "C")) return -1;
    if (!validate_id(f[1], "S")) return -1;
    copy_field(a->class_id, sizeof a->class_id, f[0]);
    copy_field(a->student_id, sizeof a->student_id, f[1]);
    copy_field(a->date, sizeof a->date, f[2]);
    a->status = attendance_status_from_string(f[3]);
    return 0;
}

/* =====================================================================
 * 4. load_xxx() / save_xxx() - one pair per data file
 * =====================================================================
 * The load functions below share one shape, so a single macro keeps them
 * short and identical in behaviour (parse, skip bad lines, mirror into
 * the line cache).
 */
#define LOAD_ENTITY(file_name, list_expr, parse_fn, type_name)                  \
    do {                                                                        \
        char  **lines = NULL;                                                   \
        size_t  n = 0, i;                                                       \
        char    path[64];                                                       \
        data_path(path, sizeof path, file_name);                                \
        if (read_all_lines(path, &lines, &n) != 0) return -1;                   \
        (list_expr).count = 0;                                                  \
        for (i = 0; i < n; i++) {                                               \
            type_name rec;                                                      \
            if (!is_data_line(lines[i])) continue;                              \
            if ((list_expr).count >= MAX_RECORDS) {                             \
                fprintf(stderr, "Warning: %s is full, extra records ignored\n", \
                        file_name);                                             \
                break;                                                          \
            }                                                                   \
            if (parse_fn(lines[i], &rec) == 0) {                                \
                (list_expr).items[(list_expr).count++] = rec;                   \
            } else {                                                            \
                fprintf(stderr, "Warning: skipped malformed record in %s: %s\n",\
                        file_name, lines[i]);                                   \
            }                                                                   \
        }                                                                       \
        if (set_cached_lines(file_name, lines, n) != 0) free_lines(lines, n);    \
        return 0;                                                               \
    } while (0)

#define SAVE_ENTITY(file_name, list_expr, format_fn)                            \
    do {                                                                        \
        char **lines;                                                           \
        size_t i, n = (list_expr).count;                                        \
        lines = malloc(sizeof(char *) * ((n == 0) ? 1 : n));                    \
        if (!lines) return -1;                                                  \
        for (i = 0; i < n; i++) {                                               \
            char buf[LINE_BUF_SIZE];                                            \
            format_fn(&(list_expr).items[i], buf, sizeof buf);                  \
            lines[i] = dup_str(buf);                                            \
            if (!lines[i]) { free_lines(lines, i); return -1; }                 \
        }                                                                       \
        return save_lines_to_file(file_name, lines, n);                         \
    } while (0)

static void fmt_account(const UserAccount *a, char *buf, size_t size) {
    snprintf(buf, size, "%s|%s|%s|%s", a->id, a->name, a->password,
             status_to_string(a->status));
}
static void fmt_instructor(const Instructor *i, char *buf, size_t size) {
    snprintf(buf, size, "%s|%s|%s|%s|%s|%.2f", i->id, i->name, i->role,
             i->contact, status_to_string(i->status), i->avg_rating);
}
static void fmt_student(const Student *s, char *buf, size_t size) {
    snprintf(buf, size, "%s|%s|%s|%s", s->id, s->name, s->contact,
             status_to_string(s->status));
}
static void fmt_class(const ClassRecord *c, char *buf, size_t size) {
    snprintf(buf, size, "%s|%s|%s|%s|%d|%d|%s", c->class_id, c->instructor_id,
             c->martial_art, c->datetime, c->capacity, c->booked_count,
             status_to_string(c->status));
}
static void fmt_booking(const Booking *b, char *buf, size_t size) {
    snprintf(buf, size, "%s|%s|%s|%s|%s", b->booking_id, b->student_id,
             b->class_id, b->booking_date, booking_status_to_string(b->status));
}
static void fmt_payment(const Payment *p, char *buf, size_t size) {
    snprintf(buf, size, "%s|%s|%s|%.2f|%s|%s", p->payment_id, p->student_id,
             p->class_id, p->amount, p->payment_date,
             payment_type_to_string(p->type));
}
static void fmt_rating(const Rating *r, char *buf, size_t size) {
    snprintf(buf, size, "%s|%s|%s|%d|%s|%s", r->rating_id, r->student_id,
             r->instructor_id, r->score, r->comment, r->date);
}
static void fmt_issue(const FacilityIssue *i, char *buf, size_t size) {
    snprintf(buf, size, "%s|%s|%s|%s|%s|%s", i->issue_id, i->instructor_id,
             i->location, i->description, issue_status_to_string(i->status),
             i->reported_date);
}
static void fmt_equipment(const Equipment *e, char *buf, size_t size) {
    snprintf(buf, size, "%s|%s|%s|%d|%d|%s", e->equip_id, e->name, e->category,
             e->quantity, e->threshold, e->last_updated);
}
static void fmt_attendance(const Attendance *a, char *buf, size_t size) {
    snprintf(buf, size, "%s|%s|%s|%s", a->class_id, a->student_id, a->date,
             attendance_status_to_string(a->status));
}

int load_admins(void)          { LOAD_ENTITY(ADMIN_FILE, g_data.admins, parse_account, UserAccount); }
int save_admins(void)          { SAVE_ENTITY(ADMIN_FILE, g_data.admins, fmt_account); }
int load_instructors(void)     { LOAD_ENTITY(INSTRUCTOR_FILE, g_data.instructors, parse_instructor, Instructor); }
int save_instructors(void)     { SAVE_ENTITY(INSTRUCTOR_FILE, g_data.instructors, fmt_instructor); }
int load_students(void)        { LOAD_ENTITY(STUDENT_FILE, g_data.students, parse_student, Student); }
int save_students(void)        { SAVE_ENTITY(STUDENT_FILE, g_data.students, fmt_student); }
int load_classes(void)         { LOAD_ENTITY(CLASS_FILE, g_data.classes, parse_class, ClassRecord); }
int save_classes(void)         { SAVE_ENTITY(CLASS_FILE, g_data.classes, fmt_class); }
int load_bookings(void)        { LOAD_ENTITY(BOOKING_FILE, g_data.bookings, parse_booking, Booking); }
int save_bookings(void)        { SAVE_ENTITY(BOOKING_FILE, g_data.bookings, fmt_booking); }
int load_payments(void)        { LOAD_ENTITY(PAYMENT_FILE, g_data.payments, parse_payment, Payment); }
int save_payments(void)        { SAVE_ENTITY(PAYMENT_FILE, g_data.payments, fmt_payment); }
int load_ratings(void)         { LOAD_ENTITY(RATING_FILE, g_data.ratings, parse_rating, Rating); }
int save_ratings(void)         { SAVE_ENTITY(RATING_FILE, g_data.ratings, fmt_rating); }
int load_facility_issues(void) { LOAD_ENTITY(ISSUE_FILE, g_data.issues, parse_issue, FacilityIssue); }
int save_facility_issues(void) { SAVE_ENTITY(ISSUE_FILE, g_data.issues, fmt_issue); }
int load_equipment(void)       { LOAD_ENTITY(EQUIPMENT_FILE, g_data.equipment, parse_equipment, Equipment); }
int save_equipment(void)       { SAVE_ENTITY(EQUIPMENT_FILE, g_data.equipment, fmt_equipment); }
int load_attendance(void)      { LOAD_ENTITY(ATTENDANCE_FILE, g_data.attendance, parse_attendance, Attendance); }
int save_attendance(void)      { SAVE_ENTITY(ATTENDANCE_FILE, g_data.attendance, fmt_attendance); }

/* =====================================================================
 * 5. Whole-database load / save
 * ===================================================================== */

void data_store_reset(void) {
    memset(&g_data, 0, sizeof g_data);
}

int load_all_data(void) {
    int failures = 0;

    data_store_reset();
    cache_init();

    failures += (load_admins()          != 0);
    failures += (load_instructors()     != 0);
    failures += (load_students()        != 0);
    failures += (load_classes()         != 0);
    failures += (load_bookings()        != 0);
    failures += (load_payments()        != 0);
    failures += (load_ratings()         != 0);
    failures += (load_facility_issues() != 0);
    failures += (load_equipment()       != 0);
    failures += (load_attendance()      != 0);

    return (failures == 0) ? 0 : -1;
}

int save_all_data(void) {
    int failures = 0;

    failures += (save_admins()          != 0);
    failures += (save_instructors()     != 0);
    failures += (save_students()        != 0);
    failures += (save_classes()         != 0);
    failures += (save_bookings()        != 0);
    failures += (save_payments()        != 0);
    failures += (save_ratings()         != 0);
    failures += (save_facility_issues() != 0);
    failures += (save_equipment()       != 0);
    failures += (save_attendance()      != 0);

    return (failures == 0) ? 0 : -1;
}

int reload_all_data(void) {
    return load_all_data();
}
