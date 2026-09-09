# Main (CLI) — SDAMS Project Entry Point

This `main.c` is the canonical entry point for the SDAMS command-line system. It implements the top-level interactive menu described in README.md and coordinates module initialization and shutdown.

Behavior

- On startup, `main()` calls `load_all_data()` from the file handler module to populate an in-memory cache of canonical `data/*.txt` files used by the project.
- The program then displays the top-level role-based menu and routes to role-specific sub-menus. The current implementation provides placeholders for each role; role modules (auth, manager, admin, instructor, student, facility_officer) should implement their own sub-menus and use the file handler cache API to read/modify data.
- Selecting the File Resource Manager option opens a sub-menu that exposes the file handler utilities (list files, view file, append line, overwrite file) for ad-hoc data inspection and editing.
- On exit (Save & Exit), `main()` calls `save_all_data()` to flush the runtime cache back to `data/*.txt` files.

Top-level menu options (current implementation)

1. Manager — placeholder; intended to require authentication and then show manager features.
2. Administrator — placeholder for admin features.
3. Instructor — placeholder for instructor features.
4. Student — placeholder for student features.
5. Facility Officer — placeholder for facility officer features.
6. File Resource Manager — interactive sub-menu:
   - List data files in `data/` directory
   - View a file's contents with line numbers
   - Append a single line to a file
   - Overwrite a file by entering multiple lines (terminate with a single `.`)
   - Back to main menu
7. Save & Exit — persist cache and exit the program

Integration notes

- Role modules must not call `fopen`/`fprintf` directly; they should parse and manipulate the raw lines from `get_cached_lines()` and persist changes by calling `set_cached_lines()` and letting `save_all_data()` persist on exit, or by invoking `write_all_lines()` / `append_line()` for immediate persistence.
- The main program intentionally keeps role areas as stubs so feature development can proceed per-module without coupling.
- Error handling: `load_all_data()` and `save_all_data()` return non-zero on I/O failure; `main()` logs warnings and attempts graceful continuation.

Developer checklist

- Implement each role module to parse its corresponding `data/*.txt` file format into typed structs.
- Use `get_cached_lines()` to read, `set_cached_lines()` to update the runtime cache, and `save_all_data()` to persist.
- Add login/authentication in `auth.c` and integrate with role menu routing.

