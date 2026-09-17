/* =====================================================================
 * SDAMS - file_handler.h   (FILE RESOURCE MANAGER)
 * Owner: M1 - Du Haoze
 *
 * This header is the ONLY interface the other modules may use to reach
 * the .txt files.  Two layers are offered:
 *
 *   1. Generic line layer   (read_all_lines / write_all_lines / append_line)
 *      - raw text access, used by the File Resource Manager menu.
 *   2. Typed entity layer   (load_students / save_students / ...)
 *      - parses "field|field|field" lines straight into the struct arrays
 *        of the global DataStore (g_data).
 * ===================================================================== */

#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stddef.h>
#include "../include/common.h"

/* ---------------- 1. Generic line layer ---------------- */

/* Read all lines from `path` into newly allocated array `*out_lines`.
 * Returns 0 on success; a missing file is treated as empty (count = 0). */
int read_all_lines(const char *path, char ***out_lines, size_t *out_count);

/* Overwrite `path` with `count` lines (each terminated by '\n'). */
int write_all_lines(const char *path, char **lines, size_t count);

/* Append a single line to `path` (creates the file when needed). */
int append_line(const char *path, const char *line);

/* List regular files of directory `dir` (names only). */
int list_files_in_dir(const char *dir, char ***out_files, size_t *out_count);

/* Free an array returned by read_all_lines() / list_files_in_dir(). */
void free_lines(char **lines, size_t count);

/* ---------------- 2. Whole-database operations ---------------- */

int  load_all_data(void);     /* every data txt file  -> g_data (+ cache) */
int  save_all_data(void);     /* g_data               -> every txt file    */
int  reload_all_data(void);   /* re-read every file from disk          */
void data_store_reset(void);  /* empty the in-memory database          */

/* ---------------- 3. Run-time line cache ---------------- */

int get_cached_lines(const char *filename, char ***out_lines, size_t *out_count);
int set_cached_lines(const char *filename, char **lines, size_t count);

/* ---------------- 4. Typed entity layer ----------------
 * One load_xxx() / save_xxx() pair per data file, as required by
 * README.md section "Module Responsibilities".
 * Every load_xxx() returns 0 on success; malformed lines are skipped with
 * a warning so one bad record can never stop the whole system.
 * Every save_xxx() rewrites its file and refreshes the line cache. */
int load_admins(void);            int save_admins(void);
int load_instructors(void);       int save_instructors(void);
int load_students(void);          int save_students(void);
int load_classes(void);           int save_classes(void);
int load_bookings(void);          int save_bookings(void);
int load_payments(void);          int save_payments(void);
int load_ratings(void);           int save_ratings(void);
int load_facility_issues(void);   int save_facility_issues(void);
int load_equipment(void);         int save_equipment(void);
int load_attendance(void);        int save_attendance(void);

#endif /* FILE_HANDLER_H */
