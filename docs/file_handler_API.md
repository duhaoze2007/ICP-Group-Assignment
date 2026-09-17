# File Handler API

This module centralises all text-file access for the SDAMS project. Programs should call these functions instead of using fopen/fprintf directly.

## Header

include/file_handler.h

## Functions

- int read_all_lines(const char *path, char ***out_lines, size_t *out_count)
  - Reads the file at `path` and returns an allocated array of lines (no trailing newlines).
  - On success returns 0 and sets `*out_lines` and `*out_count`. If the file does not exist an empty array is returned (count = 0).
  - Caller must free the returned array with `free_lines()`.

- int write_all_lines(const char *path, char **lines, size_t count)
  - Overwrites `path` with `count` lines from `lines`. Each line is written with a terminating newline.
  - Returns 0 on success.

- int append_line(const char *path, const char *line)
  - Appends `line` (plus newline) to `path`. Creates the file if needed.
  - Returns 0 on success.

- int list_files_in_dir(const char *dir, char ***out_files, size_t *out_count)
  - Lists regular files in directory `dir`. Returned names are allocated; free with `free_lines()`.
  - Returns 0 on success.

- void free_lines(char **lines, size_t count)
  - Frees arrays returned by the read/list functions.

- int load_all_data(void)
  - Loads the canonical set of project data files from the `data/` directory into an in-memory cache. Files loaded: admins.txt, instructors.txt, students.txt, classes.txt, bookings.txt, payments.txt, ratings.txt, facility_issues.txt, equipment.txt.
  - Returns 0 on success.

- int save_all_data(void)
  - Writes the in-memory cache back to `data/*.txt` files, overwriting them. Returns 0 on success.

- int get_cached_lines(const char *filename, char ***out_lines, size_t *out_count)
  - Returns pointers into the runtime cache for `filename` (e.g. "students.txt"). Useful for read-only inspection by other modules. Returns 0 on success.

- int set_cached_lines(const char *filename, char **lines, size_t count)
  - Replace the runtime cache for `filename` with the provided `lines` array (ownership transferred). Returns 0 on success.

## Notes

- The cache is a convenience for integration during development. It is intentionally lightweight and does not parse records into structs; parsing belongs to role modules (admin/student/etc.).
- load_all_data() treats missing files as empty; callers should validate required invariants after loading.
- Caller code must check return values and handle I/O errors accordingly.

## Update — typed entity layer (current implementation)

The line cache is still available, but the modules no longer parse strings
themselves. `file_handler.c` now also owns **one `load_xxx()` / `save_xxx()` pair per
data file**, which parse the `field|field|field` lines straight into the struct
arrays of the global `DataStore g_data` (declared in `include/common.h`).

```c
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

int  reload_all_data(void);       /* re-read every file from disk */
void data_store_reset(void);      /* empty the in-memory database */
```

Behaviour:

- `load_all_data()` empties the store, reads the 10 files, parses each record and
  mirrors the raw lines into the cache. A malformed line is skipped with a warning
  on `stderr`; it never aborts the program. Comment lines starting with `#` are ignored.
- `save_xxx()` serialises the struct array back to its file **and** refreshes the
  cache, so the File Resource Manager menu never shows stale content.
- Role modules never call `fopen`. They modify `g_data` and call the matching
  `save_xxx()`. `main.c` also calls `reload_all_data()` after a raw edit made through
  the File Resource Manager menu.
- Missing files are treated as empty data, so a fresh clone boots with an empty system
  instead of an error.

