#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include "../Customer/customer.h" 
#include "../loan.h"

void view_all_customer(int sock);
void Process_LoanApp(int sock);
void add_customer(int sock);
void remove_customer(int sock);
void update_customer(int sock);
void handle_employee_login(int sock);
int authenticate_employee(const char* username, const char* password);
void change_employee_password(int sock, const char* username);
void view_customer_transactions(int sock);
void view_assigned_loans(int sock, int employee_id);

typedef struct {
    char username[50];
    char password[50];
    int id;
} Employee;

extern Employee emp;
extern Employee employees[100];
extern int emp_count;
extern Employee new_employee;

#endif