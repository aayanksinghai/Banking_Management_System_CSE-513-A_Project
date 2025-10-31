#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/file.h>  
#include <errno.h>
#include <fcntl.h>   
#include "manager.h"
#include "../Employee/employee.h"
#include "../Customer/customer.h"

#define BUFFER_SIZE 1024
#define MAX_CUSTOMERS 100

#define CUSTOMER_FILE "Customer/customers.txt"
#define EMPLOYEE_FILE "Employee/employees.txt"
#define LOAN_FILE "Customer/loans.txt"
#define FEEDBACK_FILE "Customer/feedback.txt"
#define MANAGE_LOAN_FILE "manage_loan.txt"
#define MANAGER_FILE "Manager/managers.txt"

extern Manager managers[100];
extern int manager_count;

int authenticate_manager(const char* username, const char* password);
int _mgr_load_customers_from_fd(int fd);
int _mgr_save_customers_to_fd(int fd);
void Activate_Customer_Acc(int sock);
void Deactivate_Customer_Acc(int sock);
void Assign_LoanApp_to_Employee(int sock);
void Review_Customer_feedback(int sock);
void change_manager_password(int sock, const char* username);


// NEW FUNCTION (Read-Only)
int authenticate_manager(const char* username, const char* password) {
    int fd = open(MANAGER_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) {
        // If file doesn't exist, create it with the default manager
        if (errno == ENOENT) {
            fd = open(MANAGER_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                perror("Failed to create manager file");
                return 0;
            }
            // Add the original hard-coded manager
            const char *default_manager = "BudgetBoss OnBudget 901\n";
            write(fd, default_manager, strlen(default_manager));
            close(fd);
            
            // Re-open for reading
            fd = open(MANAGER_FILE, O_RDONLY);
            if (fd == -1) return 0; // Failed
        } else {
            perror("Failed to open manager file");
            return 0;
        }
    }
    
    if (flock(fd, LOCK_SH) == -1) {
        perror("Failed to lock manager file");
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
                Manager auth_mgr;
                sscanf(line, "%s %s %d", auth_mgr.username, auth_mgr.password, &auth_mgr.id);
                if (strcmp(auth_mgr.username, username) == 0 && strcmp(auth_mgr.password, password) == 0) {
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

int _mgr_load_managers_from_fd(int fd) {
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Failed to lseek to start of manager file");
        return -1;
    }
    char buffer[BUFFER_SIZE * 4];
    char line[BUFFER_SIZE]={0};
    int line_pos = 0;
    ssize_t bytes_read;
    manager_count = 0; 
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;
        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';
                if (manager_count < 100) {
                    sscanf(line, "%s %s %d", 
                           managers[manager_count].username, 
                           managers[manager_count].password, 
                           &managers[manager_count].id);
                    manager_count++;
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
    return manager_count;
}

// ADD THIS HELPER
int _mgr_save_managers_to_fd(int fd) {
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Failed to lseek to start for writing");
        return -1;
    }
    if (ftruncate(fd, 0) == -1) {
        perror("Failed to truncate manager file");
        return -1;
    }
    char buffer[BUFFER_SIZE];
    for (int i = 0; i < manager_count; i++) {
        snprintf(buffer, sizeof(buffer), "%s %s %d\n", 
                 managers[i].username, managers[i].password, 
                 managers[i].id);
        if (write(fd, buffer, strlen(buffer)) == -1) {
            perror("Failed to write manager to file");
            return -1;
        }
    }
    return 0; // Success
}


// Helper to load all customers from an already-opened file descriptor
int _mgr_load_customers_from_fd(int fd) {
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Failed to lseek to start of customer file");
        return -1;
    }

    char buffer[BUFFER_SIZE * 4];
    char line[BUFFER_SIZE]={0};
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
int _mgr_save_customers_to_fd(int fd) {
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



void handle_manager_login(int sock) {
    char username[30], password[30];

    const char *username_prompt = "Enter username: ";
    send(sock, username_prompt, strlen(username_prompt), 0);
    recv(sock, username, sizeof(username), 0);
    username[strcspn(username, "\n")] = 0;

    const char *password_prompt = "Enter password: ";
    send(sock, password_prompt, strlen(password_prompt), 0);
    recv(sock, password, sizeof(password), 0);
    password[strcspn(password, "\n")] = 0;

    if (authenticate_manager(username, password)) {
        const char *success_msg = "Login successful!";
        send(sock, success_msg, strlen(success_msg), 0);

        while (1) {
            char choice[2];
            memset(choice, 0, sizeof(choice));
            int bytes_recvd = recv(sock, choice, sizeof(choice), 0);
            if (bytes_recvd <= 0) {
                printf("Manager client disconnected.\n");
                break; // Client disconnected
            }

            int menu_choice = atoi(choice);
            switch (menu_choice) {
                case 1:
                    Activate_Customer_Acc(sock);
                    break;
                case 2:
                    Deactivate_Customer_Acc(sock);
                    break;
                case 3:
                    Assign_LoanApp_to_Employee(sock);
                    break;
                case 4:
                    Review_Customer_feedback(sock);
                    break;
                case 5:
                    change_manager_password(sock, username);
                    break;
                case 6:
                    printf("Manager logging out...\n");
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
}

void Activate_Customer_Acc(int sock) {
    char id_buffer[BUFFER_SIZE];
    int found = 0;

    memset(id_buffer, 0, BUFFER_SIZE);
    int bytes = recv(sock, id_buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) return; // Handle disconnect
    id_buffer[strcspn(id_buffer, "\n")] = 0;
    int account_id = atoi(id_buffer);

    int fd = open(CUSTOMER_FILE, O_RDWR);
    if (fd == -1) {
        perror("Unable to open customer file");
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Unable to lock file for activation");
        close(fd);
        send(sock, "Error: Database is busy. Try again.\n", 35, 0);
        return;
    }

    // 1. READ
    _mgr_load_customers_from_fd(fd);

    // 2. MODIFY
    char msg[100];
    for(int i=0; i<customer_count; i++) {
        if (customers[i].id == account_id) {
            found = 1;
            if (customers[i].is_active == 0) {
                customers[i].is_active = 1; // Activate in memory
                strcpy(msg, "Customer's account is activated\n");
            } else {
                strcpy(msg, "Customer's account is already active\n");
            }
            break;
        }
    }

    if (!found) {
        strcpy(msg, "Customer ID not found\n");
    }

    // 3. WRITE (if found and needed to be activated)
    if (found) {
        if (_mgr_save_customers_to_fd(fd) == -1) {
            perror("Failed to save customer activation");
            strcpy(msg, "Error: Failed to save changes to database.\n");
        }
    }
    
    // 4. Send result, UNLOCK, and CLOSE
    send(sock, msg, strlen(msg), 0);
    flock(fd, LOCK_UN);
    close(fd);
}

void Deactivate_Customer_Acc(int sock){
    char id_buffer[BUFFER_SIZE];
    int found = 0;

    memset(id_buffer, 0, BUFFER_SIZE);
    int bytes = recv(sock, id_buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) return; // Handle disconnect
    id_buffer[strcspn(id_buffer, "\n")] = 0;
    int account_id = atoi(id_buffer);

    int fd = open(CUSTOMER_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Unable to open customer file");
        send(sock, "Error: Could not access database.\n", 33, 0);
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Unable to lock file for deactivation");
        close(fd);
        send(sock, "Error: Database is busy. Try again.\n", 35, 0);
        return;
    }
    
    // 1. READ
    _mgr_load_customers_from_fd(fd);

    // 2. MODIFY
    char msg[100];
    for(int i=0; i<customer_count; i++) {
        if (customers[i].id == account_id) {
            found = 1;
            if (customers[i].is_active == 1) {
                customers[i].is_active = 0; // Deactivate in memory
                strcpy(msg, "Customer's account deactivated successfully\n");
            } else {
                strcpy(msg, "Customer's account is already deactivated\n");
            }
            break;
        }
    }

    if (!found) {
        strcpy(msg, "Customer ID not found\n");
    }

    // 3. WRITE (if found and needed to be deactivated)
    if (found) {
        if (_mgr_save_customers_to_fd(fd) == -1) {
            perror("Failed to save customer deactivation");
            strcpy(msg, "Error: Failed to save changes to database.\n");
        }
    }
    
    // 4. Send result, UNLOCK, and CLOSE
    send(sock, msg, strlen(msg), 0);
    flock(fd, LOCK_UN);
    close(fd);
}

void Assign_LoanApp_to_Employee(int sock) {
    int fd_loan = open(LOAN_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd_loan == -1) {
        perror("Failed to open loans.txt");
        send(sock, "Error: Cannot open loan applications.\n", 37, 0);
        return;
    }

    int fd_manage = open(MANAGE_LOAN_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd_manage == -1) {
        perror("Failed to open manage_loan.txt");
        close(fd_loan);
        send(sock, "Error: Cannot open loan management file.\n", 41, 0);
        return;
    }

    // Lock both files
    if (flock(fd_loan, LOCK_SH) == -1) {
        perror("Failed to lock loans.txt");
        close(fd_loan); close(fd_manage);
        return;
    }
    if (flock(fd_manage, LOCK_EX) == -1) {
        perror("Failed to lock manage_loan.txt");
        flock(fd_loan, LOCK_UN);
        close(fd_loan); close(fd_manage);
        return;
    }

    LoanApplication loan;
    char buffer[BUFFER_SIZE];
    char line[BUFFER_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;
    char employee_assigned[50];
    char message[BUFFER_SIZE];

    while ((bytes_read = read(fd_loan, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;

        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';

                // Process the line
                sscanf(line, "%s %f %s %f %s %s", loan.customer_username, &loan.loan_amount, loan.loan_purpose,
                       &loan.monthly_income, loan.employment_status, loan.contact_info);

                if (strcmp(loan.loan_purpose, "car") == 0)
                    strcpy(employee_assigned, "employee1 (car loan)");
                else if (strcmp(loan.loan_purpose, "home") == 0)
                    strcpy(employee_assigned, "employee2 (home loan)");
                else if (strcmp(loan.loan_purpose, "education") == 0)
                    strcpy(employee_assigned, "employee3 (education loan)");
                else if (strcmp(loan.loan_purpose, "gold") == 0)
                    strcpy(employee_assigned, "employee4 (gold loan)");
                else if (strcmp(loan.loan_purpose, "personal") == 0)
                    strcpy(employee_assigned, "employee5 (personal loan)");
                else
                    strcpy(employee_assigned, "No employee assigned for this loan purpose");

                snprintf(message, sizeof(message), "Loan application for %s (Purpose: %s) assigned to %s\n",
                        loan.customer_username, loan.loan_purpose, employee_assigned);

                send(sock, message, strlen(message), 0); // Send update to manager

                // Write to manage_loan.txt
                snprintf(message, sizeof(message), "%s | %f | %s | Assigned to: %s\n", 
                        loan.customer_username, loan.loan_amount, loan.loan_purpose, employee_assigned);
                
                if (write(fd_manage, message, strlen(message)) == -1) {
                    perror("Failed to write to manage_loan.txt");
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
    
    // Unlock and close
    flock(fd_loan, LOCK_UN);
    flock(fd_manage, LOCK_UN);
    close(fd_loan);
    close(fd_manage);
}

void Review_Customer_feedback(int sock) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    int fd = open(FEEDBACK_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open feedback file");
        send(sock, "Error: Could not open feedback.\n", 31, 0);
        return;
    }

    if (flock(fd, LOCK_EX) == -1) { // Get EXCLUSIVE lock, as we will truncate after reading
        perror("Failed to lock feedback file");
        close(fd);
        send(sock, "Error: File is busy. Try again.\n", 31, 0);
        return;
    }

    const char *header = "==========================\n     Customer Feedback\n==========================\n";
    send(sock, header, strlen(header), 0);

    // Read and send data in chunks
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        send(sock, buffer, bytes_read, 0);
    }

    const char *end_message = "==========================\n     All feedbacks reviewed\n==========================\n";
    send(sock, end_message, strlen(end_message), 0);
    
    // Now, truncate the file
    if (ftruncate(fd, 0) == -1) {
        perror("Failed to truncate feedback file");
    }

    flock(fd, LOCK_UN);
    close(fd);
}


void change_manager_password(int sock, const char* username) {
    char old_password[50], new_password[50];
    int password_matched = 0;

    recv(sock, old_password, sizeof(old_password), 0);
    old_password[strcspn(old_password, "\n")] = 0;  

    int fd = open(MANAGER_FILE, O_RDWR);
    if (fd == -1) {
        perror("Failed to open manager file");
        send(sock, "Error: Database connection failed.\n", 33, 0);
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock manager file for password change");
        close(fd);
        send(sock, "Error: Bank is busy, try again.\n", 33, 0);
        return;
    }

    // 1. READ
    _mgr_load_managers_from_fd(fd);

    // 2. MODIFY
    int user_index = -1;
    for (int i = 0; i < manager_count; i++) {
        if (strcmp(managers[i].username, username) == 0 && strcmp(managers[i].password, old_password) == 0) {
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
        strcpy(managers[user_index].password, new_password);  

        // 3. WRITE
        if (_mgr_save_managers_to_fd(fd) == 0) {
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
