#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include "../Customer/customer.h" 

void view_all_customer(int sock);
void Process_LoanApp(int sock);
void add_customer(int sock);
void remove_customer(int sock);
void update_customer(int sock);
void handle_employee_login(int sock);
int authenticate_employee(const char* username, const char* password);

typedef struct {
    char username[50];
    char password[50];
    int id;
} Employee;

typedef struct {
    char customer_username[50];
    float loan_amount;
    char loan_purpose[100];
    float monthly_income;
    char employment_status[50];
    char contact_info[50];
} LoanApplication;

extern Employee emp;
extern Employee employees[100];
extern int emp_count;
extern Employee new_employee;

#endif