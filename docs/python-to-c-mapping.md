# Python → C Conversion Map — *FitZone Gym* (previous project) → *SDAMS* (CT018-3-1-ICP)

This document records **how the C group assignment was built on top of the previous
semester's Python group project**, so that the conversion can be explained in the
report and defended during the demo / Q&A.

* Previous project: **FitZone Gym Management System** (`~/Desktop/Project Codes`, Python 3, 5 subsystems)
* Current project: **SDAMS — Self Defence Academy Management System** (ANSI C11, 10 modules)

---

## 1. Verdict — can the Python project be converted?

Yes, but not by a mechanical line-by-line translation. What transfers is the
**architecture and the logic patterns** (menu router, per-role subsystem, one text
file per entity, helper layer for I/O + validation + logging, id generation, timed
penalty rule). What does **not** transfer is the domain (a gym is not a martial-arts
academy) and Python's dynamic conveniences (`set`, list comprehensions, exceptions,
`datetime`, `dict` of file names). Those were re-implemented with C arrays, loops,
`enum`s, `<time.h>` and hand-written parsers.

Both projects share the same five-layer shape, which is why the port is natural:

| Layer | Python (FitZone) | C (SDAMS) |
| --- | --- | --- |
| Entry / router | `main.py` → `main()` | `src/main.c` → `main()` |
| I/O layer | `file_utils.py` → `read_file()` / `write_file()` / `append_file()` | `src/file_handler.c` → `read_all_lines()` / `write_all_lines()` / `append_line()` + typed `load_xxx()` / `save_xxx()` |
| Helpers | `utils.py` + `cli_utils.py` | `src/utils.c` (input, validation, IDs, dates, log, password) |
| Constants | `DATA_FILES` dict inside `utils.py` | `include/common.h` (`#define` file names, limits, business rules) |
| Subsystems | `admin.py`, `booking.py`, `accountant.py`, `member.py`, `trainer.py` | `manager.c`, `admin.c`, `student.c`, `instructor.c`, `facility_officer.c` (+ `auth.c`, `reports.c`) |

---

## 2. Module mapping (Python file → C file)

| Python (previous project) | C (SDAMS) | Owner | Notes |
| --- | --- | --- | --- |
| `main.py` (banner, progress bar, role menu, shutdown) | `src/main.c` | M1 | Kept the start-up banner, the animated progress bar and the log messages of `initsystem()`. The per-subsystem password prompt was replaced by a real login router. |
| `file_utils.py` | `src/file_handler.c` | M1 | Extended from 3 line functions to a typed layer with one `load_xxx()` / `save_xxx()` pair per entity, as the C brief demands. |
| `utils.py` (`log`, `generate_booking_id`, `passwd`) | `src/utils.c` | M1 | `log` → `log_event()`, `generate_booking_id` → `id_number()` / `make_id()`, `passwd` → `check_password()`. `DATA_FILES` dict → `#define` constants. |
| `cli_utils.py` (`pause`, `valid_input`, `show_progress_bar`) | `src/utils.c` | M1 | `valid_input()` → `read_choice()` (same loop-until-valid behaviour). |
| `admin.py` (add/delete class, trainer, member; `view_all`) | `src/admin.c` + `src/manager.c` | M3 / M2 | The "administrator manages people and resources" split of the Python project maps onto SDAMS Administrator (instructors + students) and Manager (staff accounts + reports). |
| `booking.py` (`view_classes`, `register_member`, `book_class`, `cancel_booking`, `reschedule`, `view_history`) | `src/student.c` | M4 | Book / cancel / reschedule / history became student features; `register_member()` has no SDAMS equivalent (students are created by the administrator), so it was dropped. |
| `accountant.py` (`record_payment`, `income_report`, `unpaid_members`) | `src/student.c` + `src/reports.c` | M4 / M1 | `record_payment()` → `make_payment()`; `income_report()` → `calculate_total_revenue()` / `generate_revenue_report()`. `unpaid_members()` (Python `set` difference) became the payment-record filter in `admin.c`. |
| `member.py` (member menu) | `src/student.c` | M4 | Same menu concept, but the member/student ID now comes from the login session instead of being retyped. |
| `trainer.py` (`record_attendance`, `record_notes`, `report`) | `src/instructor.c` | M4 | Attendance was kept as an **extra feature** (`mark_attendance()`, `view_attendance_report()`); the free-text "notes" file was dropped as it is not a managed entity. |
| — (no equivalent) | `src/instructor.c` (class CRUD), `src/facility_officer.c`, `src/facility_officer.c` equipment, `src/auth.c`, `src/reports.c` (staff report) | M4 / M5 / M2 / M1 | New modules required by the SDAMS brief; the CRUD/filter/report patterns were copied from `admin.py`. |

---

## 3. Function mapping (the interesting ones)

| Python function | C function | What changed |
| --- | --- | --- |
| `utils.log(msg)` → `append_file("system.log", f"[{t}] {msg}")` | `log_event(fmt, ...)` | `strftime` timestamp, then `append_line()` — logging still cannot break the program. |
| `utils.generate_booking_id()` → `f"B{int(time.time())}"` | `id_number()` + `make_id()` + per-module scan of existing IDs | Fixed-width sequential IDs (`B001`, `P004`) instead of timestamps: unique, sortable, easy to explain, and reuse-free. |
| `utils.passwd(123456, 5)` (hard-coded password, loops until match) | `check_password(expected, attempts)` **and** `auth_login(role)` | Two levels: the generic retry loop was kept, but the real check now compares a typed ID + password against `admins.txt` / `students.txt` / `instructors.txt` and records a `Session` for the role modules. |
| `cli_utils.valid_input(prompt, ['1','2',...])` | `read_choice(prompt, "123...", &c)` | `list` membership test → `strchr()` on a string constant. |
| `booking.register_member()` (name + 30-day expiry) | `admin.add_student()` | Duplicate-name check (`name in read_file(...)`) became an ID-prefix check (`validate_id("S")`) with an auto-generated `S006` ID. |
| `booking.book_class()` (any name, any class) | `student.book_class()` | Added: class must exist, be active, start in the future, have a free seat, and hold no duplicate active booking for the same student. |
| `booking.cancel_booking()` → `if datetime.now() - t < timedelta(hours=5): append_file(payments, f"{member},Penalty,10")` | `student.cancel_booking()` → `hours_between(now, class_datetime) < LATE_CANCEL_HOURS` → `Payment` with `type = penalty`, `amount = 10.00` | Kept **exactly** as a business rule; the penalty is now a proper record (`P004|S001|C008|10.00|2026-09-17|penalty`) instead of a loosely formatted line, so the revenue report can separate fees from penalties. |
| `booking.reschedule()` (`b.startswith(bid)` string surgery) | `student.reschedule_booking()` | Proper field lookup plus seat bookkeeping: one seat is returned to the old class and taken from the new one. |
| `accountant.income_report()` (`try: total += float(p.split(',')[1]) except: pass`) | `calculate_total_revenue()` | Explicit `PaymentType` check instead of swallowing an exception; fees and penalties are totalled separately. |
| `accountant.unpaid_members()` (`set(members) - set(payments)`) | `admin.view_student_payments()` filter + the "students with no fee record" check by scanning both arrays | C has no set type, so two loops replace the set arithmetic. |
| `admin.delete_*()` (`[x for x in lines if x != name]` then rewrite the file) | `manager.delete_admin_account()`, `admin.delete_instructor()`, `admin.delete_student()`, `facility_officer.delete_equipment()` | List comprehension → in-place array compaction (shift elements left, decrement `count`), followed by `save_xxx()`. Delete is now guarded (logged-in account, active classes, penalty records). |
| `admin.view_all()` (dump every data file) | `view_all_admins()` / `view_all_instructors()` / `view_all_students()` / `view_all_equipment()` + the File Resource Manager menu in `main.c` | Split into per-entity tables; the raw dump still exists as option 6 of the main menu. |
| `cli_utils.show_progress_bar()` (Unicode block, 0.2 s sleep) | `show_progress_bar()` (`#` fill, `clock()` busy wait) | Unicode blocks and `time.sleep()` were replaced to stay portable text-mode C. |

---

## 4. Data file mapping

| Python `DATA_FILES` key | Python file | C file | Format change |
| --- | --- | --- | --- |
| `members` | `members.txt` | `students.txt` | `Name,Expiry` → `S001\|Name\|Contact\|Status` |
| `classes` | `classes.txt` (bare class names) | `classes.txt` | name only → `ClassID\|InstructorID\|MartialArt\|DateTime\|Capacity\|BookedCount\|Status` |
| `trainers` | `trainers.txt` (bare names) | `instructors.txt` | name only → `ID\|Name\|Role\|Contact\|Status\|AvgRating` |
| `bookings` | `bookings.txt` | `bookings.txt` | `B1777306095,LOO SUN ZE,HIIT,2026-04-28 00:08` → `B001\|S001\|C001\|2026-09-15 10:00\|booked` (status field added) |
| `payments` | `payments.txt` | `payments.txt` | `Name,100.0,ID: B1777302037` → `P001\|S001\|C001\|120.00\|2026-09-15\|payment`; the `Type` field replaces the old `"Penalty"` text trick |
| `attendance` | `attendance.txt` | `attendance.txt` | space-separated free text → `ClassID\|StudentID\|Date\|Status` |
| `log` | `system.log` | `data/system.log` | unchanged idea (`[timestamp] message`) |
| `notes` | `notes.txt` | — | dropped, not a managed entity |
| — | — | `admins.txt`, `ratings.txt`, `facility_issues.txt`, `equipment.txt` | new files required by the SDAMS brief |

**Format change to note in the report:** `payments.txt` gained a sixth field
(`Type` = `payment` / `penalty`) so that the revenue report can separate class fees
from late-cancellation penalties. README.md was updated accordingly.

---

## 5. Feature coverage of the SDAMS brief

| Brief requirement | Where it lives | Ported / new |
| --- | --- | --- |
| Login / logout (all roles) | `auth.c` + `main.c` router | new (Python used one shared hard-coded password per subsystem) |
| Manager: admin accounts CRUD + status toggle | `manager.c` | ported from `admin.py` (trainer/member CRUD) |
| Manager: staff report (filter role / name) | `reports.c` | new |
| Manager: revenue report (month / year, registrations + total) | `reports.c` | ported from `accountant.income_report()`, extended with the period filter |
| Administrator: instructor CRUD + status, student CRUD + status | `admin.c` | ported from `admin.py` |
| Administrator: view student payment records | `admin.c` | ported from `accountant.py` |
| Student: book / view / reschedule / cancel classes | `student.c` | ported from `booking.py` |
| Student: pay + view payment history | `student.c` | ported from `booking.py` + `accountant.py` |
| Student: rate instructor + view ratings | `student.c` | new (Python had no ratings) |
| Instructor: class schedule CRUD | `instructor.c` | ported from `admin.py` (class part) + `trainer.py` |
| Instructor: facility issues | `instructor.c` | new |
| Instructor: own rating scores | `instructor.c` | new |
| Facility Officer: equipment CRUD, issues, stock report | `facility_officer.c` + `reports.c` | new (no facilities role in Python) |
| **Extra:** attendance marking + report | `instructor.c` | ported from `trainer.py` |
| **Extra:** activity log (`system.log`) | `utils.c` | ported from `utils.py` |
| **Extra:** animated progress bar | `utils.c` | ported from `cli_utils.py` |
| **Extra:** security prompt with attempt counter | `utils.c` | ported from `utils.py` |
| **Extra:** low-stock warnings, late-cancellation penalty | `facility_officer.c`, `student.c` | penalty ported, low-stock new |

---

## 6. C concepts demonstrated (for the "Implementation" chapter)

| Concept | Example in this project |
| --- | --- |
| `struct` + `enum` | `include/common.h` — 10 entity structs, `Status`, `BookingStatus`, `PaymentType`, `IssueStatus`, `Role` |
| Arrays of structs | `DataStore g_data` (e.g. `g_data.students.items[i]`, `count`) |
| Pointers | `char ***out_lines`, `Student *record`, `const Session *get_logged_in_user()` |
| Dynamic memory | `read_all_lines()` (`malloc`/`realloc`/`free`), `free_lines()` |
| File I/O | `fopen` / `fgets` / `fprintf` / `fclose` in `file_handler.c` only; one cache mirror per file |
| String handling | `strlen`, `strcpy`-style bounded copies, `strcmp`, `strchr`, `strtol`, `strtod`, `sscanf`, `snprintf` |
| Modular programming | 10 `.c` + 10 `.h` files, every module owns its menu and its records |
| Symbolic constants | `MAX_RECORDS`, `ID_LEN`, `DEFAULT_PASSWORD`, `LATE_CANCEL_HOURS`, `MAX_PAYMENT`, file names |
| Date / time | `time`, `localtime`, `strftime`, `mktime`, `difftime` (validation, "future class", 5-hour penalty window) |
| Input validation | `read_int` / `read_double` / `read_choice`, `validate_date`, `validate_datetime`, `validate_id`, `validate_contact`, `validate_amount` |
| Defensive file parsing | `split_fields()` + `parse_xxx()` skip malformed lines with a warning instead of crashing |
| Macros used for repetition | `LOAD_ENTITY` / `SAVE_ENTITY` in `file_handler.c` keep 20 load/save functions consistent |

> Optional extra marks: linked lists are not used (fixed arrays with `MAX_RECORDS`),
> but the brief lists them as *extra*, not required.

---

## 7. Things that had to change (and why) — talk about these in the Q&A

1. **No `set` type** — `unpaid_members()` became a two-loop comparison.
2. **No exceptions** — the `try/except` around payment parsing became an explicit
   `PaymentType` check and a `validate_amount()` gate.
3. **No `dict`** — `DATA_FILES` became `#define` constants, and the entities became
   typed arrays instead of lists of strings.
4. **Format strings / fixed sizes** — every field has a width (`ID_LEN`, `NAME_LEN`, …)
   and every copy is bounded, so overflow is impossible.
5. **Login instead of a shared password** — the Python project asked every subsystem
   the same password `123456`; the brief requires login/logout per role, so a real
   session (`g_session`) was introduced.
6. **Seat bookkeeping** — bookings now keep `ClassRecord.booked_count` in sync
   (Python never tracked capacity), which is where the capacity/full-class
   validation and the reschedule logic come from.
7. **Plaintext passwords** — allowed by the brief, but the account list masks them
   (`******`) because printing credentials is bad practice.

---

## 8. Referencing this honestly

The brief's report chapter 6 asks for "existing martial arts management systems
reviewed". Use this conversion as one of the reviewed systems **and** declare it in
the workload matrix / assumptions:

```
Du Haoze (2026) FitZone Gym Management System [Python source code].
Previous semester group project, Asia Pacific University. Adapted to ANSI C as
SDAMS for CT018-3-1-ICP.
```

Every member must still be able to explain their own module (and draw its
flowchart/pseudocode), because the demonstration and Q&A carry 20 % of the mark.
