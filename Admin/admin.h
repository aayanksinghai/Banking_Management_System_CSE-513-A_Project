#ifndef ADMIN_H
#define ADMIN_H

#include "../Customer/customer.h" 
#include "../Employee/employee.h" 
#include "../Manager/manager.h"

void handle_admin_login(int sock);
int _admin_load_customers_from_fd(int fd, int sock);
int _admin_save_customers_to_fd(int fd);
int _admin_load_managers_from_fd(int fd);
int _admin_save_managers_to_fd(int fd);
void save_new_customer(Customer new_customer);
void save_new_employee(Employee new_employee);
void add_customer(int sock);
void remove_customer(int sock);
void view_all_customer(int sock);
int is_customer_exists(const char *username, const char *password);
int is_employee_exists(const char *username, const char *password);
void add_BankEmp(int sock);
void change_admin_password(int sock);
void modify_user_details(int sock);
void manage_user_roles(int sock);
void remove_BankEmp(int sock);
void add_Manager(int sock);      
void remove_Manager(int sock);
int authenticate_admin(const char* username, const char* password);
void modify_manager_details(int sock);

#endif
