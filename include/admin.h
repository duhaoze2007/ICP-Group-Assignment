/* =====================================================================
 * SDAMS - admin.h
 * Owner: M3 - Htoo Aung Htet (Administrative Control Lead)
 * ===================================================================== */

#ifndef ADMIN_H
#define ADMIN_H

#include "../include/common.h"

/* Administrator sub-menu (login is performed by main.c before this runs). */
void admin_menu(void);

/* Instructor management (instructors.txt) */
int  add_instructor(void);
void view_all_instructors(void);
void search_instructor(void);
int  update_instructor(void);
int  update_instructor_status(void);
int  delete_instructor(void);

/* Student management (students.txt) */
int  add_student(void);
void view_all_students(void);
void search_student(void);
int  update_student(void);
int  update_student_status(void);
int  delete_student(void);

/* Payment records (read-only view of payments.txt) */
void view_student_payments(void);

#endif /* ADMIN_H */
