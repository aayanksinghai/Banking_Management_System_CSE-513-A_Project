#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>       // For errno and ENOENT
#include "employee.h"
#include "../Admin/admin.h"
#include "../Customer/customer.h"

#define BUFFER_SIZE 1024
#define MAX_EMPLOYEES 100
#define MINIMUM_INCOME_THRESHOLD 30000.0
#define CUSTOMER_FILE "Customer/customers.txt"
#define LOAN_FILE "Customer/loans.txt"
#define TRANSACTION_FILE "Customer/transaction_history.txt"
#define EMPLOYEE_FILE "Employee/employees.txt"
#define MANAGE_LOAN_FILE "manage_loan.txt"

Employee emp;

// Function prototypes
int _emp_load_customers_from_fd(int fd);
int _emp_save_customers_to_fd(int fd);
int authenticate_employee(const char* username, const char* password);
void handle_employee_login(int sock);
void update_customer(int sock);
void Process_LoanApp(int sock);
void change_employee_password(int sock, const char* username);



// Helper to load all customers from an already-opened file descriptor
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

// Helper to save all customers to an already-opened file descriptor
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
    return 0; // Success
}


// Authenticate employee
int authenticate_employee(const char* username, const char* password) {
    // FILE *file = fopen(EMPLOYEE_FILE, "r");
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

                // Process the line
                sscanf(line, "%s %s %d", emp.username, emp.password, &emp.id);
                if (strcmp(emp.username, username) == 0 && strcmp(emp.password, password) == 0) {
                    found = 1;
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
    char username[50];
    char buffer[BUFFER_SIZE * 4]; // Large buffer for reading
    char line[BUFFER_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;
    char history[BUFFER_SIZE * 10] = ""; // Large buffer for sending
    int history_len = 0;
    int found = 0;

    // 1. Get username from client
    int bytes = recv(sock, username, sizeof(username) - 1, 0);
    username[bytes] = '\0';
    username[strcspn(username, "\n")] = 0;

    // 2. Open transaction file
    int fd = open(TRANSACTION_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open transaction history file");
        send(sock, "Error: Could not retrieve history.\n", 35, 0);
        return;
    }

    // 3. Get shared lock for reading
    if (flock(fd, LOCK_SH) == -1) {
        perror("Failed to lock transaction file");
        close(fd);
        send(sock, "Error: Server busy, try again.\n", 30, 0);
        return;
    }

    // 4. Read and parse file
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;

        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';
                
                // Check if line contains the *requested* username
                if (strstr(line, username) != NULL) {
                    found = 1;
                    // Append to history buffer if it fits
                    if (history_len + len + 1 < sizeof(history)) {
                        strcat(history, line);
                        strcat(history, "\n");
                        history_len += len + 1;
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

    // 5. Unlock and close
    flock(fd, LOCK_UN);
    close(fd);

    // 6. Send result
    if (!found) {
        send(sock, "No transaction history found for that user.\n", 44, 0);
    } else {
        send(sock, history, strlen(history), 0);
    }
}

void view_assigned_loans(int sock, const char* username) {
    char buffer[BUFFER_SIZE * 4]; // Large buffer for reading
    char line[BUFFER_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;
    char assigned_loans[BUFFER_SIZE * 10] = ""; // Large buffer for sending
    int history_len = 0;
    int found = 0;

    // 1. Open the manager's assignment file
    int fd = open(MANAGE_LOAN_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open assigned loans file");
        send(sock, "Error: Could not retrieve assigned loans.\n", 41, 0);
        return;
    }

    // 2. Get shared lock for reading
    if (flock(fd, LOCK_SH) == -1) {
        perror("Failed to lock assigned loans file");
        close(fd);
        send(sock, "Error: Server busy, try again.\n", 30, 0);
        return;
    }

    const char *header = "--- Your Assigned Loans ---\n";
    strcat(assigned_loans, header);
    history_len += strlen(header);

    // 3. Read and parse file
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;

        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';
                
                // Check if line contains the employee's username
                // This assumes manager assigned to "employee1", "employee2", etc.
                if (strstr(line, username) != NULL) {
                    found = 1;
                    // Append to history buffer if it fits
                    if (history_len + len + 1 < sizeof(assigned_loans)) {
                        strcat(assigned_loans, line);
                        strcat(assigned_loans, "\n");
                        history_len += len + 1;
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

    // 4. Unlock and close
    flock(fd, LOCK_UN);
    close(fd);

    // 5. Send result
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

    if (authenticate_employee(username, password)) {
        const char *success_msg = "Login successful!";
        send(sock, success_msg, strlen(success_msg), 0);

        while (1) {
            char choice_str[BUFFER_SIZE];
            memset(choice_str, 0, sizeof(choice_str));
            // recv(sock, choice_str, sizeof(choice_str), 0);

            int bytes_recvd = recv(sock, choice_str, sizeof(choice_str), 0);
            if(bytes_recvd <= 0) {
                 printf("Employee client disconnected.\n");
                 break; // Client disconnected
            }

            int menu_choice = atoi(choice_str);
            switch (menu_choice) {
                case 1:
                    view_all_customer(sock);
                    break;
                case 2:
                    Process_LoanApp(sock);
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
                    view_assigned_loans(sock, username);
                    break;
                case 8:
                    change_employee_password(sock, username);
                    break;
                case 9:
                    printf("Employee logging out...\n");
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

    // 1. READ
    _emp_load_customers_from_fd(fd);

    // 2. MODIFY (in memory)
    for (int i = 0; i < customer_count; i++) {
        if (strcmp(customers[i].username, old_username) == 0) {
            found = 1;
            // Update the details in the global array
            strcpy(customers[i].username, new_username);
            strcpy(customers[i].password, new_password);
            break;
        }
    }

    // 3. WRITE (back to file)
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
    
    // 4. UNLOCK and CLOSE
    flock(fd, LOCK_UN);
    close(fd);
}

void Process_LoanApp(int sock) {
    int fd = open(LOAN_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open loan file");
        const char *error_msg = "Failed to retrieve loan applications.";
        send(sock, error_msg, strlen(error_msg), 0);
        return;
    }

    if (flock(fd, LOCK_SH) == -1) {
        perror("Failed to lock loan file");
        close(fd);
        const char *error_msg = "Error: Loan file is busy. Try again.\n";
        send(sock, error_msg, strlen(error_msg), 0);
        return;
    }

    LoanApplication loan_app;
    char buffer[BUFFER_SIZE*2];
    char line[BUFFER_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;

    const char *header_msg = "Processing loan applications:\n";
    send(sock, header_msg, strlen(header_msg), 0);

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;

        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';
                
                // Process the line
                sscanf(line, "%s %f %s %f %s %s", loan_app.customer_username, 
                       &loan_app.loan_amount, loan_app.loan_purpose, 
                       &loan_app.monthly_income, loan_app.employment_status, loan_app.contact_info);
                
                // Process each loan application
                if (loan_app.monthly_income >= MINIMUM_INCOME_THRESHOLD) {
                    char success_msg[BUFFER_SIZE];
                    snprintf(success_msg, BUFFER_SIZE, 
                             "Loan approved for %s (Amount: %.2f, Purpose: %s)\n",
                             loan_app.customer_username, loan_app.loan_amount, loan_app.loan_purpose);
                    send(sock, success_msg, strlen(success_msg), 0);
                } else {
                    char reject_msg[BUFFER_SIZE];
                    snprintf(reject_msg, BUFFER_SIZE, 
                             "Loan rejected for %s due to insufficient income (Income: %.2f)\n", 
                             loan_app.customer_username, loan_app.monthly_income);
                    send(sock, reject_msg, strlen(reject_msg), 0);
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

    const char *completion_msg = "Loan processing completed.\n";
    send(sock, completion_msg, strlen(completion_msg), 0);
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


