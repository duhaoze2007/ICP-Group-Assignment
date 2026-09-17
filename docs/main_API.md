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

## Update — current implementation

The role areas are no longer placeholders. The flow of `main()` is now:

1. `init_system()` — banner, animated progress bar and the start-up log entries
   (same messages as the previous project's `initsystem()`).
2. `load_all_data()` — one read of every `data/*.txt` file into `g_data`; a failure
   only prints a warning.
3. Main menu loop (current time is printed above the menu, like `main.py` did):

   ```
   1. Manager           -> auth_login(ROLE_MANAGER)    -> manager_menu()
   2. Administrator     -> auth_login(ROLE_ADMIN)      -> admin_menu()
   3. Instructor        -> auth_login(ROLE_INSTRUCTOR) -> instructor_menu()
   4. Student           -> auth_login(ROLE_STUDENT)    -> student_menu()
   5. Facility Officer  -> auth_login(ROLE_FACILITY)   -> facility_officer_menu()
   6. File Resource Manager (list / view / append / overwrite / record counts)
   7. Save & Exit
   ```

   A failed or cancelled login logs `Security System: <role> login failed` and returns
   to the main menu. Every role menu has `0. Logout and back`, which calls
   `auth_logout()` and closes the session.
4. `Save & Exit` writes every file with `save_all_data()`, logs `System shutdown`,
   animates the progress bar and returns 0.

The File Resource Manager options 3 and 4 write directly to disk through
`append_line()` / `write_all_lines()` and then call `reload_all_data()`, so the typed
database can never drift from the files on disk.

Regression scripts for all of this live in `tests/` — run `bash tests/run_cases.sh ./sdams`.


