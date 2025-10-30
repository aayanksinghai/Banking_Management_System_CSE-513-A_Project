#ifndef MANAGER_H
#define MANAGER_H

#include "../Customer/customer.h" 

void handle_manager_login(int sock);
void Activate_Customer_Acc(int sock);
void Deactivate_Customer_Acc(int sock);
void Assign_LoanApp_to_Employee(int sock);
void Review_Customer_feedback(int sock);

#endif