/* =====================================================================
 * SDAMS - Self Defence Academy Management System
 * common.h - shared symbolic constants, enumerations and structures
 *
 * Owner : M1 - Du Haoze (Lead Architect & Integrator)
 * Subject: CT018-3-1-ICP
 *
 * Every record of every entity lives in ONE line of ONE text file.
 * Fields are separated by '|' (pipe).  Record formats:
 *
 *   data/admins.txt          ID|Name|Password|Status
 *   data/instructors.txt     ID|Name|Role|Contact|Status|AvgRating
 *   data/students.txt        ID|Name|Contact|Status
 *   data/classes.txt         ClassID|InstructorID|MartialArt|DateTime|Capacity|
 *                            BookedCount|Status
 *   data/bookings.txt        BookingID|StudentID|ClassID|BookingDate|Status
 *   data/payments.txt        PaymentID|StudentID|ClassID|Amount|PaymentDate|Type
 *   data/ratings.txt         RatingID|StudentID|InstructorID|Score|Comment|Date
 *   data/facility_issues.txt IssueID|InstructorID|Location|Description|Status|
 *                            ReportedDate
 *   data/equipment.txt       EquipID|Name|Category|Quantity|Threshold|LastUpdated
 *   data/attendance.txt      ClassID|StudentID|Date|Status      (extra feature)
 * ===================================================================== */

#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

/* ---------------------------------------------------------------------
 * Symbolic constants (#define) - see assignment section 4 "Technical
 * Requirements: use symbolic constants where appropriate".
 * ------------------------------------------------------------------- */
#define DATA_DIR           "data"          /* folder holding every .txt file */
#define LOG_FILE           "system.log"    /* activity log (data/system.log)   */

#define MAX_RECORDS        500             /* capacity of each in-memory array */
#define ID_LEN             12              /* "C001"                            */
#define NAME_LEN           64
#define CONTACT_LEN        32
#define PASS_LEN           32
#define SHORT_LEN          24              /* status / category / martial art   */
#define TEXT_LEN           160             /* description / comment / location  */
#define DATETIME_LEN       24              /* "2026-10-05 18:00"                */
#define LINE_BUF_SIZE      1024

/* Canonical data file names (single source of truth for file I/O) */
#define ADMIN_FILE         "admins.txt"
#define INSTRUCTOR_FILE    "instructors.txt"
#define STUDENT_FILE       "students.txt"
#define CLASS_FILE         "classes.txt"
#define BOOKING_FILE       "bookings.txt"
#define PAYMENT_FILE       "payments.txt"
#define RATING_FILE        "ratings.txt"
#define ISSUE_FILE         "facility_issues.txt"
#define EQUIPMENT_FILE     "equipment.txt"
#define ATTENDANCE_FILE    "attendance.txt"

/* Business rules shared by every module */
#define DEFAULT_PASSWORD   "123456"   /* assumption: password of student/instructor */
#define MAX_LOGIN_ATTEMPTS 3
#define LATE_CANCEL_HOURS  5          /* cancel < 5h before class => RM10 penalty   */
#define LATE_CANCEL_FEE    10.00
#define MAX_RATING         5
#define MAX_EQUIP_QTY      10000
#define MAX_CAPACITY       200
#define MAX_PAYMENT        100000.00

/* ---------------------------------------------------------------------
 * Enumerations - the assignment lists enum as an "extra" feature.
 * ------------------------------------------------------------------- */
typedef enum { ST_INACTIVE = 0, ST_ACTIVE = 1 } Status;
typedef enum { BK_BOOKED = 0, BK_CANCELLED = 1, BK_ATTENDED = 2 } BookingStatus;
typedef enum { PAY_FEE = 0, PAY_PENALTY = 1 } PaymentType;
typedef enum { IS_PENDING = 0, IS_FIXED = 1 } IssueStatus;
typedef enum { AT_ABSENT = 0, AT_PRESENT = 1 } AttendanceStatus;
typedef enum {
    ROLE_NONE = 0,
    ROLE_MANAGER,
    ROLE_ADMIN,
    ROLE_STUDENT,
    ROLE_INSTRUCTOR,
    ROLE_FACILITY
} Role;

/* ---------------------------------------------------------------------
 * Structures - one struct per entity described in README.md
 * ------------------------------------------------------------------- */

/* admins.txt - staff accounts (manager / administrator / facility officer) */
typedef struct {
    char   id[ID_LEN];
    char   name[NAME_LEN];
    char   password[PASS_LEN];
    Status status;
} UserAccount;

/* instructors.txt */
typedef struct {
    char   id[ID_LEN];
    char   name[NAME_LEN];
    char   role[SHORT_LEN];        /* speciality, e.g. "Karate"       */
    char   contact[CONTACT_LEN];
    Status status;
    double avg_rating;             /* recalculated whenever a rating is added */
} Instructor;

/* students.txt */
typedef struct {
    char   id[ID_LEN];
    char   name[NAME_LEN];
    char   contact[CONTACT_LEN];
    Status status;
} Student;

/* classes.txt */
typedef struct {
    char   class_id[ID_LEN];
    char   instructor_id[ID_LEN];
    char   martial_art[SHORT_LEN];
    char   datetime[DATETIME_LEN]; /* "YYYY-MM-DD HH:MM" */
    int    capacity;
    int    booked_count;
    Status status;
} ClassRecord;

/* bookings.txt */
typedef struct {
    char          booking_id[ID_LEN];
    char          student_id[ID_LEN];
    char          class_id[ID_LEN];
    char          booking_date[DATETIME_LEN];
    BookingStatus status;
} Booking;

/* payments.txt  (Type distinguishes a class fee from a late-cancel penalty) */
typedef struct {
    char        payment_id[ID_LEN];
    char        student_id[ID_LEN];
    char        class_id[ID_LEN];
    double      amount;
    char        payment_date[SHORT_LEN];
    PaymentType type;
} Payment;

/* ratings.txt */
typedef struct {
    char rating_id[ID_LEN];
    char student_id[ID_LEN];
    char instructor_id[ID_LEN];
    int  score;                    /* 1 .. MAX_RATING */
    char comment[TEXT_LEN];
    char date[SHORT_LEN];
} Rating;

/* facility_issues.txt */
typedef struct {
    char        issue_id[ID_LEN];
    char        instructor_id[ID_LEN];
    char        location[TEXT_LEN];
    char        description[TEXT_LEN];
    IssueStatus status;
    char        reported_date[SHORT_LEN];
} FacilityIssue;

/* equipment.txt */
typedef struct {
    char equip_id[ID_LEN];
    char name[NAME_LEN];
    char category[SHORT_LEN];
    int  quantity;
    int  threshold;                /* quantity <= threshold => low stock warning */
    char last_updated[DATETIME_LEN];
} Equipment;

/* attendance.txt - extra feature ported from the previous FitZone project */
typedef struct {
    char             class_id[ID_LEN];
    char             student_id[ID_LEN];
    char             date[SHORT_LEN];
    AttendanceStatus status;
} Attendance;

/* ---------------------------------------------------------------------
 * Fixed-capacity collections.  Arrays (not linked lists) keep the code
 * portable ANSI C and easy to explain in the report; record limits are
 * validated at the point of use as required by AGENTS.md.
 * ------------------------------------------------------------------- */
typedef struct { UserAccount   items[MAX_RECORDS]; size_t count; } AccountList;
typedef struct { Instructor    items[MAX_RECORDS]; size_t count; } InstructorList;
typedef struct { Student       items[MAX_RECORDS]; size_t count; } StudentList;
typedef struct { ClassRecord   items[MAX_RECORDS]; size_t count; } ClassList;
typedef struct { Booking       items[MAX_RECORDS]; size_t count; } BookingList;
typedef struct { Payment       items[MAX_RECORDS]; size_t count; } PaymentList;
typedef struct { Rating        items[MAX_RECORDS]; size_t count; } RatingList;
typedef struct { FacilityIssue items[MAX_RECORDS]; size_t count; } IssueList;
typedef struct { Equipment     items[MAX_RECORDS]; size_t count; } EquipmentList;
typedef struct { Attendance    items[MAX_RECORDS]; size_t count; } AttendanceList;

/* The single in-memory database of the whole application.  It is defined
 * (allocated) in file_handler.c - the only module allowed to touch files. */
typedef struct {
    AccountList   admins;
    InstructorList instructors;
    StudentList   students;
    ClassList     classes;
    BookingList   bookings;
    PaymentList   payments;
    RatingList    ratings;
    IssueList     issues;
    EquipmentList equipment;
    AttendanceList attendance;
} DataStore;

extern DataStore g_data;

#endif /* COMMON_H */
