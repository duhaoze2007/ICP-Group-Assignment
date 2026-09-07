# CT018-3-1-ICP Group Assignment Brief

**Subject:** Introduction to C Programming (ICP)  
**Assignment:** Self Defence Academy Management System (SDAMS)  
**Group Size:** 5 members  
**Submission:** Via Moodle (3 files)  
**Deadline:** Check Moodle

---

## 1. Project Overview

We are required to develop a **Self Defence Academy Management System (SDAMS)** using **ANSI C**. The system centralises daily operations for a martial arts academy (Karate, Judo, Taekwondo, MMA, etc.). Currently, all records are manual – our job is to digitalise and streamline them.

---

## 2. C Language Knowledge Required

To successfully complete this assignment, every group member should be familiar with the following C concepts. The more advanced ones can be distributed among members based on their strengths.

| Level | Concepts | Where/Why Used |
|-------|----------|-----------------|
| **Basic** | Variables & data types (int, float, char, string) | Storing user data (IDs, names, prices, statuses) |
| **Basic** | Input/output functions (printf, scanf, gets/fgets) | Displaying menus, reading user choices, capturing text input |
| **Basic** | Comments (// and /* */) | Explaining code logic for documentation & readability |
| **Intermediate** | Selection control (if, else if, switch) | Menu navigation, login role checking, validation logic |
| **Intermediate** | Iteration control (for, while, do-while) | Looping menus, searching lists, displaying reports |
| **Intermediate** | Arrays (single & double-dimensional) | Storing multiple records (e.g., list of students, classes) |
| **Intermediate** | Strings (string.h functions: strcpy, strcmp, strlen, etc.) | Comparing names/IDs, searching/filtering records |
| **Advanced** | Functions (user-defined) | Modular coding – each feature as a separate function (e.g., addStudent(), viewReport()) |
| **Advanced** | Library functions (stdio.h, stdlib.h, string.h, ctype.h, etc.) | File operations, string manipulation, memory allocation |
| **Advanced** | Pointers | Passing arrays/structures to functions efficiently, dynamic memory if used |
| **Advanced** | Structures (struct) | Defining entities like Student, Instructor, Class, Payment, FacilityIssue |
| **Advanced** | Unions | Optional – can be used for saving memory if a field has multiple possible types |
| **Advanced** | File I/O (fopen, fclose, fread, fwrite, fprintf, fscanf, fgets, etc.) | **CRITICAL** – all data must be stored in .txt files (read/write) |
| **Advanced** | Modular programming (multiple .c and .h files) | Separating code into logical modules (e.g., student.c, instructor.c, file_utils.c) |
| **Advanced** | Symbolic constants (#define) | Defining fixed values like MAX_RECORDS, FILE_PATHS, menu options |
| **Extra** | Linked lists (optional) | If you want dynamic data storage without fixed array limits – earns extra marks |
| **Extra** | Enumerations (enum) | For status fields like Active/Inactive, role types – cleaner code |

> ⚠️ **Note:** File I/O is **mandatory** – without it, the program cannot save/retrieve data between runs.

---

## 3. User Roles & Mandatory Features

| Role | Must-Have Features |
|------|---------------------|
| **Manager** | Login/logout; Manage admin accounts (add/view/search/update status); Staff report (filter by role/name); Revenue report (monthly/yearly with registration count & total revenue) |
| **Administrator** | Login/logout; Manage instructors (add/view/search/update status); Manage students (add/view/search/update status); View student payment records |
| **Student** | Login/logout; Book/view/reschedule/cancel classes; Make payments & view past payment records; Add/view instructor ratings |
| **Instructor** | Login/logout; Manage class schedules (add/view/search/update); Submit facility issues (e.g., damaged mats, broken equipment); View own rating scores |
| **Facility Officer** | Login/logout; Manage equipment inventory (add/view/search/update); View/search/update facility issues; View/search stock reports (by ID or name) |

> 💡 These are the **minimum** requirements. **Extra features** (beyond this list) will earn additional marks.

---

## 4. Technical Requirements

- **NO graphics** – command-line interface only.
- **NO C++ / Java** – portable ANSI C only.
- All data must be stored in **text files**.
- Use **symbolic constants** (#define) where appropriate.
- Include **input validation** (both valid and invalid test cases).
- Use **modular programming** – split code into multiple `.c` and `.h` files.

---

## 5. Deliverables (3 Files to Upload)

| # | File Type | Contents |
|---|-----------|----------|
| 1 | **PDF** – Documentation | Cover page, TOC, introduction/assumptions, design (pseudocode OR flowchart), code snippets with explanation, extra features section, sample I/O screenshots (with explanations), conclusion, APA references, workload matrix |
| 2 | **ZIP/RAR** – Source Code | All `.c` files, `.h` header files, and `.txt` data files |
| 3 | **PDF/DOCX** – Video Link | A link to your presentation/demo video (execution + code walkthrough) |

---

## 6. Marking Scheme (Total 100%)

| Component | Weight | Key Points |
|-----------|--------|------------|
| **Design Solution** (Pseudocode/Flowchart) | 20% | Logical, complete (>75% requirements), minimal errors |
| **Coding / Implementation** | 40% | Compiles & runs, meets >75% requirements, good style + validation, matches design |
| **Documentation** | 20% | Complete structure, APA referencing, clear screenshots + explanations |
| **Demonstration + Q&A** | 20% | Can run system, explain code, answer questions, show extra thinking |

### Grade Bands:
- **Distinction (75%+)** – excellent logic, advanced C concepts, unique solution, flawless documentation & demo.
- **Credit (65–74%)** – good standard, intermediate concepts, minor errors.
- **Pass (50–64%)** – meets ~50% requirements, basic concepts, average documentation.
- **Fail (<50%)** – major errors, no validation, poor documentation, cannot explain code.

> ⚠️ **If a member misses the demo**, their max mark is **40/100** (only design + documentation components).

---

## 7. Important Rules

- **No plagiarism** – APU regulations apply.
- **Data must be saved to text files** – not hardcoded.
- Document all **assumptions** (e.g., default statuses, ID formats) in your report.
- Workload matrix must be filled – total contribution per member = **100%** (if 4 members, ignore column 5).

---

## 8. Suggested Team Workflow

1. **Assign roles** – divide features among members, fill workload matrix early.
2. **Design data structures** – plan `struct` definitions for all entities (User, Student, Instructor, Class, Payment, FacilityIssue, Equipment, etc.).
3. **Design file format** – each entity gets a `.txt` file (e.g., `students.txt`, `classes.txt`).
4. **Build main menu + login** – route users to role-based sub-menus.
5. **Iterative coding** – start with CRUD, then add search/filter/reports/validation.
6. **Test thoroughly** – capture both valid and invalid test screenshots.
7. **Document + record video** – leave 1–2 weeks for this.

---

## 9. Next Steps (Action Items)
- Confirm group members and team leader.
- Divide features by role.
- Set internal deadlines (coding, testing, documentation, video).
- Start research on existing martial arts systems for inspiration.
- Create a shared folder (Google Drive / GitHub) for collaboration.

---
MarkDown 2 PDF Engine Powered by Joplin