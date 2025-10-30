#ifndef ADMIN_H
#define ADMIN_H

#include "../Customer/customer.h" 
#include "../Employee/employee.h" 

void handle_admin_login(int sock);
void load_customers(int sock);
void save_customers();
void save_new_customer(Customer new_customer);
void save_new_employee(Employee new_employee);
void add_customer(int sock);
void remove_customer(int sock);
void view_all_customer(int sock);
int is_customer_exists(const char *username, const char *password);
int is_employee_exists(const char *username, const char *password);
void add_BankEmp(int sock);
void change_admin_password(int sock);

#endif
