#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/file_handler.h"

static void pause_for_enter(void) {
    printf("\nPress Enter to continue...");
    int c; while ((c = getchar()) != '\n' && c != EOF) {}
}

static void show_role_stub(const char *role) {
    printf("\n--- %s area ---\n", role);
    printf("This area is a placeholder. Individual role modules implement CLI sub-menus per README.\n");
    pause_for_enter();
}

int main(void) {
    char choice[16];
    if (load_all_data() != 0) {
        fprintf(stderr, "Warning: failed to load data files. Continuing with empty data.\n");
    }

    while (1) {
        printf("\n=== SDAMS Main Menu ===\n");
        printf("1. Manager (login required)\n");
        printf("2. Administrator\n");
        printf("3. Instructor\n");
        printf("4. Student\n");
        printf("5. Facility Officer\n");
        printf("6. File Resource Manager (list/view/append/overwrite)\n");
        printf("7. Save & Exit\n");
        printf("Select an option: ");
        if (!fgets(choice, sizeof(choice), stdin)) break;
        int opt = atoi(choice);
        switch (opt) {
            case 1:
                show_role_stub("Manager");
                break;
            case 2:
                show_role_stub("Administrator");
                break;
            case 3:
                show_role_stub("Instructor");
                break;
            case 4:
                show_role_stub("Student");
                break;
            case 5:
                show_role_stub("Facility Officer");
                break;
            case 6: {
                /* Delegate to the smaller file manager implemented earlier */
                char inner[16];
                while (1) {
                    printf("\n--- File Resource Manager ---\n");
                    printf("1. List data files\n");
                    printf("2. View file contents\n");
                    printf("3. Append a line to a file\n");
                    printf("4. Overwrite a file\n");
                    printf("5. Back to main menu\n");
                    printf("Choose: "); if (!fgets(inner, sizeof(inner), stdin)) break;
                    int iopt = atoi(inner);
                    if (iopt == 1) {
                        char **files = NULL; size_t n = 0;
                        if (list_files_in_dir("data", &files, &n) != 0) printf("Error listing files\n");
                        else {
                            printf("\nFiles in data/:\n");
                            for (size_t i = 0; i < n; ++i) printf("  %zu. %s\n", i+1, files[i]);
                            if (n==0) printf("  (no files)\n");
                        }
                        free_lines(files, n);
                        pause_for_enter();
                    } else if (iopt == 2) {
                        char fname[512];
                        printf("Enter filename (relative to data/): ");
                        if (!fgets(fname, sizeof(fname), stdin)) break;
                        fname[strcspn(fname, "\n")] = '\0';
                        char path[1024]; snprintf(path, sizeof(path), "data/%s", fname);
                        char **lines = NULL; size_t n = 0;
                        if (read_all_lines(path, &lines, &n) != 0) printf("Error reading file\n");
                        else {
                            printf("\nContents of %s (%zu lines):\n", path, n);
                            for (size_t i = 0; i < n; ++i) printf("%4zu: %s\n", i+1, lines[i]);
                            if (n==0) printf("(empty)\n");
                        }
                        free_lines(lines, n);
                        pause_for_enter();
                    } else if (iopt == 3) {
                        char fname[512]; char line[1024];
                        printf("Enter filename (relative to data/): "); if (!fgets(fname, sizeof(fname), stdin)) break;
                        fname[strcspn(fname, "\n")] = '\0';
                        printf("Enter the line to append: "); if (!fgets(line, sizeof(line), stdin)) break;
                        line[strcspn(line, "\n")] = '\0';
                        char path[1024]; snprintf(path, sizeof(path), "data/%s", fname);
                        if (append_line(path, line) != 0) printf("Error appending to %s\n", path);
                        else printf("Appended to %s\n", path);
                        pause_for_enter();
                    } else if (iopt == 4) {
                        char fname[512];
                        printf("Enter filename (relative to data/): "); if (!fgets(fname, sizeof(fname), stdin)) break;
                        fname[strcspn(fname, "\n")] = '\0';
                        char path[1024]; snprintf(path, sizeof(path), "data/%s", fname);
                        printf("Enter lines (end with a single '.' on its own line):\n");
                        char **buf = NULL; size_t cap = 0, cnt = 0;
                        char tmp[1024];
                        while (1) {
                            if (!fgets(tmp, sizeof(tmp), stdin)) break;
                            tmp[strcspn(tmp, "\n")] = '\0';
                            if (strcmp(tmp, ".") == 0) break;
                            if (cap == 0) { cap = 8; buf = malloc(cap * sizeof(char*)); }
                            if (cnt + 1 > cap) { cap *= 2; buf = realloc(buf, cap * sizeof(char*)); }
                            buf[cnt++] = strdup(tmp);
                        }
                        if (write_all_lines(path, buf, cnt) != 0) printf("Error writing file %s\n", path);
                        else printf("Wrote %zu lines to %s\n", cnt, path);
                        free_lines(buf, cnt);
                        pause_for_enter();
                    } else if (iopt == 5) {
                        break;
                    } else {
                        printf("Invalid option\n");
                    }
                }
                break;
            }
            case 7:
                if (save_all_data() != 0) fprintf(stderr, "Warning: failed to save data files.\n");
                printf("Data saved. Exiting.\n");
                return 0;
            default:
                printf("Invalid option.\n");
        }
    }
    /* on abnormal exit try to save */
    save_all_data();
    return 0;
}
