#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <stdio.h>

typedef struct {
    char username[50];
    char password[50];
    float balance;
    int id;
    int is_active;
} Customer;


extern Customer customers[100];
extern int customer_count;
extern Customer new_customer;


void view_account_balance(const char *username, int sock);
void deposit_money(const char *username, int sock);
void withdraw_money(const char *username, int sock);
void transfer_funds(const char *username, int sock);
void handle_customer_login(int sock);
int authenticate_customer(const char *username, const char *password);
void apply_for_loan(const char *username, int sock);  
void change_password(const char *username, int sock); 
void add_feedback(const char *username, int sock); 
void view_transaction_history(const char *username, int sock); 
void log_transaction(const char *username, const char *type, float amount);


#endif
