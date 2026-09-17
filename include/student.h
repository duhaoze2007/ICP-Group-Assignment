/* =====================================================================
 * SDAMS - student.h
 * Owner: M4 - Lhaksam Tiempey Geltsan (Operations & Training Lead)
 * ===================================================================== */

#ifndef STUDENT_H
#define STUDENT_H

#include "../include/common.h"

/* Student sub-menu (login is performed by main.c before this runs). */
void student_menu(void);

/* Class booking (ported from the previous project's booking.py) */
void view_available_classes(void);
int  book_class(void);
void view_my_bookings(void);
int  reschedule_booking(void);
int  cancel_booking(void);

/* Payments and ratings */
int  make_payment(void);
void view_my_payments(void);
int  rate_instructor(void);
void view_instructor_ratings(void);

/* Shared lookup used by instructor.c as well */
int  find_class_index(const char *class_id);

#endif /* STUDENT_H */
