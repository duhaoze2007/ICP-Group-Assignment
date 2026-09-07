# Agent Instructions

## Project context

- This is the SDAMS command-line project for the CT018-3-1-ICP assignment.
- Keep the implementation portable C and consistent with the assignment's ANSI C, text-file persistence, and modular-programming requirements.
- Read [README.md](README.md) for the planned module boundaries and data files, and [the assignment brief](docs/CT018-3-1-ICP%20Group%20Assignment%20Brief.md) for mandatory features and deliverables.

## Implementation rules

- Keep responsibilities separated by module: role features belong in their role module; shared structs belong in `include/common.h`; generic validation belongs in `utils.c`.
- Treat `file_handler.c` as the only owner of application text-file I/O. Other `.c` files must use its API instead of calling `fopen` directly.
- Preserve the command-line interface. Do not add graphics, C++, Java, or hard-coded application data in place of the required text files.
- Validate user input, file-open failures, record limits, identifiers, dates, status transitions, and numeric boundaries at the point of use.
- Prefer small functions, explicit ownership of mutable state, named constants, and interfaces declared in headers. Avoid unrelated refactors.

## Review and testing workflow

- Before changing code, identify the owning module and check nearby callers and declarations.
- Review for correctness first: authorization and role boundaries, stale or invalid records, file parsing and persistence failures, duplicate IDs, capacity/payment invariants, buffer safety, and unchecked return values.
- For every behavior change, add or update focused tests where the repository supports them. Cover normal input, invalid input, boundary values, missing/empty files, and repeated operations when relevant.
- There is currently no committed build system or test suite. At minimum, compile all available sources with strict warnings, for example:

  `cc -std=c11 -Wall -Wextra -pedantic -Iinclude src/*.c -o sdams`

- Run the resulting executable through the changed workflow and record the command and outcome. Do not claim tests passed when only compilation was checked.
- Remove generated binaries and temporary data before committing unless the repository explicitly tracks them.

## Change and commit discipline

- Keep commits small and single-purpose. Use imperative subject lines such as `Add student booking validation` and avoid bundling formatting, generated files, or unrelated fixes.
- Keep diffs minimal, preserve existing public APIs unless required, and update documentation only when behavior or usage changes.
- In the final response, summarize the behavior change, list validation commands and results, and call out any untested path or remaining risk.