#include "session.h"
#include <stdio.h>

#define MAX_LOGGED_IN_USERS 200

// A struct to track a single logged-in user
typedef struct {
    int id;
    int role;
} LoggedInUser;

static LoggedInUser loggedInUsers[MAX_LOGGED_IN_USERS];
static int loggedInUserCount = 0;
static pthread_mutex_t sessionMutex;

// Initialize the mutex
void initSessionLock() {
    pthread_mutex_init(&sessionMutex, NULL);
}

// Checks if a user is already in our list
int isUserLoggedIn(int id, int role) {
    int found = 0;
    pthread_mutex_lock(&sessionMutex);
    for (int i = 0; i < loggedInUserCount; i++) {
        if (loggedInUsers[i].id == id && loggedInUsers[i].role == role) {
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&sessionMutex);
    return found;
}

// Adds a user to our list
void logUserIn(int id, int role) {
    pthread_mutex_lock(&sessionMutex);
    if (loggedInUserCount < MAX_LOGGED_IN_USERS) {
        loggedInUsers[loggedInUserCount].id = id;
        loggedInUsers[loggedInUserCount].role = role;
        loggedInUserCount++;
    }
    pthread_mutex_unlock(&sessionMutex);
}

// Removes a user from our list
void logUserOut(int id, int role) {
    pthread_mutex_lock(&sessionMutex);
    for (int i = 0; i < loggedInUserCount; i++) {
        if (loggedInUsers[i].id == id && loggedInUsers[i].role == role) {
            // Remove by swapping with the last element
            loggedInUsers[i] = loggedInUsers[loggedInUserCount - 1];
            loggedInUserCount--;
            break;
        }
    }
    pthread_mutex_unlock(&sessionMutex);
}