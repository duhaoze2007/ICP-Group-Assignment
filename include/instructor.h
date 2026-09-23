/* =====================================================================
 * SDAMS - instructor.h
 * Owner: M4 - Lhaksam Tiempey Geltsan (Operations & Training Lead)
 * ===================================================================== */

#ifndef INSTRUCTOR_H
#define INSTRUCTOR_H

#include "../include/common.h"

/* Instructor sub-menu (login is performed by main.c before this runs). */
void instructor_menu(void);

/* Class schedule management (classes.txt) */
int  add_class(void);
void view_my_classes(void);
void search_class(void);
int  update_class(void);
int  toggle_class_status(void);

/* Facility issues reported by this instructor (facility_issues.txt) */
int  submit_facility_issue(void);
void view_my_issues(void);

/* Ratings received from students (ratings.txt) */
void view_my_overall_rating(void);

/* Extra features ported from the previous project's trainer.py */
int  mark_attendance(void);
void view_attendance_report(void);

#endif /* INSTRUCTOR_H */
