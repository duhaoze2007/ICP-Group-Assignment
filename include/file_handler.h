#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stddef.h>

/* Read all lines from `path` into newly allocated array `*out_lines`.
 * Returns 0 on success, non-zero on error. Caller frees with free_lines().
 */
int read_all_lines(const char *path, char ***out_lines, size_t *out_count);

/* Write `count` lines to `path` (overwrite). Returns 0 on success.
 */
int write_all_lines(const char *path, char **lines, size_t count);

/* Append a single line (without adding extra newlines) to `path`.
 * Returns 0 on success.
 */
int append_line(const char *path, const char *line);

/* List regular files in directory `dir` (names only). Caller frees with free_lines().
 * Returns 0 on success.
 */
int list_files_in_dir(const char *dir, char ***out_files, size_t *out_count);

/* Free the array returned by read_all_lines / list_files_in_dir. */
void free_lines(char **lines, size_t count);

/* Lightweight run-time cache and convenience functions for the project's canonical data files. */
int load_all_data(void);    /* load data/*.txt into memory cache (returns 0 on success) */
int save_all_data(void);    /* write cache back to disk */
int get_cached_lines(const char *filename, char ***out_lines, size_t *out_count);
int set_cached_lines(const char *filename, char **lines, size_t count);

#endif /* FILE_HANDLER_H */
