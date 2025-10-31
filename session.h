#ifndef SESSION_H
#define SESSION_H

#include <pthread.h>

// Role IDs to distinguish user types
#define ROLE_ADMIN 1
#define ROLE_MANAGER 2
#define ROLE_EMPLOYEE 3
#define ROLE_CUSTOMER 4

void initSessionLock();
int isUserLoggedIn(int id, int role);
void logUserIn(int id, int role);
void logUserOut(int id, int role);

#endif