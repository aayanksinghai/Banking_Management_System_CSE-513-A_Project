#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h> 
#include "employee.h"
#include "../Admin/admin.h"
#include "../Customer/customer.h"
#include "../Session/session.h"

#define BUFFER_SIZE 1024
#define MAX_EMPLOYEES 100
#define MINIMUM_INCOME_THRESHOLD 30000.0
#define CUSTOMER_FILE "Customer/customers.txt"
#define LOAN_FILE "Customer/loans.txt"
#define TRANSACTION_FILE "Customer/transaction_history.txt"
#define EMPLOYEE_FILE "Employee/employees.txt"
#define MANAGE_LOAN_FILE "Loan/manage_loan.txt"
#define APPROVED_LOANS_FILE "Loan/processed_loans_approved.txt"
#define REJECTED_LOANS_FILE "Loan/processed_loans_rejected.txt"
#define TEMP_MANAGE_LOAN_FILE "temp_manage_loan.txt"

Employee emp;

// Function prototypes
int _emp_load_customers_from_fd(int fd);
int _emp_save_customers_to_fd(int fd);
int _emp_load_employees_from_fd(int fd);
int _emp_save_employees_to_fd(int fd);
int authenticate_employee(const char* username, const char* password);
void handle_employee_login(int sock);
void update_customer(int sock);
void Process_LoanApp(int sock, int employee_id);
void change_employee_password(int sock, const char* username);

int _emp_load_customers_from_fd(int fd) {
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Failed to lseek to start of customer file");
        return -1;
    }

    char buffer[BUFFER_SIZE * 4];
    char line[BUFFER_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;
    customer_count = 0; 

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;

        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';
                
                if (customer_count < 100) {
                    sscanf(line, "%s %s %f %d %d", 
                           customers[customer_count].username, 
                           customers[customer_count].password, 
                           &customers[customer_count].balance, 
                           &customers[customer_count].id, 
                           &customers[customer_count].is_active);
                    customer_count++;
                }
                line[0] = '\0';
                line_pos = 0;
                ptr = newline + 1;
            } else {
                strcpy(line, ptr);
                line_pos = strlen(line);
                break;
            }
        }
    }
    return customer_count;
}

int _emp_save_customers_to_fd(int fd) {
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Failed to lseek to start for writing");
        return -1;
    }
    if (ftruncate(fd, 0) == -1) {
        perror("Failed to truncate customer file");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    for (int i = 0; i < customer_count; i++) {
        snprintf(buffer, sizeof(buffer), "%s %s %.2f %d %d\n", 
                 customers[i].username, customers[i].password, 
                 customers[i].balance, customers[i].id, customers[i].is_active);
        
        if (write(fd, buffer, strlen(buffer)) == -1) {
            perror("Failed to write customer to file");
            return -1;
        }
    }
    return 0;
}

int _emp_load_employees_from_fd(int fd) {
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Failed to lseek to start of employee file");
        return -1;
    }

    char buffer[BUFFER_SIZE * 4];
    char line[BUFFER_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;
    emp_count = 0;

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;

        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';
                
                if (emp_count < 100) {
                    sscanf(line, "%s %s %d", 
                           employees[emp_count].username, 
                           employees[emp_count].password, 
                           &employees[emp_count].id);
                    emp_count++;
                }
                line[0] = '\0';
                line_pos = 0;
                ptr = newline + 1;
            } else {
                strcpy(line, ptr);
                line_pos = strlen(line);
                break;
            }
        }
    }
    return emp_count;
}

int _emp_save_employees_to_fd(int fd) {
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Failed to lseek to start for writing");
        return -1;
    }
    if (ftruncate(fd, 0) == -1) {
        perror("Failed to truncate employee file");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    for (int i = 0; i < emp_count; i++) {
        snprintf(buffer, sizeof(buffer), "%s %s %d\n", 
                 employees[i].username, employees[i].password, 
                 employees[i].id);
        
        if (write(fd, buffer, strlen(buffer)) == -1) {
            perror("Failed to write employee to file");
            return -1;
        }
    }
    return 0; // Success
}


// Authenticate employee
int authenticate_employee(const char* username, const char* password) {
    
    int fd = open(EMPLOYEE_FILE, O_RDONLY | O_CREAT, 0644);
    if(fd == -1){
        perror("Failed to open employee file");
        return 0;
    }

    if (flock(fd, LOCK_SH) == -1) {
        perror("Failed to lock customer file");
        close(fd);
        return 0;
    }

    char buffer[BUFFER_SIZE * 2];
    char line[BUFFER_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;
    int found = 0;

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;

        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';

                Employee auth_emp;
                sscanf(line, "%s %s %d", auth_emp.username, auth_emp.password, &auth_emp.id);
                if (strcmp(auth_emp.username, username) == 0 && strcmp(auth_emp.password, password) == 0) {
                    found = auth_emp.id;
                    break;
                }
                
                line[0] = '\0';
                line_pos = 0;
                ptr = newline + 1;
            } else {
                strcpy(line, ptr);
                line_pos = strlen(line);
                break;
            }
        }
        if (found) break;
    }

    flock(fd, LOCK_UN); 
    close(fd);
    return found;
}

void view_customer_transactions(int sock) {
    char id_buffer[50];
    char buffer[BUFFER_SIZE * 4]; 
    char line[BUFFER_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;
    char history[BUFFER_SIZE * 10] = ""; 
    int history_len = 0;
    int found = 0;

    int bytes = recv(sock, id_buffer, sizeof(id_buffer) - 1, 0);
    id_buffer[bytes] = '\0';
    id_buffer[strcspn(id_buffer, "\n")] = 0;
    int search_id = atoi(id_buffer);

    int fd = open(TRANSACTION_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) { /* error handling */ return; }

    if (flock(fd, LOCK_SH) == -1) { /* error handling */ return; }

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;

        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';

                int line_id;
                if (sscanf(line, "%d |", &line_id) == 1) {
                    if (line_id == search_id) {
                        found = 1;
                        if (history_len + len + 1 < sizeof(history)) {
                            strcat(history, line);
                            strcat(history, "\n");
                            history_len += len + 1;
                        }
                    }
                }

                line[0] = '\0';
                line_pos = 0;
                ptr = newline + 1;
            } else {
                strcpy(line, ptr);
                line_pos = strlen(line);
                break;
            }
        }
    }

    flock(fd, LOCK_UN);
    close(fd);

    if (!found) {
        send(sock, "No transaction history found for that user ID.\n", 47, 0);
    } else {
        send(sock, history, strlen(history), 0);
    }
}

void view_assigned_loans(int sock, int employee_id) {
    char buffer[BUFFER_SIZE * 4]; 
    char line[BUFFER_SIZE] = {0};
    int line_pos = 0;
    ssize_t bytes_read;
    char assigned_loans[BUFFER_SIZE * 10] = ""; 
    int history_len = 0;
    int found = 0;

    int fd = open(MANAGE_LOAN_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) { /* ... error handling ... */ return; }

    if (flock(fd, LOCK_SH) == -1) { /* ... error handling ... */ return; }

    const char *header = "--- Your Assigned Loans ---\n";
    strcat(assigned_loans, header);
    history_len += strlen(header);

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;

        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';

                int line_loan_id, line_emp_id;
                // Format is: LoanID | EmpID | ...
                if (sscanf(line, "%d | %d", &line_loan_id, &line_emp_id) == 2) {
                    if (line_emp_id == employee_id) { // Check if ID matches
                        found = 1;
                        if (history_len + len + 1 < sizeof(assigned_loans)) {
                            strcat(assigned_loans, line);
                            strcat(assigned_loans, "\n");
                            history_len += len + 1;
                        }
                    }
                }

                line[0] = '\0';
                line_pos = 0;
                ptr = newline + 1;
            } else {
                strcpy(line, ptr);
                line_pos = strlen(line);
                break;
            }
        }
    }

    flock(fd, LOCK_UN);
    close(fd);

    if (!found) {
        send(sock, "You have no loans assigned to you.\n", 35, 0);
    } else {
        const char *footer = "--- End of List ---\n";
        strcat(assigned_loans, footer);
        send(sock, assigned_loans, strlen(assigned_loans), 0);
    }
}

void handle_employee_login(int sock) {
    char username[50], password[50];

    const char *username_prompt = "Enter username: ";
    send(sock, username_prompt, strlen(username_prompt), 0);
    recv(sock, username, sizeof(username), 0);
    username[strcspn(username, "\n")] = 0; 

    const char *password_prompt = "Enter password: ";
    send(sock, password_prompt, strlen(password_prompt), 0);
    recv(sock, password, sizeof(password), 0);
    password[strcspn(password, "\n")] = 0;

    int employee_id = authenticate_employee(username, password);
    if (employee_id > 0) {

        // --- 1. SESSION CHECK ---
        if (isUserLoggedIn(employee_id, ROLE_EMPLOYEE)) {
            const char *error_msg = "Login failed! This user is already logged in.";
            send(sock, error_msg, strlen(error_msg), 0);
            close(sock);
            return;
        }
        logUserIn(employee_id, ROLE_EMPLOYEE); // <-- 2. LOG IN

        const char *success_msg = "Login successful!";
        send(sock, success_msg, strlen(success_msg), 0);

        while (1) {
            char choice_str[BUFFER_SIZE];
            memset(choice_str, 0, sizeof(choice_str));

            int bytes_recvd = recv(sock, choice_str, sizeof(choice_str), 0);
            if(bytes_recvd <= 0) {
                 printf("Employee client disconnected.\n");
                 logUserOut(employee_id, ROLE_EMPLOYEE); // <-- 3. LOG OUT
                 break; // Client disconnected
            }

            int menu_choice = atoi(choice_str);
            switch (menu_choice) {
                case 1:
                    view_all_customer(sock);
                    break;
                case 2:
                    Process_LoanApp(sock, employee_id);
                    break;
                case 3:
                    add_customer(sock);
                    break;
                case 4:
                    remove_customer(sock);
                    break;
                case 5:
                    update_customer(sock);
                    break;
                case 6:
                    view_customer_transactions(sock);
                    break;
                case 7:
                    view_assigned_loans(sock, employee_id);
                    break;
                case 8:
                    change_employee_password(sock, username);
                    break;
                case 9:
                    printf("Employee logging out...\n");
                    logUserOut(employee_id, ROLE_EMPLOYEE); // <-- 3. LOG OUT
                    close(sock);
                    return; // Exit thread
                default:
                    const char *invalid_choice = "Invalid choice, please try again.";
                    send(sock, invalid_choice, strlen(invalid_choice), 0);
            }
        }
    } else {
        const char *error_msg = "Login failed! Invalid username or password.";
        send(sock, error_msg, strlen(error_msg), 0);
    }
    close(sock);
    printf("Client disconnected.\n");
}

void update_customer(int sock) {
    char buffer[BUFFER_SIZE];
    char old_username[50], new_username[50], new_password[50];
    int found = 0;
    Customer customer;
    recv(sock, buffer, BUFFER_SIZE, 0);
    sscanf(buffer, "UPDATE_CUSTOMER %s %s %s", old_username, new_username, new_password);

    int fd = open(CUSTOMER_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        const char *error_msg = "Error: Could not open customer file.\n";
        send(sock, error_msg, strlen(error_msg), 0);
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock customer file for update");
        close(fd);
        const char *error_msg = "Error: Database is busy. Try again.\n";
        send(sock, error_msg, strlen(error_msg), 0);
        return;
    }

    _emp_load_customers_from_fd(fd);

    for (int i = 0; i < customer_count; i++) {
        if (strcmp(customers[i].username, old_username) == 0) {
            found = 1;
            strcpy(customers[i].username, new_username);
            strcpy(customers[i].password, new_password);
            break;
        }
    }

    if (found) {
        if (_emp_save_customers_to_fd(fd) == -1) {
            // Failed to save
            const char *error_msg = "Error: Could not save updated customer data.\n";
            send(sock, error_msg, strlen(error_msg), 0);
        } else {
            // Success
            const char *success_msg = "Customer update completed.\n";
            send(sock, success_msg, strlen(success_msg), 0);
        }
    } else {
        const char *error_msg = "Error: Customer not found.\n";
        send(sock, error_msg, strlen(error_msg), 0);
    }
    
    flock(fd, LOCK_UN);
    close(fd);
}

void Process_LoanApp(int sock, int employee_id) {
    char line[BUFFER_SIZE] = {0};
    char buffer[BUFFER_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;
    int found_loans_for_emp = 0;
    Loan loan_details; // To store the parsed loan

    // 1. Open all necessary files
    int fd_manage = open(MANAGE_LOAN_FILE, O_RDONLY);
    if (fd_manage == -1) {
        perror("Failed to open manage_loan.txt");
        send(sock, "No loans to process.\n", 22, 0);
        return;
    }
    int fd_temp = open(TEMP_MANAGE_LOAN_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int fd_approved = open(APPROVED_LOANS_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    int fd_rejected = open(REJECTED_LOANS_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);

    // 2. Lock all files
    flock(fd_manage, LOCK_EX); // Exclusive lock, we will replace this file
    flock(fd_temp, LOCK_EX);
    flock(fd_approved, LOCK_EX);
    flock(fd_rejected, LOCK_EX);

    const char *header_msg = "Processing assigned loans...\n";
    send(sock, header_msg, strlen(header_msg), 0);

    // 3. Read the assignment file (manage_loan.txt)
    while ((bytes_read = read(fd_manage, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;
        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';

                // --- Main Logic ---
                int loan_id, emp_id;
                // Parse the new 8-field format from the line
                sscanf(line, "%d | %d | %s | %f | %s | %f | %s | %s",
                       &loan_id,
                       &emp_id,
                       loan_details.customer_username,
                       &loan_details.loan_amount,
                       loan_details.loan_purpose,
                       &loan_details.monthly_income,
                       loan_details.employment_status,
                       loan_details.contact_info);

                loan_details.loan_id = loan_id; // Store the ID

                if (emp_id == employee_id) {
                    found_loans_for_emp = 1;

                    // Send details to client
                    char loan_str[BUFFER_SIZE];
                    snprintf(loan_str, sizeof(loan_str), 
                             "\n--- Loan ID: %d ---\n"
                             "Customer: %s\n"
                             "Amount: %.2f\n"
                             "Purpose: %s\n"
                             "Income: %.2f\n"
                             "Contact: %s\n"
                             "--------------------",
                             loan_details.loan_id, loan_details.customer_username,
                             loan_details.loan_amount, loan_details.loan_purpose,
                             loan_details.monthly_income, loan_details.contact_info);

                    send(sock, loan_str, strlen(loan_str), 0);

                    // Wait for client's decision
                    char choice_buf[4];
                    memset(choice_buf, 0, 4);
                    recv(sock, choice_buf, sizeof(choice_buf), 0);
                    int choice = atoi(choice_buf);

                    // Write to the appropriate processed file
                    if (choice == 1) { // --- LOAN IS APPROVED ---
                        write(fd_approved, line, strlen(line));
                        write(fd_approved, "\n", 1);

                        // --- NEW LOGIC TO UPDATE CUSTOMER BALANCE ---
                        int fd_cust = open(CUSTOMER_FILE, O_RDWR);
                        if (fd_cust == -1) {
                            perror("Failed to open customer file for loan deposit");
                            send(sock, "Error: Loan approved but failed to update balance.\n", 50, 0);
                        } else {
                            if (flock(fd_cust, LOCK_EX) == -1) {
                                perror("Failed to lock customer file for loan deposit");
                                send(sock, "Error: Loan approved but customer DB is busy.\n", 45, 0);
                            } else {
                                // 1. Load all customers
                                _emp_load_customers_from_fd(fd_cust);

                                int cust_found = 0;
                                for (int i = 0; i < customer_count; i++) {
                                    // 2. Find the matching customer
                                    if (strcmp(customers[i].username, loan_details.customer_username) == 0) {
                                        // 3. Update their balance
                                        customers[i].balance += loan_details.loan_amount;
                                        cust_found = 1;

                                        // 4. Save the updated customer file
                                        _emp_save_customers_to_fd(fd_cust);

                                        // 5. Log the transaction (we have access to customer.h)
                                        log_transaction(customers[i].id, customers[i].username, "Loan (Approved)", loan_details.loan_amount);

                                        break;
                                    }
                                }
                                if (!cust_found) {
                                    send(sock, "Error: Loan approved but customer account not found.\n", 53, 0);
                                }

                                flock(fd_cust, LOCK_UN);
                            }
                            close(fd_cust);
                        }
                        // --- END OF NEW LOGIC ---

                    } else { // --- LOAN IS REJECTED ---
                        write(fd_rejected, line, strlen(line));
                        write(fd_rejected, "\n", 1);
                    }
                } else {
                    // This loan is not for this employee, write it to the temp file
                    write(fd_temp, line, strlen(line));
                    write(fd_temp, "\n", 1);
                }

                line[0] = '\0';
                line_pos = 0;
                ptr = newline + 1;
            } else {
                strcpy(line, ptr);
                line_pos = strlen(line);
                break;
            }
        }
    }

    // 4. Unlock and close all files
    flock(fd_manage, LOCK_UN);
    flock(fd_temp, LOCK_UN);
    flock(fd_approved, LOCK_UN);
    flock(fd_rejected, LOCK_UN);
    close(fd_manage);
    close(fd_temp);
    close(fd_approved);
    close(fd_rejected);

    // 5. Replace old manage_loan.txt with the temp file
    remove(MANAGE_LOAN_FILE);
    rename(TEMP_MANAGE_LOAN_FILE, MANAGE_LOAN_FILE);

    if (!found_loans_for_emp) {
        send(sock, "No loans assigned to you to process.\n", 37, 0);
    } else {
        send(sock, "Loan processing completed.\n", 28, 0);
    }
}

void change_employee_password(int sock, const char* username) {
    char old_password[50], new_password[50];
    int password_matched = 0;

    recv(sock, old_password, sizeof(old_password), 0);
    old_password[strcspn(old_password, "\n")] = 0;  

    int fd = open(EMPLOYEE_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open employee file");
        send(sock, "Error: Database connection failed.\n", 33, 0);
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock employee file for password change");
        close(fd);
        send(sock, "Error: Bank is busy, try again.\n", 33, 0);
        return;
    }

    // 1. READ
    _emp_load_employees_from_fd(fd);

    // 2. MODIFY
    int user_index = -1;
    for (int i = 0; i < emp_count; i++) {
        if (strcmp(employees[i].username, username) == 0 && strcmp(employees[i].password, old_password) == 0) {
            password_matched = 1;
            user_index = i;
            break;
        }
    }

    if (password_matched) {
        const char *match_msg = "Password match! Enter new password.";
        send(sock, match_msg, strlen(match_msg), 0);

        recv(sock, new_password, sizeof(new_password), 0);
        new_password[strcspn(new_password, "\n")] = 0;  
        
        // Update in memory
        strcpy(employees[user_index].password, new_password);  
                
        // 3. WRITE
        if (_emp_save_employees_to_fd(fd) == 0) {
            const char *success_msg = "Password changed successfully!";
            send(sock, success_msg, strlen(success_msg), 0);
        } else {
            perror("CRITICAL: Failed to save password change");
            send(sock, "Error: Failed to save new password.\n", 36, 0);
        }

    } else {
        const char *error_msg = "Old password did not match.";
        send(sock, error_msg, strlen(error_msg), 0);
    }
    
    flock(fd, LOCK_UN);
    close(fd);
}


