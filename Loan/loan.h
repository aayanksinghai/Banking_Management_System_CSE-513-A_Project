#ifndef LOAN_H
#define LOAN_H

typedef struct {
    int loan_id;
    char customer_username[50];
    float loan_amount;
    char loan_purpose[100];
    float monthly_income;
    char employment_status[50];
    char contact_info[50];
} Loan;

#endif