#include "../include/file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define INITIAL_CAPACITY 32
#define LINE_BUF_SIZE 1024

static int ensure_array_capacity(char ***arr, size_t *cap, size_t needed) {
    if (*cap >= needed) return 0;
    size_t newcap = (*cap == 0) ? INITIAL_CAPACITY : (*cap * 2);
    while (newcap < needed) newcap *= 2;
    char **tmp = realloc(*arr, newcap * sizeof(char*));
    if (!tmp) return -1;
    *arr = tmp;
    *cap = newcap;
    return 0;
}

int read_all_lines(const char *path, char ***out_lines, size_t *out_count) {
    if (!path || !out_lines || !out_count) return -1;
    FILE *f = fopen(path, "r");
    if (!f) {
        *out_lines = NULL;
        *out_count = 0;
        return 0; /* treat missing file as empty (caller can create) */
    }
    size_t cap = 0, cnt = 0;
    char **arr = NULL;
    char buf[LINE_BUF_SIZE];
    while (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        /* trim trailing newline */
        if (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
            while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = '\0';
        }
        if (ensure_array_capacity(&arr, &cap, cnt+1) != 0) {
            fclose(f);
            free_lines(arr, cnt);
            return -1;
        }
        arr[cnt] = strdup(buf);
        if (!arr[cnt]) { fclose(f); free_lines(arr, cnt); return -1; }
        cnt++;
    }
    fclose(f);
    *out_lines = arr;
    *out_count = cnt;
    return 0;
}

int write_all_lines(const char *path, char **lines, size_t count) {
    if (!path) return -1;
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    for (size_t i = 0; i < count; ++i) {
        if (lines[i]) fprintf(f, "%s\n", lines[i]);
        else fprintf(f, "\n");
    }
    fclose(f);
    return 0;
}

int append_line(const char *path, const char *line) {
    if (!path || !line) return -1;
    FILE *f = fopen(path, "a");
    if (!f) return -1;
    fprintf(f, "%s\n", line);
    fclose(f);
    return 0;
}

int list_files_in_dir(const char *dir, char ***out_files, size_t *out_count) {
    if (!dir || !out_files || !out_count) return -1;
    DIR *d = opendir(dir);
    if (!d) {
        *out_files = NULL;
        *out_count = 0;
        return 0; /* treat missing dir as empty */
    }
    struct dirent *ent;
    size_t cap = 0, cnt = 0;
    char **arr = NULL;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        /* check regular file */
        char pathbuf[4096];
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(pathbuf, &st) != 0) continue;
        if (!S_ISREG(st.st_mode)) continue;
        if (ensure_array_capacity(&arr, &cap, cnt+1) != 0) { closedir(d); free_lines(arr, cnt); return -1; }
        arr[cnt] = strdup(ent->d_name);
        if (!arr[cnt]) { closedir(d); free_lines(arr, cnt); return -1; }
        cnt++;
    }
    closedir(d);
    *out_files = arr;
    *out_count = cnt;
    return 0;
}

void free_lines(char **lines, size_t count) {
    if (!lines) return;
    for (size_t i = 0; i < count; ++i) free(lines[i]);
    free(lines);
}

/* Simple in-memory cache for data/*.txt files used during a run. This is intentionally
 * lightweight: it maps a known set of filenames to arrays of lines loaded from disk.
 */

struct cache_entry { const char *name; char **lines; size_t count; };

static const char *known_files[] = {
    "admins.txt",
    "instructors.txt",
    "students.txt",
    "classes.txt",
    "bookings.txt",
    "payments.txt",
    "ratings.txt",
    "facility_issues.txt",
    "equipment.txt",
};
static const size_t known_files_count = sizeof(known_files)/sizeof(known_files[0]);
static struct cache_entry cache[sizeof(known_files)/sizeof(known_files[0])];
static int cache_initialized = 0;
static char data_dir_path[512] = "data";

int load_all_data(void) {
    if (!cache_initialized) {
        for (size_t i = 0; i < known_files_count; ++i) {
            cache[i].name = known_files[i];
            cache[i].lines = NULL;
            cache[i].count = 0;
            char path[1024]; snprintf(path, sizeof(path), "%s/%s", data_dir_path, known_files[i]);
            if (read_all_lines(path, &cache[i].lines, &cache[i].count) != 0) return -1;
        }
        cache_initialized = 1;
    }
    return 0;
}

int save_all_data(void) {
    if (!cache_initialized) return 0;
    for (size_t i = 0; i < known_files_count; ++i) {
        char path[1024]; snprintf(path, sizeof(path), "%s/%s", data_dir_path, cache[i].name);
        if (write_all_lines(path, cache[i].lines, cache[i].count) != 0) return -1;
    }
    return 0;
}

int get_cached_lines(const char *filename, char ***out_lines, size_t *out_count) {
    if (!cache_initialized) return -1;
    for (size_t i = 0; i < known_files_count; ++i) {
        if (strcmp(cache[i].name, filename) == 0) {
            *out_lines = cache[i].lines;
            *out_count = cache[i].count;
            return 0;
        }
    }
    *out_lines = NULL; *out_count = 0;
    return -1;
}

int set_cached_lines(const char *filename, char **lines, size_t count) {
    if (!cache_initialized) return -1;
    for (size_t i = 0; i < known_files_count; ++i) {
        if (strcmp(cache[i].name, filename) == 0) {
            free_lines(cache[i].lines, cache[i].count);
            cache[i].lines = lines;
            cache[i].count = count;
            return 0;
        }
    }
    return -1;
}
