/* =====================================================================
 * SDAMS - auth.h
 * Owner: M2 - Justin Loo (Authentication & Management Lead)
 *
 * The previous project asked for a password at the entrance of every
 * subsystem (passwd(123456, 5)).  In SDAMS the login is proper: each
 * role enters an ID and a password, and the session is remembered in
 * g_session so that every role module knows who is logged in.
 * ===================================================================== */

#ifndef AUTH_H
#define AUTH_H

#include "../include/common.h"

/* The one logged-in user of the running session. */
typedef struct {
    Role role;
    char user_id[ID_LEN];
    char user_name[NAME_LEN];
    int  logged_in;
} Session;

extern Session g_session;

/* Ask for ID + password until successful or until MAX_LOGIN_ATTEMPTS is
 * reached.  Returns 1 when logged in, 0 when the login failed/cancelled. */
int auth_login(Role role);

void auth_logout(void);
int  auth_is_logged_in(void);
const Session *get_logged_in_user(void);

/* Lookup helpers shared with the manager/admin modules. */
int find_account_index(const char *id);         /* -1 when not found */
int find_student_index(const char *id);
int find_instructor_index(const char *id);

#endif /* AUTH_H */
