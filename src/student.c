/* =====================================================================
 * SDAMS - student.c
 * Owner: M4 - Lhaksam Tiempey Geltsan (Operations & Training Lead)
 *
 * Ported from the previous (FitZone) project's booking.py + parts of
 * member.py and accountant.py:
 *
 *   view_classes()     -> view_available_classes()
 *   book_class()       -> book_class()
 *   cancel_booking()   -> cancel_booking()   (keeps the RM10 late-cancel
 *                        penalty that triggered when a booking was
 *                        cancelled less than 5 hours before the class)
 *   reschedule()       -> reschedule_booking()
 *   view_history()     -> view_my_bookings()
 *   record_payment()   -> make_payment()      income_report() -> view_my_payments()
 *   (new)              -> rate_instructor() / view_instructor_ratings()
 *
 * The Python version asked the member to retype their name; here the
 * logged-in session supplies the student ID, which removes a whole class
 * of input errors.  No fopen() in this file - everything is persisted
 * through the file handler.
 * ===================================================================== */

#include "../include/student.h"
#include "../include/auth.h"
#include "../include/utils.h"
#include "../include/reports.h"
#include "../include/file_handler.h"

#include <stdio.h>
#include <string.h>

/* =====================================================================
 * 0. Shared lookups and ID helpers
 * ===================================================================== */

int find_class_index(const char *class_id) {
    size_t i;
    if (class_id == NULL) return -1;
    for (i = 0; i < g_data.classes.count; i++) {
        if (equals_ci(g_data.classes.items[i].class_id, class_id)) return (int)i;
    }
    return -1;
}

static void next_id_for(const char *prefix, char (*ids)[ID_LEN], size_t count,
                        char *out, size_t size) {
    size_t i;
    int    best = 0;

    for (i = 0; i < count; i++) {
        int n = id_number(ids[i], prefix);
        if (n > best) best = n;
    }
    make_id(prefix, best + 1, out, size);
}

static void collect_booking_ids(char (*ids)[ID_LEN]) {
    size_t i;
    for (i = 0; i < g_data.bookings.count; i++) snprintf(ids[i], ID_LEN, "%s", g_data.bookings.items[i].booking_id);
}

static void collect_payment_ids(char (*ids)[ID_LEN]) {
    size_t i;
    for (i = 0; i < g_data.payments.count; i++) snprintf(ids[i], ID_LEN, "%s", g_data.payments.items[i].payment_id);
}

static void collect_rating_ids(char (*ids)[ID_LEN]) {
    size_t i;
    for (i = 0; i < g_data.ratings.count; i++) snprintf(ids[i], ID_LEN, "%s", g_data.ratings.items[i].rating_id);
}

/* A student may hold at most one active booking for the same class. */
static int has_active_booking(const char *student_id, const char *class_id) {
    size_t i;
    for (i = 0; i < g_data.bookings.count; i++) {
        const Booking *b = &g_data.bookings.items[i];
        if (equals_ci(b->student_id, student_id) &&
            equals_ci(b->class_id, class_id) &&
            b->status == BK_BOOKED) {
            return 1;
        }
    }
    return 0;
}

static int find_booking_index(const char *booking_id) {
    size_t i;
    if (booking_id == NULL) return -1;
    for (i = 0; i < g_data.bookings.count; i++) {
        if (equals_ci(g_data.bookings.items[i].booking_id, booking_id)) return (int)i;
    }
    return -1;
}

/* Recalculate and store an instructor's average rating. */
static void recalculate_instructor_rating(const char *instructor_id) {
    int    index = find_instructor_index(instructor_id);
    size_t i;
    int    count = 0;
    double sum   = 0.0;

    if (index < 0) return;
    for (i = 0; i < g_data.ratings.count; i++) {
        if (equals_ci(g_data.ratings.items[i].instructor_id, instructor_id)) {
            sum += g_data.ratings.items[i].score;
            count++;
        }
    }
    g_data.instructors.items[index].avg_rating = (count > 0) ? (sum / count) : 0.0;
    save_instructors();
}

/* =====================================================================
 * 1. View available classes
 * ===================================================================== */

void view_available_classes(void) {
    size_t i;
    int    shown = 0;

    print_heading("AVAILABLE CLASSES");
    printf("%-6s %-14s %-20s %-18s %-9s %s\n", "ID", "MARTIAL ART", "INSTRUCTOR",
           "DATE & TIME", "SLOTS", "STATUS");
    print_table_border(84);

    for (i = 0; i < g_data.classes.count; i++) {
        const ClassRecord *c = &g_data.classes.items[i];
        int   instructor_index = find_instructor_index(c->instructor_id);
        const char *instructor_name = (instructor_index >= 0)
                                      ? g_data.instructors.items[instructor_index].name
                                      : "(unknown)";
        int   free_slots = c->capacity - c->booked_count;

        if (c->status != ST_ACTIVE) continue;      /* only active classes are bookable */
        printf("%-6s %-14s %-20s %-18s %-9d %s\n", c->class_id, c->martial_art,
               instructor_name, c->datetime, free_slots,
               (free_slots > 0) ? "open" : "FULL");
        shown++;
    }
    if (shown == 0) printf("Prompt: No classes available.\n");   /* same wording as Python */
    print_table_border(84);
    printf("Total: %d bookable class(es)\n", shown);
}

/* =====================================================================
 * 2. Book a class  (port of booking.book_class())
 * ===================================================================== */

int book_class(void) {
    char  class_id[ID_LEN];
    int   index;
    Booking booking;

    print_heading("BOOK A CLASS");
    if (read_line("Class ID (Enter = list classes first): ", class_id, sizeof class_id) != 0) return 0;
    trim_whitespace(class_id);
    if (class_id[0] == '\0') {
        view_available_classes();
        if (read_line("Class ID: ", class_id, sizeof class_id) != 0) return 0;
        trim_whitespace(class_id);
    }

    if (!validate_id(class_id, "C")) {
        printf("ERROR: Class ID must look like C001.\n");
        return 0;
    }

    index = find_class_index(class_id);
    if (index < 0) {
        printf("ERROR: Class '%s' does not exist.\n", class_id);   /* Python: "Class not available!" */
        return 0;
    }
    if (g_data.classes.items[index].status != ST_ACTIVE) {
        printf("ERROR: Class %s is not active.\n", class_id);
        return 0;
    }
    if (!is_future_datetime(g_data.classes.items[index].datetime)) {
        printf("ERROR: Class %s already started (%s).\n", class_id,
               g_data.classes.items[index].datetime);
        return 0;
    }
    if (g_data.classes.items[index].booked_count >= g_data.classes.items[index].capacity) {
        printf("ERROR: Class %s is full (%d/%d).\n", class_id,
               g_data.classes.items[index].booked_count, g_data.classes.items[index].capacity);
        return 0;
    }
    if (has_active_booking(g_session.user_id, class_id)) {
        printf("ERROR: You already have an active booking for class %s.\n", class_id);
        return 0;
    }

    {
        char ids[MAX_RECORDS][ID_LEN];
        collect_booking_ids(ids);
        next_id_for("B", ids, g_data.bookings.count, booking.booking_id, sizeof booking.booking_id);
    }
    snprintf(booking.student_id, sizeof booking.student_id, "%s", g_session.user_id);
    snprintf(booking.class_id, sizeof booking.class_id, "%s", class_id);
    current_datetime(booking.booking_date, sizeof booking.booking_date, 1);
    booking.status = BK_BOOKED;

    if (!confirm_action("Confirm this booking?")) {
        printf("Booking cancelled.\n");
        return 0;
    }

    g_data.bookings.items[g_data.bookings.count++] = booking;
    g_data.classes.items[index].booked_count++;

    if (save_bookings() != 0 || save_classes() != 0) {
        printf("ERROR: Could not save the booking.\n");
        return 0;
    }

    printf("Booking successful. Class %s on %s, booking ID %s.\n", class_id,
           g_data.classes.items[index].datetime, booking.booking_id);
    log_event("Booking %s: student %s booked class %s", booking.booking_id,
              g_session.user_id, class_id);
    return 1;
}

/* =====================================================================
 * 3. View my bookings
 * ===================================================================== */

void view_my_bookings(void) {
    size_t i;
    int    found = 0;

    print_heading("MY BOOKINGS");
    printf("%-6s %-8s %-14s %-18s %-11s %s\n", "ID", "CLASS", "MARTIAL ART",
           "CLASS DATE", "BOOKED ON", "STATUS");
    print_table_border(88);

    for (i = 0; i < g_data.bookings.count; i++) {
        const Booking *b = &g_data.bookings.items[i];
        int class_index;
        const ClassRecord *c;

        if (!equals_ci(b->student_id, g_session.user_id)) continue;
        class_index = find_class_index(b->class_id);
        c = (class_index >= 0) ? &g_data.classes.items[class_index] : NULL;

        printf("%-6s %-8s %-14s %-18s %-11s %s\n", b->booking_id, b->class_id,
               (c != NULL) ? c->martial_art : "(removed)",
               (c != NULL) ? c->datetime : "-",
               b->booking_date, booking_status_to_string(b->status));
        found++;
    }
    if (found == 0) printf("(you have no bookings yet)\n");
    print_table_border(88);
    printf("Total: %d booking(s)\n", found);
    log_event("Student %s viewed their bookings", g_session.user_id);
}

/* =====================================================================
 * 4. Reschedule  (port of booking.reschedule())
 * ===================================================================== */

int reschedule_booking(void) {
    char booking_id[ID_LEN];
    char new_class_id[ID_LEN];
    int  booking_index, old_class_index, new_class_index;

    print_heading("RESCHEDULE A BOOKING");
    if (read_line("Booking ID: ", booking_id, sizeof booking_id) != 0) return 0;
    trim_whitespace(booking_id);

    booking_index = find_booking_index(booking_id);
    if (booking_index < 0) {
        printf("ERROR: Booking '%s' does not exist.\n", booking_id);
        return 0;
    }
    if (!equals_ci(g_data.bookings.items[booking_index].student_id, g_session.user_id)) {
        printf("ERROR: Booking %s does not belong to you.\n", booking_id);
        return 0;
    }
    if (g_data.bookings.items[booking_index].status != BK_BOOKED) {
        printf("ERROR: Booking %s is %s and cannot be rescheduled.\n", booking_id,
               booking_status_to_string(g_data.bookings.items[booking_index].status));
        return 0;
    }

    old_class_index = find_class_index(g_data.bookings.items[booking_index].class_id);
    if (old_class_index < 0) {
        printf("ERROR: The original class no longer exists.\n");
        return 0;
    }

    view_available_classes();
    if (read_line("New class ID: ", new_class_id, sizeof new_class_id) != 0) return 0;
    trim_whitespace(new_class_id);

    new_class_index = find_class_index(new_class_id);
    if (new_class_index < 0) {
        printf("ERROR: Class '%s' does not exist.\n", new_class_id);
        return 0;
    }
    if (new_class_index == old_class_index) {
        printf("ERROR: The booking is already for class %s.\n", new_class_id);
        return 0;
    }
    if (g_data.classes.items[new_class_index].status != ST_ACTIVE) {
        printf("ERROR: Class %s is not active.\n", new_class_id);
        return 0;
    }
    if (!is_future_datetime(g_data.classes.items[new_class_index].datetime)) {
        printf("ERROR: Class %s already started.\n", new_class_id);
        return 0;
    }
    if (g_data.classes.items[new_class_index].booked_count >=
        g_data.classes.items[new_class_index].capacity) {
        printf("ERROR: Class %s is full.\n", new_class_id);
        return 0;
    }
    if (has_active_booking(g_session.user_id, new_class_id)) {
        printf("ERROR: You already booked class %s.\n", new_class_id);
        return 0;
    }

    if (!confirm_action("Confirm the reschedule?")) {
        printf("Cancelled.\n");
        return 0;
    }

    /* Move one seat from the old class to the new one. */
    if (g_data.classes.items[old_class_index].booked_count > 0) {
        g_data.classes.items[old_class_index].booked_count--;
    }
    g_data.classes.items[new_class_index].booked_count++;
    snprintf(g_data.bookings.items[booking_index].class_id, ID_LEN, "%s", new_class_id);

    save_bookings();
    save_classes();
    printf("Rescheduled %s from %s to %s.\n", booking_id,
           g_data.classes.items[old_class_index].class_id, new_class_id);
    log_event("Reschedule %s to class %s", booking_id, new_class_id);
    return 1;
}

/* =====================================================================
 * 5. Cancel  (port of booking.cancel_booking(), including the RM10
 *    penalty when the cancellation happens less than 5 hours before the
 *    class - the Python rule datetime.now() - t < timedelta(hours=5)).
 * ===================================================================== */

int cancel_booking(void) {
    char        booking_id[ID_LEN];
    int         booking_index, class_index;
    double      hours_left;
    int         penalty = 0;
    Payment     payment;
    char        ids[MAX_RECORDS][ID_LEN];

    print_heading("CANCEL A BOOKING");
    if (read_line("Booking ID: ", booking_id, sizeof booking_id) != 0) return 0;
    trim_whitespace(booking_id);

    booking_index = find_booking_index(booking_id);
    if (booking_index < 0) {
        printf("ERROR: Booking '%s' does not exist.\n", booking_id);
        return 0;
    }
    if (!equals_ci(g_data.bookings.items[booking_index].student_id, g_session.user_id)) {
        printf("ERROR: Booking %s does not belong to you.\n", booking_id);
        return 0;
    }
    if (g_data.bookings.items[booking_index].status != BK_BOOKED) {
        printf("ERROR: Booking %s is already %s.\n", booking_id,
               booking_status_to_string(g_data.bookings.items[booking_index].status));
        return 0;
    }

    class_index = find_class_index(g_data.bookings.items[booking_index].class_id);
    hours_left  = 0.0;
    if (class_index >= 0) {
        char now[DATETIME_LEN];
        current_datetime(now, sizeof now, 1);
        hours_left = hours_between(now, g_data.classes.items[class_index].datetime);
        if (hours_left < (double)LATE_CANCEL_HOURS) penalty = 1;
    }

    if (penalty) {
        printf("NOTE: Class starts in %.1f hour(s). A late cancellation fee of RM%.2f applies.\n",
               hours_left, LATE_CANCEL_FEE);
    }

    if (!confirm_action("Confirm the cancellation?")) {
        printf("Cancelled - your booking was kept.\n");
        return 0;
    }

    g_data.bookings.items[booking_index].status = BK_CANCELLED;
    if (class_index >= 0 && g_data.classes.items[class_index].booked_count > 0) {
        g_data.classes.items[class_index].booked_count--;
    }
    save_bookings();
    save_classes();

    if (penalty) {
        collect_payment_ids(ids);
        next_id_for("P", ids, g_data.payments.count, payment.payment_id, sizeof payment.payment_id);
        snprintf(payment.student_id, sizeof payment.student_id, "%s", g_session.user_id);
        snprintf(payment.class_id, sizeof payment.class_id, "%s",
                 g_data.bookings.items[booking_index].class_id);
        payment.amount = LATE_CANCEL_FEE;
        current_datetime(payment.payment_date, sizeof payment.payment_date, 0);
        payment.type = PAY_PENALTY;
        g_data.payments.items[g_data.payments.count++] = payment;
        save_payments();
        printf("Late cancellation penalty RM%.2f recorded as %s.\n",
               LATE_CANCEL_FEE, payment.payment_id);
    }

    printf("Cancelled %s for %s.\n", booking_id, g_session.user_id);
    log_event("Cancel %s for student %s (penalty=%s)", booking_id, g_session.user_id,
              penalty ? "yes" : "no");
    return 1;
}

/* =====================================================================
 * 6. Payments  (port of accountant.record_payment())
 * ===================================================================== */

int make_payment(void) {
    char    booking_id[ID_LEN];
    char    class_id[ID_LEN];
    int     booking_index, class_index;
    double  amount = 0.0;
    Payment payment;
    char    ids[MAX_RECORDS][ID_LEN];

    print_heading("MAKE A PAYMENT");

    if (g_data.bookings.count == 0) {
        printf("ERROR: You have no booking to pay for. Book a class first.\n");
        return 0;
    }

    view_my_bookings();
    if (read_line("Booking ID to pay for: ", booking_id, sizeof booking_id) != 0) return 0;
    trim_whitespace(booking_id);

    booking_index = find_booking_index(booking_id);
    if (booking_index < 0) {
        printf("ERROR: Booking '%s' does not exist.\n", booking_id);
        return 0;
    }
    if (!equals_ci(g_data.bookings.items[booking_index].student_id, g_session.user_id)) {
        printf("ERROR: Booking %s does not belong to you.\n", booking_id);
        return 0;
    }
    if (g_data.bookings.items[booking_index].status != BK_BOOKED &&
        g_data.bookings.items[booking_index].status != BK_ATTENDED) {
        printf("ERROR: Booking %s is %s - nothing to pay.\n", booking_id,
               booking_status_to_string(g_data.bookings.items[booking_index].status));
        return 0;
    }

    snprintf(class_id, sizeof class_id, "%s", g_data.bookings.items[booking_index].class_id);
    class_index = find_class_index(class_id);

    if (read_double("Amount to pay (RM): ", 0.01, MAX_PAYMENT, &amount) != 0) return 0;

    if (!confirm_action("Confirm this payment?")) {
        printf("Payment cancelled.\n");
        return 0;
    }

    collect_payment_ids(ids);
    next_id_for("P", ids, g_data.payments.count, payment.payment_id, sizeof payment.payment_id);
    snprintf(payment.student_id, sizeof payment.student_id, "%s", g_session.user_id);
    snprintf(payment.class_id, sizeof payment.class_id, "%s", class_id);
    payment.amount = amount;
    current_datetime(payment.payment_date, sizeof payment.payment_date, 0);
    payment.type = PAY_FEE;

    g_data.payments.items[g_data.payments.count++] = payment;
    if (save_payments() != 0) {
        printf("ERROR: Could not write %s.\n", PAYMENT_FILE);
        g_data.payments.count--;
        return 0;
    }

    printf("Payment %.2f recorded for %s (class %s, receipt %s) on %s.\n", amount,
           g_session.user_id, class_id,
           payment.payment_id, payment.payment_date);
    if (class_index >= 0) {
        printf("Class: %s with instructor %s.\n", g_data.classes.items[class_index].martial_art,
               g_data.classes.items[class_index].instructor_id);
    }
    log_event("Payment %.2f recorded for student %s (receipt %s)", amount,
              g_session.user_id, payment.payment_id);
    return 1;
}

void view_my_payments(void) {
    size_t i;
    int    found = 0;
    double fees = 0.0, penalties = 0.0;

    print_heading("MY PAYMENT HISTORY");
    printf("%-6s %-8s %-9s %10s %s\n", "PAYID", "CLASS", "TYPE", "AMOUNT", "DATE");
    print_table_border(54);

    for (i = 0; i < g_data.payments.count; i++) {
        const Payment *p = &g_data.payments.items[i];
        if (!equals_ci(p->student_id, g_session.user_id)) continue;
        printf("%-6s %-8s %-9s %10.2f %s\n", p->payment_id, p->class_id,
               payment_type_to_string(p->type), p->amount, p->payment_date);
        found++;
        if (p->type == PAY_FEE) fees += p->amount;
        else                    penalties += p->amount;
    }
    if (found == 0) printf("(no payment records)\n");
    print_table_border(54);
    printf("Class fees: RM%.2f   Penalties: RM%.2f   Total paid: RM%.2f\n",
           fees, penalties, fees + penalties);
    log_event("Student %s viewed their payment history", g_session.user_id);
}

/* =====================================================================
 * 7. Ratings
 * ===================================================================== */

int rate_instructor(void) {
    char   instructor_id[ID_LEN];
    char   comment[TEXT_LEN];
    int    index, score = 0;
    size_t i;
    Rating rating;
    char   ids[MAX_RECORDS][ID_LEN];

    print_heading("RATE AN INSTRUCTOR");
    if (read_line("Instructor ID: ", instructor_id, sizeof instructor_id) != 0) return 0;
    trim_whitespace(instructor_id);

    index = find_instructor_index(instructor_id);
    if (index < 0) {
        printf("ERROR: Instructor '%s' does not exist.\n", instructor_id);
        return 0;
    }
    if (g_data.instructors.items[index].status != ST_ACTIVE) {
        printf("ERROR: Instructor %s is not active.\n", instructor_id);
        return 0;
    }

    /* One rating per instructor per student keeps the average meaningful. */
    for (i = 0; i < g_data.ratings.count; i++) {
        if (equals_ci(g_data.ratings.items[i].student_id, g_session.user_id) &&
            equals_ci(g_data.ratings.items[i].instructor_id, instructor_id)) {
            printf("ERROR: You already rated %s (score %d on %s).\n", instructor_id,
                   g_data.ratings.items[i].score, g_data.ratings.items[i].date);
            return 0;
        }
    }

    printf("Instructor: %s (%s, %s)\n", g_data.instructors.items[index].name,
           g_data.instructors.items[index].role, g_data.instructors.items[index].contact);

    if (read_int("Score (1 = poor ... 5 = excellent): ", 1, MAX_RATING, &score) != 0) return 0;
    if (read_line("Comment (optional): ", comment, sizeof comment) != 0) return 0;
    trim_whitespace(comment);
    if (comment[0] == '\0') snprintf(comment, sizeof comment, "-");

    collect_rating_ids(ids);
    next_id_for("R", ids, g_data.ratings.count, rating.rating_id, sizeof rating.rating_id);
    snprintf(rating.student_id, sizeof rating.student_id, "%s", g_session.user_id);
    snprintf(rating.instructor_id, sizeof rating.instructor_id, "%s", instructor_id);
    rating.score = score;
    snprintf(rating.comment, sizeof rating.comment, "%s", comment);
    current_datetime(rating.date, sizeof rating.date, 0);

    g_data.ratings.items[g_data.ratings.count++] = rating;
    if (save_ratings() != 0) {
        printf("ERROR: Could not write %s.\n", RATING_FILE);
        g_data.ratings.count--;
        return 0;
    }

    recalculate_instructor_rating(instructor_id);
    printf("Thank you. Rating %s saved (score %d). Instructor average is now %.2f.\n",
           rating.rating_id, score, g_data.instructors.items[index].avg_rating);
    log_event("Student %s rated instructor %s with %d", g_session.user_id, instructor_id, score);
    return 1;
}

void view_instructor_ratings(void) {
    char   instructor_id[ID_LEN];
    size_t i;
    int    index, count = 0;
    double sum = 0.0;

    print_heading("INSTRUCTOR RATINGS");
    if (read_line("Instructor ID: ", instructor_id, sizeof instructor_id) != 0) return;
    trim_whitespace(instructor_id);

    index = find_instructor_index(instructor_id);
    if (index < 0) {
        printf("ERROR: Instructor '%s' does not exist.\n", instructor_id);
        return;
    }

    printf("\n%-6s %-8s %-6s %-18s %s\n", "RATID", "STUDENT", "SCORE", "DATE", "COMMENT");
    print_table_border(72);
    for (i = 0; i < g_data.ratings.count; i++) {
        const Rating *r = &g_data.ratings.items[i];
        if (!equals_ci(r->instructor_id, instructor_id)) continue;
        printf("%-6s %-8s %-6d %-18s %s\n", r->rating_id, r->student_id, r->score,
               r->date, r->comment);
        sum += r->score;
        count++;
    }
    if (count == 0) printf("(no ratings yet)\n");
    print_table_border(72);
    printf("Instructor: %s (%s)\n", g_data.instructors.items[index].name,
           g_data.instructors.items[index].role);
    printf("Ratings: %d   Average: %.2f / %d\n", count,
           (count > 0) ? sum / count : 0.0, MAX_RATING);
    log_event("Student %s viewed ratings of instructor %s", g_session.user_id, instructor_id);
}

/* =====================================================================
 * 8. Student sub-menu
 * ===================================================================== */

void student_menu(void) {
    char choice;

    while (1) {
        printf("\n[Student Subsystem Menu] %s - %s\n", g_session.user_name, g_session.user_id);
        printf("1. View available classes\n");
        printf("2. Book a class\n");
        printf("3. View my bookings\n");
        printf("4. Reschedule a booking\n");
        printf("5. Cancel a booking\n");
        printf("6. Make a payment\n");
        printf("7. View my payment history\n");
        printf("8. Rate an instructor\n");
        printf("9. View instructor ratings\n");
        printf("0. Logout and back\n");

        if (read_choice("Enter choice: ", "1234567890", &choice) != 0) return;

        switch (choice) {
            case '1': view_available_classes(); pause_screen(); break;
            case '2': book_class(); pause_screen(); break;
            case '3': view_my_bookings(); pause_screen(); break;
            case '4': reschedule_booking(); pause_screen(); break;
            case '5': cancel_booking(); pause_screen(); break;
            case '6': make_payment(); pause_screen(); break;
            case '7': view_my_payments(); pause_screen(); break;
            case '8': rate_instructor(); pause_screen(); break;
            case '9': view_instructor_ratings(); pause_screen(); break;
            case '0':
                auth_logout();
                printf("Logged out of the student subsystem.\n");
                return;
            default:
                printf("Invalid option.\n");
        }
    }
}
