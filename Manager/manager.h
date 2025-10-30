#ifndef MANAGER_H
#define MANAGER_H

#include "../Customer/customer.h" 

typedef struct {
    char username[50];
    char password[50];
    int id;
} Manager;

void handle_manager_login(int sock);
void Activate_Customer_Acc(int sock);
void Deactivate_Customer_Acc(int sock);
void Assign_LoanApp_to_Employee(int sock);
void Review_Customer_feedback(int sock);
void change_manager_password(int sock, const char* username);

#endif