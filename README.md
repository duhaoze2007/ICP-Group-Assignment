# ICS Project – Source Code & Documentation Structure (Brief)

## 📁 Project Root Directory (GitHub Repo): `ICP-Group-Assignment/`

[GitHub Repo Link](https://github.com/duhaoze2007/ICP-Group-Assignment)

```text
ICP-Group-Assignment/
│
├── src/                           # All .c source files
│   ├── main.c                    # Program entry point (main menu + login router)
│   ├── auth.c                    # Login / Logout authentication module
│   ├── manager.c                 # Manager features (Staff Report, Revenue Report, Manage Admins)
│   ├── admin.c                   # Administrator features (Manage Instructors/Students + Payment Records)
│   ├── student.c                 # Student features (Book/Reschedule/Cancel Classes + Pay + Rate Instructors)
│   ├── instructor.c              # Instructor features (Class Schedule + Facility Issues + View Ratings)
│   ├── facility_officer.c        # Facility Officer features (Equipment Inventory + Facility Issues + Stock Reports)
│   ├── file_handler.c            # FILE RESOURCE MANAGER (All .txt read/write operations)
│   ├── reports.c                 # Report generation module (Revenue, Staff stats, Stock summaries)
│   └── utils.c                   # Utility tools (Input validation, string handling, ID generation, date checks)
│
├── include/                       # All .h header files
│   ├── common.h                  # Global struct definitions (Student, Instructor, Class, Payment, FacilityIssue, Equipment, User)
│   ├── auth.h
│   ├── manager.h
│   ├── admin.h
│   ├── student.h
│   ├── instructor.h
│   ├── facility_officer.h
│   ├── file_handler.h
│   ├── reports.h
│   └── utils.h
│
├── data/                          # All .txt data files (read/written at runtime)
│   ├── admins.txt                # Admin accounts (ID|Name|Password|Status)
│   ├── instructors.txt           # Instructor info (ID|Name|Role|Contact|Status|AvgRating)
│   ├── students.txt              # Student info (ID|Name|Contact|Status)
│   ├── classes.txt               # Class schedules (ClassID|InstructorID|MartialArt|DateTime|Capacity|BookedCount|Status)
│   ├── bookings.txt              # Student bookings (BookingID|StudentID|ClassID|BookingDate|Status)
│   ├── payments.txt              # Payment records (PaymentID|StudentID|ClassID|Amount|PaymentDate)
│   ├── ratings.txt               # Instructor ratings (RatingID|StudentID|InstructorID|Score|Comment|Date)
│   ├── facility_issues.txt       # Facility issues (IssueID|InstructorID|Location|Description|Status|ReportedDate)
│   └── equipment.txt             # Equipment inventory (EquipID|Name|Category|Quantity|Threshold|LastUpdated)
│
├── docs/                          # Documentation materials
│   └── screenshots/              # Screenshots for the final PDF report
│
└── README.md                      # Short build/run instructions
```

* * *

## Module Responsibilities

| File | Core Responsibility | Key C Concepts Used |
| --- | --- | --- |
| **main.c** | Display main menu, route to role-based sub-menus, load data at start, save on exit | `switch`, `while`, function calls |
| **auth.c** | Login validation (ID + Password), logout, track current logged-in user | `strcmp`, `struct`, file reading |
| **manager.c** | CRUD for Admin accounts; generate Staff Reports (filter by role/name); generate Monthly/Yearly Revenue Reports | Array/linked-list traversal, conditional filtering, summation |
| **admin.c** | Manage Instructors (CRUD + status toggle); Manage Students (CRUD + status toggle); View student payment history | Struct arrays, CRUD operations, status enumeration |
| **student.c** | Book classes (view available → select → write booking); Reschedule/Cancel; Make payments; Rate instructors | Date comparison, cross-table queries (Student→Class→Instructor) |
| **instructor.c** | Manage own class schedules (CRUD); Submit facility issues; View own average rating | Filter by InstructorID, average score calculation |
| **facility_officer.c** | Equipment CRUD; View/Update facility issues (pending → fixed); Generate stock reports (filter by ID/name) | Stock threshold alerts, report filtering |
| **file_handler.c** ★ | **ALL .txt file I/O** – one `load_xxx()` and `save_xxx()` function per entity (e.g., `loadStudents()`, `saveStudents()`, `loadClasses()`, `saveClasses()`). | `FILE*`, `fopen`, `fclose`, `fscanf`, `fprintf`, `fgets`, `feof` |
| **reports.c** | Aggregation functions: count staff by role, calculate total revenue, group by month/year, low-stock warnings | Traversal + accumulation, conditional filtering, formatted table output |
| **utils.c** | Generic input validations (numeric, date format, non-empty string); Generate unique IDs (e.g., `S001`, `I001`); Menu confirmation prompts | `isdigit`, `strlen`, `atoi`, manual format checking |

> **Important**: `file_handler.c` is your **File Resource Manager**. No other `.c` file should directly call `fopen` – they must go through `file_handler.h` functions. This keeps **business logic** and **data persistence** completely separate.

* * *

## Final Report (PDF Documentation) Structure

Based on the marking scheme (Documentation = 20%), your report **must** include these sections:

```
Cover Page
├── Project Title: Self Defence Academy Management System (SDAMS)
├── Subject Code: CT018-3-1-ICP
├── Group Members' Names & IDs
├── Submission Date

Table of Contents

1. Introduction
   ├── Project Background (current manual issues at the academy)
   ├── Project Objectives
   └── Team Assumptions (e.g., default passwords, ID formats, operating hours)

2. Design Solution (20% of total grade – very important)
   ├── Overall System Architecture (module diagram)
   ├── Flowcharts OR Pseudocode for each major module
   │   ├── Login flow
   │   ├── Role-based main menu flows
   │   └── Core feature logic (e.g., Booking, Reports)
   └── Data Structure Design (all struct definitions with explanations)

3. Implementation & Code Explanation
   ├── Modular programming approach (.c / .h file breakdown)
   ├── Key code snippets with explanations (show structs, pointers, file I/O, functions)
   ├── Input validation mechanisms (e.g., empty input, negative numbers, date format checks)
   └── Additional features (if any, with code examples)

4. Sample Outputs (Screenshots)
   ├── Positive test cases (2-3 screenshots per core role feature + explanations)
   └── Negative test cases (error inputs, boundary values + explanations)

5. Conclusion
   ├── Project summary
   ├── Challenges faced & solutions
   └── Future improvements

6. References (APA Style)
   ├── C programming textbooks / documentation
   ├── Online resources referenced
   └── Existing martial arts management systems reviewed

Appendix
   └── Workload Matrix (each member's contribution % per module – total must sum to 100%)
```

* * *

# SDAMS – Task Division & Workload Assignment (5 Members)

**Team Leader (M1):** *Du Haoze* – Architecture & Integration Lead

* * *

## 1\. Team Role Overview

| Member | Role Title | Assigned `.c` Files | Core Responsibility |
| --- | --- | --- | --- |
| **M1 (Du Haoze)** | **Lead Architect & Integrator** | `main.c`, `file_handler.c`, `utils.c`, `reports.c` | Entry point, all file I/O, utilities, report helper functions, final documentation merging & formatting |
| **M2 (Justin Loo)** | **Authentication & Management Lead** | `auth.c`, `manager.c` | Login/logout, Admin CRUD, Staff Report, Revenue Report |
| **M3 (Htoo Aung Htet)** | **Administrative Control Lead** | `admin.c` | Manage Instructors (CRUD + status), Manage Students (CRUD + status), View payment records |
| **M4 (Lhaksam Tiempey Geltsan)** | **Operations & Training Lead** | `student.c`, `instructor.c` | Class booking/reschedule/cancel, payments, ratings (Student side); Class schedule + facility issues (Instructor side) |
| **M5 (Rehan Ali)** | **Facilities & Logistics Lead** | `facility_officer.c` | Equipment inventory (CRUD), Facility issue management (update status), Stock reports |

> **Header files (.h)**: Each member creates and maintains their own `.h` file (e.g., M2 → `auth.h`, `manager.h`). Shared structs go into `common.h` (maintained by M1).

* * *

## 2\. Detailed Task Breakdown

### M1 – Team Leader (Du Haoze)

**Files:** `main.c` + `file_handler.c` + `utils.c` + `reports.c` + `common.h`

| Category | Specific Functions / Tasks |
| --- | --- |
| **Main Program** | `main()` – display main menu, login router, call `loadAllData()` on start and `saveAllData()` on exit. |
| **File Resource Manager** | Write **ALL** `load_xxx()` and `save_xxx()` functions for 9 entities (admins, instructors, students, classes, bookings, payments, ratings, issues, equipment). Use `fopen`, `fscanf`, `fprintf`, `fclose`. |
| **Utilities Example** | `generateID()` (e.g., A001, S001), `validateDate()`, `validateNonEmpty()`, `confirmAction()`, `toLowerCase()` for case-insensitive search. |
| **Reports Helper** | Generic formatting helpers (e.g., `printTableBorder()`, `calculateTotalRevenue()`, `countByRole()`) – *called by M2 and M5 for their reports*. |
| **Documentation** | **Final PDF compilation**: Merge all members' sections, format TOC, write Introduction, Conclusion, Assumptions, and Workload Matrix. Ensure APA references are unified. |
| **Coordination** | Set internal deadlines, review pull requests, ensure code consistency, record final demo video (or coordinate recording). |

* * *

### 👤 M2 – Authentication & Management (Justin Loo)

**Files:** `auth.c` + `manager.c` + `auth.h` + `manager.h`

| Category | Specific Functions / Tasks |
| --- | --- |
| **Auth Module** | `login()` – check ID/password from `admins.txt`; `logout()` – clear session; `getLoggedInUser()` – return current user role. |
| **Manager – Admin Management** | `addAdmin()`, `viewAllAdmins()`, `searchAdminByNameOrID()`, `updateAdminStatus()` (active ↔ not active). |
| **Manager – Staff Report** | `generateStaffReport()` – display all admins + instructors, filter by role and name, show total count per role. |
| **Manager – Revenue Report** | `generateRevenueReport()` – filter by month/year, display total class registrations + total revenue for that period. |
| **Documentation** | Write the **Design Flowchart/Pseudocode** for your modules + provide 2-3 screenshots (positive + negative tests) + code snippets for the final PDF. |

* * *

### 👤 M3 – Administrative Control (Htoo Aung Htet)

**Files:** `admin.c` + `admin.h`

| Category | Specific Functions / Tasks |
| --- | --- |
| **Instructor Management** | `addInstructor()`, `viewAllInstructors()`, `searchInstructorByID/Name/Contact()`, `updateInstructorStatus()` (active ↔ not active). |
| **Student Management** | `addStudent()`, `viewAllStudents()`, `searchStudentByID/Name()`, `updateStudentStatus()` (active ↔ not active). |
| **Payment Records (View)** | `viewStudentPayments()` – filter by Student ID, display all past payments from `payments.txt`. |
| **Documentation** | Write the **Design Flowchart/Pseudocode** for your modules + provide 2-3 screenshots (positive + negative tests) + code snippets for the final PDF. |

* * *

### 👤 M4 – Operations & Training (Lhaksam Tiempey Geltsan)

**Files:** `student.c` + `instructor.c` + `student.h` + `instructor.h`

| Category | Specific Functions / Tasks |
| --- | --- |
| **Student – Booking** | `bookClass()` – display available classes from `classes.txt`, let student select, write to `bookings.txt`; `viewMyBookings()`; `rescheduleBooking()`; `cancelBooking()`. |
| **Student – Payment** | `makePayment()` – record payment in `payments.txt`; `viewMyPayments()` – show history. |
| **Student – Rating** | `rateInstructor()` – write score + comment to `ratings.txt`; `viewInstructorRatings()` – display average score. |
| **Instructor – Schedule** | `addClass()` – add new class to `classes.txt`; `viewMyClasses()`; `searchClass()`; `updateClass()` (edit details). |
| **Instructor – Facility Issues** | `submitFacilityIssue()` – write to `facility_issues.txt` with status "pending"; `viewMyIssues()` – filter by own ID. |
| **Instructor – View Ratings** | `viewMyOverallRating()` – calculate average from `ratings.txt` for that instructor. |
| **Documentation** | Write the **Design Flowchart/Pseudocode** for your modules + provide 2-3 screenshots (positive + negative tests) + code snippets for the final PDF. |

* * *

### 👤 M5 – Facilities & Logistics (Rehan Ali)

**Files:** `facility_officer.c` + `facility_officer.h`

| Category | Specific Functions / Tasks |
| --- | --- |
| **Equipment Inventory** | `addEquipment()` – add to `equipment.txt`; `viewAllEquipment()`; `searchEquipmentByID/Name()`; `updateEquipment()` (quantity, threshold). |
| **Facility Problem Management** | `viewAllIssues()` – display from `facility_issues.txt`; `searchIssueByID/Location()`; `updateIssueStatus()` – change from "pending" to "fixed". |
| **Stock Reports** | `generateStockReport()` – filter by equipment ID or name, display current quantity and highlight low-stock items (below threshold). |
| **Documentation** | Write the **Design Flowchart/Pseudocode** for your modules + provide 2-3 screenshots (positive + negative tests) + code snippets for the final PDF. |

* * *

## 3\. Internal Project Timeline (FOR EXAMPLE ONLY)

| Phase | What to Deliver | Deadline (Set by you) |
| --- | --- | --- |
| **Phase 1: Design** | Each member submits their module's **flowchart OR pseudocode** (draft) for review. | *Week 1* |
| **Phase 2: Core Coding** | All `load_xxx()` / `save_xxx()` files ready (M1). Each member completes basic CRUD for their modules. | *Week 2* |
| **Phase 3: Integration** | Merge all `.c` files. M1 integrates and tests full system flow. All members fix bugs in their own modules. | *Week 3* |
| **Phase 4: Testing & Screenshots** | Each member captures 2 positive + 2 negative test screenshots for their features. Write module explanations. | *Week 4* |
| **Phase 5: Documentation Merge** | Everyone sends their sections to M1 (you). You compile the final PDF, TOC, Introduction, Conclusion, Workload Matrix. | *Week 5 (final)* |
| **Phase 6: Demo Video** | Record final system walkthrough (5-10 mins). All members should speak for their modules. | *2 days before deadline* |