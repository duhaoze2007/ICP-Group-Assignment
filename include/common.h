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

typedef struct 
{
    char id[5];
    char name[50];
    char contact[13];
    char status[11];
}Student;

typedef struct
{
    char classID[5];
    char instructorID[5];
    char martialArt[50];
    char dateTime[17];
    unsigned int capacity;
    unsigned int bookedCount;
    char status[10];
}Class;

typedef struct 
{
    char bookingID[5];
    char studentID[5];
    char classID[5];
    char bookingDate[11];
    char status[12];
}Booking;

typedef struct 
{
    char paymentID[5];
    char studentID[5];
    char classID[5];
    char paymentDate[11];
    unsigned int ringgitCent;
}Payment;

typedef struct 
{
    char ratingID[5];
    char studentID[5];
    char instructorID[5];
    int score;
    char comment[901];
    char date[11];
}Rating;

typedef struct
{
    char issueID[5];
    char instructorID[5];
    char location[20];
    char description[90];
    char status[10];
    char reportedDate[11];
}FacilityIssue;

#endif
