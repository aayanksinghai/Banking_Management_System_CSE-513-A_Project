#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <time.h>
#include <errno.h>
#include "customer.h"
#include "../loan.h"


#define BUFFER_SIZE 1024
#define CUSTOMER_FILE "Customer/customers.txt"
#define LOAN_FILE "Customer/loans.txt"
#define FEEDBACK_FILE "Customer/feedback.txt"
#define TRANSACTION_FILE "Customer/transaction_history.txt"

Customer customers[100];
int customer_count = 0;
Customer new_customer;

// typedef struct {
//     int loan_id;
//     char username[50];
//     float loan_amount;
//     char loan_purpose[50];
//     float monthly_income;
//     char employment_status[50];
//     char contact_info[20];
// } Loan;

// Function prototypes
void view_account_balance(const char *username, int sock);
void deposit_money(const char *username, int customer_id, int sock);
void withdraw_money(const char *username, int customer_id, int sock);
void transfer_funds(const char *username, int customer_id, int sock);
int _load_customers_from_fd(int fd);
int _save_customers_to_fd(int fd);
void handle_customer_login(int sock);
int authenticate_customer(const char *username, const char *password);
void apply_for_loan(const char *username, int sock); 
void change_password(const char *username, int sock); 
void add_feedback(int customer_id, int sock); 
void view_transaction_history(int customer_id, int sock); 
void log_transaction(int customer_id, const char *username, const char *type, float amount);


// Helper to load all customers from an already-opened file descriptor
// This allows us to re-use it within a locked transaction
int _load_customers_from_fd(int fd) {
    // Rewind file descriptor to the beginning
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Failed to lseek to start of customer file");
        return -1;
    }

    char buffer[BUFFER_SIZE * 4]; // Large buffer for reading
    char line[BUFFER_SIZE]={0};
    int line_pos = 0;
    ssize_t bytes_read;
    customer_count = 0; // Reset global count

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
// This truncates the file and writes everything from the global array
int _save_customers_to_fd(int fd) {
    // Rewind and truncate the file (clear it)
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
            return -1; // Stop on write error
        }
    }
    return 0; // Success
}



void handle_customer_login(int sock) {
    char username[30], password[30];

    const char *username_prompt = "Enter username: ";
    send(sock, username_prompt, strlen(username_prompt), 0);
    recv(sock, username, sizeof(username), 0);
    username[strcspn(username, "\n")] = 0;  

    const char *password_prompt = "Enter password: ";
    send(sock, password_prompt, strlen(password_prompt), 0);
    recv(sock, password, sizeof(password), 0);
    password[strcspn(password, "\n")] = 0;  

    int customer_id = authenticate_customer(username, password);
    if (customer_id > 0){
        const char *success_msg = "Login successful!";
        send(sock, success_msg, strlen(success_msg), 0);

        while (1) {
            char choice[2];
            memset(choice, 0, sizeof(choice));
            // recv(sock, choice, sizeof(choice), 0);
            int bytes_recvd = recv(sock, choice, sizeof(choice), 0);
            if(bytes_recvd <= 0) {
                 printf("Customer client disconnected.\n");
                 break; // Client disconnected
            }

            int menu_choice = atoi(choice);

            switch (menu_choice) {
                case 1:
                    view_account_balance(username, sock);
                    break;
                case 2:
                    deposit_money(username, customer_id, sock);
                    break;
                case 3:
                    withdraw_money(username, customer_id, sock);
                    break;
                case 4:
                    transfer_funds(username, customer_id, sock);
                    break;
                case 5:
                    apply_for_loan(username, sock); 
                    break;
                case 6:
                    change_password(username, sock);
                    break;
                case 7:
                    add_feedback(customer_id, sock);
                    break;
                case 8:
                    view_transaction_history(customer_id, sock);
                    break;
                case 9: // Logout
                    printf("Customer logging out...\n");
                    close(sock);
                    return; // Exit thread                    
                default:
                    const char *invalid_choice = "Invalid choice, please try again.";
                    send(sock, invalid_choice, strlen(invalid_choice), 0);
            }
        }
    } 
    else if (customer_id == -1) {
        const char *inactive_msg = "Login failed! Your account is deactivated.";
        send(sock, inactive_msg, strlen(inactive_msg), 0);
    }
    else {
        const char *error_msg = "Login failed! Invalid username or password.";
        send(sock, error_msg, strlen(error_msg), 0);
    }

    close(sock);
}

int authenticate_customer(const char *username, const char *password) {
    int fd = open(CUSTOMER_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open customer file");
        return 0; // 0 = fail
    }

    if (flock(fd, LOCK_SH) == -1) {
        perror("Failed to lock customer file");
        close(fd);
        return 0;
    }

    _load_customers_from_fd(fd); // Use helper

    flock(fd, LOCK_UN);
    close(fd);

    // Now, check the array
    for (int i = 0; i < customer_count; i++) {
        if (strcmp(customers[i].username, username) == 0 && strcmp(customers[i].password, password) == 0) {
            if (customers[i].is_active == 1) {
                return customers[i].id;  // --- RETURN ID INSTEAD OF 1 ---
            } else {
                return -1; // Account is deactivated
            }
        }
    }

    return 0; // Not found
}

void view_account_balance(const char *username, int sock) {
    // FILE *file = fopen(CUSTOMER_FILE, "r");
    int fd = open(CUSTOMER_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open customer file");
        send(sock, "Error: Could not access the account. \n", 31, 0);
        return;
    }

    if (flock(fd, LOCK_SH) == -1) {
        perror("Failed to lock customer file");
        close(fd);
        send(sock, "Error: Could not access account.\n", 31, 0);
        return;
    }

    _load_customers_from_fd(fd);
    
    flock(fd, LOCK_UN);
    close(fd);

    for (int i = 0; i < customer_count; i++) {
        if (strcmp(customers[i].username, username) == 0) {
            char balance_msg[100];
            snprintf(balance_msg, sizeof(balance_msg), "Account Balance: %.2f\n", customers[i].balance);
            send(sock, balance_msg, strlen(balance_msg), 0);
            return;
        }
    }
    // Should not happen if logged in, but as a fallback
    send(sock, "Error: Account not found.\n", 26, 0);
}

void deposit_money(const char *username, int customer_id, int sock) {
    float amount;
    char msg[100];

    recv(sock, msg, sizeof(msg), 0);
    sscanf(msg, "%f", &amount);

    if (amount <= 0) {
        snprintf(msg, sizeof(msg), "Deposit amount must be positive.");
        send(sock, msg, strlen(msg), 0);
        return;
    }

    int fd = open(CUSTOMER_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open customer file");
        send(sock, "Error: Database connection failed.\n", 33, 0);
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock customer file for deposit");
        close(fd);
        send(sock, "Error: Bank is busy, try again.\n", 33, 0);
        return;
    }

    // 1. READ
    _load_customers_from_fd(fd);

    // 2. MODIFY
    int found = 0;
    for (int i = 0; i < customer_count; i++) {
        if (strcmp(customers[i].username, username) == 0) {
            customers[i].balance += amount;
            snprintf(msg, sizeof(msg), "Deposit Successful! New Balance: %.2f", customers[i].balance);
            send(sock, msg, strlen(msg), 0);
            found = 1;
            break;
        }
    }

    // 3. WRITE (only if modification was successful)
    if (found) {
        if (_save_customers_to_fd(fd) == 0) {
            // Log only on successful save
            log_transaction(customer_id, username, "Deposit", amount); // <-- USE PASSED-IN ID
        } else {
            // This is bad, the file might be corrupt.
            // A real system would roll back.
            perror("CRITICAL: Failed to save customers after deposit");
        }
    } else {
        snprintf(msg, sizeof(msg), "Error: Account not found during deposit.");
        send(sock, msg, strlen(msg), 0);
    }
    
    // 4. UNLOCK and CLOSE
    flock(fd, LOCK_UN);
    close(fd);

}

void withdraw_money(const char *username, int customer_id, int sock) {
    float amount;
    char msg[100];

    recv(sock, msg, sizeof(msg), 0);
    sscanf(msg, "%f", &amount);

    if (amount <= 0) {
        snprintf(msg, sizeof(msg), "Withdrawal amount must be positive.");
        send(sock, msg, strlen(msg), 0);
        return;
    }

    // FILE *file = fopen(CUSTOMER_FILE, "r");
    int fd = open(CUSTOMER_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open customer file");
        send(sock, "Error: Database connection failed.\n", 33, 0);
        return;
    }
    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock customer file for withdrawal");
        close(fd);
        send(sock, "Error: Bank is busy, try again.\n", 33, 0);
        return;
    }

    // 1. READ
    _load_customers_from_fd(fd);

    // 2. MODIFY
    int found = 0;
    int success = 0;
    for (int i = 0; i < customer_count; i++) {
        if (strcmp(customers[i].username, username) == 0) {
            found = 1;
            if (customers[i].balance >= amount) {
                customers[i].balance -= amount;
                snprintf(msg, sizeof(msg), "Withdrawal Successful! New Balance: %.2f", customers[i].balance);
                success = 1;
            } else {
                snprintf(msg, sizeof(msg), "Insufficient funds! Current Balance: %.2f", customers[i].balance);
            }
            send(sock, msg, strlen(msg), 0);
            break;
        }
    }

    // 3. WRITE (only if modification was successful)
    if (found && success) {
        if (_save_customers_to_fd(fd) == 0) {
            log_transaction(customer_id, username, "Withdrawal", amount); // <-- USE PASSED-IN ID
        } else {
            perror("CRITICAL: Failed to save customers after withdrawal");
        }
    } else if (!found) {
        send(sock, "Error: Account not found.\n", 26, 0);
    }
    
    flock(fd, LOCK_UN);
    close(fd);
}

// Function to transfer funds
void transfer_funds(const char *username, int customer_id, int sock) {
    char msg[100];
    int target_account_number;
    float transfer_amount;

    recv(sock, msg, sizeof(msg), 0);
    sscanf(msg, "%d %f", &target_account_number, &transfer_amount);

    if (transfer_amount <= 0) {
        snprintf(msg, sizeof(msg), "Transfer amount must be positive.");
        send(sock, msg, strlen(msg), 0);
        return;
    }

    // FILE *file = fopen(CUSTOMER_FILE, "r");
    int fd = open(CUSTOMER_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open customer file");
        send(sock, "Error: Database connection failed.\n", 33, 0);
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock customer file for transfer");
        close(fd);
        send(sock, "Error: Bank is busy, try again.\n", 33, 0);
        return;
    }
    
    // 1. READ
    _load_customers_from_fd(fd);

    // 2. MODIFY
    Customer *current_user = NULL;
    Customer *target_user = NULL;
    int success = 0;

    for (int i = 0; i < customer_count; i++) {
        if (strcmp(customers[i].username, username) == 0) {
            current_user = &customers[i];
        }
        if (customers[i].id == target_account_number) {
            target_user = &customers[i];
        }
    }

    if (current_user && target_user) {
        if (current_user->balance >= transfer_amount) {
            current_user->balance -= transfer_amount;
            target_user->balance += transfer_amount;
            snprintf(msg, sizeof(msg), "Transfer Successful! New Balance: %.2f", current_user->balance);
            success = 1;
        } else {
            snprintf(msg, sizeof(msg), "Insufficient funds! Current Balance: %.2f", current_user->balance);
        }
    } else {
        snprintf(msg, sizeof(msg), "Transfer Failed! Target user not found.");
    }
    send(sock, msg, strlen(msg), 0);

    // 3. WRITE
    if (success) {
        if (_save_customers_to_fd(fd) == 0) {
            // Log for the SENDER (Debit)
            log_transaction(customer_id, username, "Transfer (Debit)", transfer_amount);

            // Log for the RECEIVER (Credit)
            log_transaction(target_user->id, target_user->username, "Transfer (Credit)", transfer_amount);

        } else {
            perror("CRITICAL: Failed to save customers after transfer");
        }
    }

    flock(fd, LOCK_UN);
    close(fd);
}

void apply_for_loan(const char *username, int sock) {
    Loan new_loan; // Use the struct from loan.h
    char msg[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    Loan temp_loan; // For reading/parsing
    int max_loan_id = 0;

    // 1. Receive data from client
    memset(msg, 0, BUFFER_SIZE);
    int bytes_recvd = recv(sock, msg, sizeof(msg) - 1, 0);
    msg[bytes_recvd] = '\0';

    sscanf(msg, "%f %s %f %s %s", 
           &new_loan.loan_amount, 
           new_loan.loan_purpose, 
           &new_loan.monthly_income, 
           new_loan.employment_status, 
           new_loan.contact_info);

    strcpy(new_loan.customer_username, username);

    // 2. Open and Lock LOAN_FILE to find max ID
    int fd = open(LOAN_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open loan file");
        send(sock, "Error: Loan application failed.\n", 31, 0);
        return;
    }
    if (flock(fd, LOCK_EX) == -1) { // Exclusive lock for read/write
        perror("Failed to lock loan file");
        close(fd);
        send(sock, "Error: Server busy, try again.\n", 30, 0);
        return;
    }

    // 3. Read all loans to find the highest ID
    char read_buffer[BUFFER_SIZE * 4];
    char line[BUFFER_SIZE] = {0};
    int line_pos = 0;
    ssize_t bytes_read;

    while ((bytes_read = read(fd, read_buffer, sizeof(read_buffer) - 1)) > 0) {
        read_buffer[bytes_read] = '\0';
        char *ptr = read_buffer;
        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';

                // Parse the line to get the ID
                sscanf(line, "%d |", &temp_loan.loan_id);
                if (temp_loan.loan_id > max_loan_id) {
                    max_loan_id = temp_loan.loan_id;
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

    // 4. Set new ID (start at 1000)
    new_loan.loan_id = (max_loan_id == 0) ? 1000 : max_loan_id + 1;

    // 5. Format and append the new loan
    snprintf(buffer, sizeof(buffer), "%d | %s | %.2f | %s | %.2f | %s | %s\n", 
            new_loan.loan_id,
            new_loan.customer_username, 
            new_loan.loan_amount, 
            new_loan.loan_purpose, 
            new_loan.monthly_income, 
            new_loan.employment_status, 
            new_loan.contact_info); 

    // lseek to end to append
    lseek(fd, 0, SEEK_END);
    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Failed to write loan application");
        flock(fd, LOCK_UN);
        close(fd);
        return;
    }

    flock(fd, LOCK_UN);
    close(fd);

    const char *success_msg = "Loan application submitted successfully!";
    send(sock, success_msg, strlen(success_msg), 0);
}

void change_password(const char *username, int sock) {
    char old_password[50], new_password[50];
    int password_matched = 0;

    recv(sock, old_password, sizeof(old_password), 0);
    old_password[strcspn(old_password, "\n")] = 0;  

    int fd = open(CUSTOMER_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open customer file");
        send(sock, "Error: Database connection failed.\n", 33, 0);
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock customer file for password change");
        close(fd);
        send(sock, "Error: Bank is busy, try again.\n", 33, 0);
        return;
    }

    // 1. READ
    _load_customers_from_fd(fd);

    // 2. MODIFY
    int user_index = -1;
    for (int i = 0; i < customer_count; i++) {
        if (strcmp(customers[i].username, username) == 0 && strcmp(customers[i].password, old_password) == 0) {
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
        strcpy(customers[user_index].password, new_password);  
                
        // 3. WRITE
        if (_save_customers_to_fd(fd) == 0) {
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

void add_feedback(int customer_id, int sock){
    char feedback[500];

    recv(sock, feedback, sizeof(feedback), 0);
    feedback[strcspn(feedback, "\n")] = 0;

    int fd = open(FEEDBACK_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error opening feedback file");
        send(sock, "Failed to save feedback\n", 24, 0);
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock feedback file");
        close(fd);
        send(sock, "Error: Server busy, try again.\n", 30, 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    // --- THIS IS THE NEW FORMAT ---
    snprintf(buffer, sizeof(buffer), "%d : %s\n", customer_id, feedback);

    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Failed to write feedback");
    }

    flock(fd, LOCK_UN);
    close(fd);

    send(sock, "Thank you for your feedback!\n", 29, 0);
}

void view_transaction_history(int customer_id, int sock) {
    int fd = open(TRANSACTION_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) { 
        perror("Failed to open transaction history");
        send(sock, "Error: Could not open history.\n", 31, 0);
        return; 
    }
    if (flock(fd, LOCK_SH) == -1) { 
        perror("Failed to lock transaction history");
        close(fd);
        send(sock, "Error: Database busy, try again.\n", 33, 0);
        return; 
    }

    char buffer[BUFFER_SIZE * 4]; 
    char line[BUFFER_SIZE] = {0}; // <--- FIX 1: INITIALIZE LINE TO ALL ZEROS
    int line_pos = 0;
    ssize_t bytes_read;
    char history[BUFFER_SIZE * 10] = ""; 
    int history_len = 0;
    
    // --- FIX 2: ADD A HEADER TO THE MESSAGE ---
    strcat(history, "--- Transaction History ---\n");
    history_len = strlen(history);
      
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
                if (sscanf(line, "%d", &line_id) == 1) {
                    if (line_id == customer_id) {
                        // Check if it fits
                        if (history_len + len + 2 < sizeof(history)) {
                            strcat(history, line);
                            strcat(history, "\n"); // Add the newline back
                            history_len += (len + 1);
                        }
                    }
                }
                
                line[0] = '\0'; // Reset line buffer
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

    // --- FIX 3: CHECK IF ONLY THE HEADER IS PRESENT ---
    if (history_len == strlen("--- Transaction History ---\n")) { 
        send(sock, "No transaction history found.\n", strlen("No transaction history found.\n"), 0);
    } else {
        strcat(history, "--- End of History ---\n"); // Add a footer
        send(sock, history, strlen(history), 0);
    }
}
void log_transaction(int customer_id, const char *username, const char *type, float amount) {
    int fd = open(TRANSACTION_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Failed to open transaction history file");
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock transaction file");
        close(fd);
        return;
    }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strcspn(timestamp, "\n")] = 0;  
    char buffer[BUFFER_SIZE];

    // --- THIS IS THE NEW FORMAT ---
    snprintf(buffer, sizeof(buffer), "%d | %s | %s | %.2f | %s\n", 
             customer_id, username, type, amount, timestamp);

    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Failed to write to transaction history");
    }

    flock(fd, LOCK_UN);
    close(fd);
}

