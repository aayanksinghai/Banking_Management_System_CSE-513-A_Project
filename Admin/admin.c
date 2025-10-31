#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include "admin.h"
#include "../Employee/employee.h"
#include "../Customer/customer.h"
#include "../Manager/manager.h"

#define BUFFER_SIZE 1024
#define MAX_CUSTOMERS 100
#define CUSTOMER_FILE "Customer/customers.txt"
#define EMPLOYEE_FILE "Employee/employees.txt"
#define MANAGER_FILE "Manager/managers.txt"

static char admin_username[] = "root";
static char admin_password[] = "root";

Employee employees[100];
int emp_count = 0;
Employee new_employee;

Manager managers[100];
int manager_count = 0;
Manager new_manager;


// Helper to load all employees from an already-opened file descriptor
int _admin_load_employees_from_fd(int fd) {
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Failed to lseek to start of employee file");
        return -1;
    }

    char buffer[BUFFER_SIZE * 4];
    char line[BUFFER_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;
    emp_count = 0; // Reset global employee count

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

// Helper to save all employees to an already-opened file descriptor
int _admin_save_employees_to_fd(int fd) {
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

void add_BankEmp(int sock){
    char message[BUFFER_SIZE];
    recv(sock, message, BUFFER_SIZE, 0);
    sscanf(message, "%s %s %d", new_employee.username, new_employee.password, &new_employee.id);
    if (is_employee_exists(new_employee.username, new_employee.password)) {
        const char *error_msg = "Employee already exists!";
        send(sock, error_msg, strlen(error_msg), 0);
        return;
    }
    employees[emp_count++] = new_employee;
    save_new_employee(new_employee);
    const char *success_msg = "Employee added successfully!";
    send(sock, success_msg, strlen(success_msg), 0);
}

int is_employee_exists(const char *username, const char *password) {
    int fd = open(EMPLOYEE_FILE, O_RDONLY); //open sys call
    if (fd == -1) {
        if(errno == ENOENT){
            fd = open(EMPLOYEE_FILE, O_CREAT, 0644);
            if(fd != -1) close(fd);
            return 0;
        }
        perror("Failed to open customers file");
        return 0;
    }

    // Shared Lock for reading
    if(flock(fd, LOCK_SH) == -1){
        perror("Failed to lock customer file");
        close(fd);
        return 0;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    char line[BUFFER_SIZE];
    int line_pos = 0;
    int found = 0;

    // Read file chunk by chunk
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;

        // Process the buffer line by line
        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                // Full line found
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';
                
                // Process the complete line
                char existing_username[30], existing_password[30];
                if (sscanf(line, "%s %s", existing_username, existing_password) == 2) {
                    if (strcmp(existing_username, username) == 0 && strcmp(existing_password, password) == 0) {
                        found = 1;
                        break;
                    }
                }
                
                // Reset line buffer
                line[0] = '\0';
                line_pos = 0;
                ptr = newline + 1;
            } else {
                // Incomplete line, store it and read more
                strcpy(line, ptr);
                line_pos = strlen(line);
                break;
            }
        }
        if (found) break;
    }

    flock(fd, LOCK_UN); // Unlock
    close(fd);          // Use 'close' (system call)
    
    return found;

    /*
    char line[BUFFER_SIZE];
    char existing_username[30], existing_password[30];
    
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%s %s", existing_username, existing_password);

        if (strcmp(existing_username, username) == 0 && strcmp(existing_password, password) == 0) {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0; 
    */
}
void save_new_employee(Employee new_employee) {

    // Open with flags: Write-Only, Create if not exist, Append to end
    // 0644 = permissions (rw-r--r--)
    int fd = open(EMPLOYEE_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Failed to open employees file for appending");
        exit(1); // Keep exit(1) on critical save failure
    }

    // Apply exclusive lock for writing
    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock employees file");
        close(fd);
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    // Format data into a string buffer
    snprintf(buffer, sizeof(buffer), "%s %s %d\n", 
             new_employee.username, new_employee.password, new_employee.id);

    // Use 'write' (system call) to write the buffer to the file
    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Failed to write new employee");
    }

    flock(fd, LOCK_UN); // Unlock
    close(fd);          // Use 'close' (system call)

    /*
    FILE *file = fopen(EMPLOYEE_FILE, "a");
    if (file == NULL) {
        perror("Failed to open customers file for appending");
        exit(1);
    }

    fprintf(file, "%s %s %d\n", new_employee.username, new_employee.password, new_employee.id);
    
    fclose(file);
    */
}

int _admin_load_customers_from_fd(int fd, int sock) {

    // Rewind file descriptor to the beginning
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Failed to lseek to start of customer file");
        send(sock, "Error: Could not read database.\n", 31, 0);
        return -1;
    }

    char buffer[BUFFER_SIZE * 4]; // Larger buffer for reading file
    char line[BUFFER_SIZE];
    int line_pos = 0;
    ssize_t bytes_read;

    customer_count = 0; // Reset count

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = buffer;

        while (*ptr) {
            char *newline = strchr(ptr, '\n');
            if (newline) {
                int len = newline - ptr;
                strncat(line, ptr, len);
                line[line_pos + len] = '\0';

                // Process the complete line
                if (customer_count < MAX_CUSTOMERS) {
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

int _admin_save_customers_to_fd(int fd) {
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


int is_customer_exists(const char *username, const char *password) {

    int fd = open(CUSTOMER_FILE, O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) {
             fd = open(CUSTOMER_FILE, O_CREAT, 0644);
             if(fd != -1) close(fd);
             return 0;
        }
        perror("Failed to open customers file");
        return 0;
    }

    if (flock(fd, LOCK_SH) == -1) {
        perror("Failed to lock customers file");
        close(fd);
        return 0;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    char line[BUFFER_SIZE] = {0};
    int line_pos = 0;
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
                
                char existing_username[30], existing_password[30];
                if (sscanf(line, "%s %s", existing_username, existing_password) == 2) {
                    if (strcmp(existing_username, username) == 0 && strcmp(existing_password, password) == 0) {
                        found = 1;
                        break;
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
        if (found) break;
    }

    flock(fd, LOCK_UN);
    close(fd);
    return found;

    /*
    FILE *file = fopen(CUSTOMER_FILE, "r");
    if (file == NULL) {
        perror("Failed to open customers file");
        return 0;
    }

    char line[BUFFER_SIZE];
    char existing_username[30], existing_password[30];
    
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%s %s", existing_username, existing_password);

        if (strcmp(existing_username, username) == 0 && strcmp(existing_password, password) == 0) {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0; 
    */
}

// Function to save new customer to the file
void save_new_customer(Customer new_customer) {

    int fd = open(CUSTOMER_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Failed to open customers file for appending");
        exit(1);
    }
    
    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock customer file");
        close(fd);
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s %s %.2f %d %d\n", 
             new_customer.username, new_customer.password, 
             new_customer.balance, new_customer.id, new_customer.is_active);

    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Failed to write new customer");
    }
    
    flock(fd, LOCK_UN);
    close(fd);

    /*
    FILE *file = fopen(CUSTOMER_FILE, "a");
    if (file == NULL) {
        perror("Failed to open customers file for appending");
        exit(1);
    }

    fprintf(file, "%s %s %.2f %d %d\n", new_customer.username, new_customer.password, 
            new_customer.balance, new_customer.id, new_customer.is_active);
    
    fclose(file);
    */
}

// Function to add a new customer
void add_customer(int sock) {
    char message[BUFFER_SIZE];
    recv(sock, message, BUFFER_SIZE, 0);
    sscanf(message, "%s %s %f %d", new_customer.username, new_customer.password, 
           &new_customer.balance, &new_customer.id);

    if (is_customer_exists(new_customer.username, new_customer.password)) {
        const char *error_msg = "Customer already exists!";
        send(sock, error_msg, strlen(error_msg), 0);
        return;
    }
    new_customer.is_active = 1;
    customers[customer_count++] = new_customer;
    save_new_customer(new_customer);
    const char *success_msg = "Customer added successfully!";
    send(sock, success_msg, strlen(success_msg), 0);
}

void view_all_customer(int sock) {
    char buffer[BUFFER_SIZE];
    // A large buffer to hold all customer data
    char all_customers_buffer[BUFFER_SIZE * 10] = {0}; 

    int fd = open(CUSTOMER_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) {
         perror("Failed to open customers file");
         send(sock, "Error: Could not open customer database.\n", 40, 0);
         return;
    }

    if (flock(fd, LOCK_SH) == -1) { // SHARED lock
        perror("Failed to lock customer file");
        close(fd);
        send(sock, "Error: Could not lock customer database.\n", 40, 0);
        return;
    }

    _admin_load_customers_from_fd(fd, sock); // Call helper

    flock(fd, LOCK_UN);
    close(fd);

    if (customer_count == 0) {
        const char *no_cust_msg = "No customers in the database.\n";
        send(sock, no_cust_msg, strlen(no_cust_msg), 0);
        return;
    }

    // Build one large string
    strcat(all_customers_buffer, "Customers:\n");
    for (int i = 0; i < customer_count; i++) {
        snprintf(buffer, BUFFER_SIZE, "%s %s %.2f %d %d\n", 
                 customers[i].username, customers[i].password, 
                 customers[i].balance, customers[i].id, customers[i].is_active);
        
        // Append to the large buffer, checking for overflow
        if(strlen(all_customers_buffer) + strlen(buffer) < sizeof(all_customers_buffer)) {
            strcat(all_customers_buffer, buffer);
        } else {
            // Buffer is full, stop appending
            break;
        }
    }
    
    // Send the entire buffer at once
    send(sock, all_customers_buffer, strlen(all_customers_buffer), 0);
}


void remove_customer(int sock) {
    char message[BUFFER_SIZE];
    int id_to_remove, found = 0;

    recv(sock, message, BUFFER_SIZE, 0);
    sscanf(message, "%d", &id_to_remove);

    int fd = open(CUSTOMER_FILE, O_RDWR); // Open for Read-Write
    if(fd == -1) {
        perror("Failed to open customer file for remove");
        send(sock, "Error: Database failure.\n", 25, 0);
        return;
    }

    if (flock(fd, LOCK_EX) == -1) { // EXCLUSIVE lock
        perror("Failed to lock customer file for remove");
        close(fd);
        send(sock, "Error: Database busy, try again.\n", 33, 0);
        return;
    }

    _admin_load_customers_from_fd(fd, sock); // 1. READ

    // 2. MODIFY
    for (int i = 0; i < customer_count; i++) {
        if (customers[i].id == id_to_remove) {
            found = 1;
            for (int j = i; j < customer_count - 1; j++) {
                customers[j] = customers[j + 1];
            }
            customer_count--;
            _admin_save_customers_to_fd(fd); // 3. WRITE
            const char *success_msg = "Customer removed successfully!";
            send(sock, success_msg, strlen(success_msg), 0);

            flock(fd, LOCK_UN); // 4. UNLOCK
            close(fd);          // 5. CLOSE
            return;
        }
    }

    flock(fd, LOCK_UN); // 4. UNLOCK (if not found)
    close(fd);          // 5. CLOSE (if not found)

    if (!found) {
        const char *error_msg = "Customer ID not found!";
        send(sock, error_msg, strlen(error_msg), 0);
    }
}

void modify_customer_details(int sock) {
    char old_username[50], new_username[50], new_password[50];
    char buffer[BUFFER_SIZE];
    int found = 0;

    // Get details from client
    recv(sock, buffer, BUFFER_SIZE, 0);
    sscanf(buffer, "%s %s %s", old_username, new_username, new_password);

    int fd = open(CUSTOMER_FILE, O_RDWR);
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

    // 1. READ (using existing admin helper)
    _admin_load_customers_from_fd(fd, sock);// This populates the global 'customers' array

    // 2. MODIFY (in memory)
    for (int i = 0; i < customer_count; i++) {
        if (strcmp(customers[i].username, old_username) == 0) {
            found = 1;
            strcpy(customers[i].username, new_username);
            strcpy(customers[i].password, new_password);
            break;
        }
    }

    // 3. WRITE (back to file using existing admin helper)
    if (found) {
        _admin_save_customers_to_fd(fd);// This saves the global 'customers' array
        const char *success_msg = "Customer update completed.\n";
        send(sock, success_msg, strlen(success_msg), 0);
    } else {
        const char *error_msg = "Error: Customer not found.\n";
        send(sock, error_msg, strlen(error_msg), 0);
    }
    
    flock(fd, LOCK_UN);
    close(fd);
}

void modify_employee_details(int sock) {
    char old_username[50], new_username[50], new_password[50];
    char buffer[BUFFER_SIZE];
    int found = 0;

    // Get details from client
    recv(sock, buffer, BUFFER_SIZE, 0);
    sscanf(buffer, "%s %s %s", old_username, new_username, new_password);

    int fd = open(EMPLOYEE_FILE, O_RDWR);
    if (fd == -1) {
        const char *error_msg = "Error: Could not open employee file.\n";
        send(sock, error_msg, strlen(error_msg), 0);
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock employee file for update");
        close(fd);
        const char *error_msg = "Error: Database is busy. Try again.\n";
        send(sock, error_msg, strlen(error_msg), 0);
        return;
    }

    // 1. READ (using new admin helper)
    _admin_load_employees_from_fd(fd);

    // 2. MODIFY (in memory)
    for (int i = 0; i < emp_count; i++) {
        if (strcmp(employees[i].username, old_username) == 0) {
            found = 1;
            strcpy(employees[i].username, new_username);
            strcpy(employees[i].password, new_password);
            break;
        }
    }

    // 3. WRITE (back to file using new admin helper)
    if (found) {
        _admin_save_employees_to_fd(fd);
        const char *success_msg = "Employee update completed.\n";
        send(sock, success_msg, strlen(success_msg), 0);
    } else {
        const char *error_msg = "Error: Employee not found.\n";
        send(sock, error_msg, strlen(error_msg), 0);
    }
    
    flock(fd, LOCK_UN);
    close(fd);
}

void modify_user_details(int sock) {
    char choice_buffer[4];
    recv(sock, choice_buffer, sizeof(choice_buffer), 0);
    int choice = atoi(choice_buffer);

    if (choice == 1) {
        modify_customer_details(sock);
    } else if (choice == 2) {
        modify_employee_details(sock);
    } else {
        const char *error_msg = "Invalid user type selected.\n";
        send(sock, error_msg, strlen(error_msg), 0);
    }
}

void handle_admin_login(int sock) {
    char username[30], password[30];

    const char *username_prompt = "Enter username: ";
    send(sock, username_prompt, strlen(username_prompt), 0);
    recv(sock, username, sizeof(username), 0);
    username[strcspn(username, "\n")] = 0;

    const char *password_prompt = "Enter password: ";
    send(sock, password_prompt, strlen(password_prompt), 0);
    recv(sock, password, sizeof(password), 0);
    password[strcspn(password, "\n")] = 0;

    if (strcmp(username, admin_username) == 0 && strcmp(password, admin_password) == 0) {
        const char *success_msg = "Login successful!";
        send(sock, success_msg, strlen(success_msg), 0);

        while (1) {
            char choice[2];
            recv(sock, choice, sizeof(choice), 0);
            int menu_choice = atoi(choice);

            switch (menu_choice) {
                case 1:
                    view_all_customer(sock);
                    break;
                case 2:
                    add_customer(sock);
                    break;
                case 3:
                    remove_customer(sock);
                    break;
                case 4:
                    manage_user_roles(sock);
                    break;
                case 5: 
                    modify_user_details(sock);
                    break;
                case 6:
                    change_admin_password(sock);
                    break;
                case 7:
                    printf("Logging out...");
                    close(sock);
                    return;
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

void change_admin_password(int sock) {
    char old_password[50], new_password[50];
    char buffer[BUFFER_SIZE];

    const char *old_prompt = "Enter old password: ";
    send(sock, old_prompt, strlen(old_prompt), 0);
    int bytes = recv(sock, old_password, sizeof(old_password), 0);
    old_password[bytes] = '\0';
    old_password[strcspn(old_password, "\n")] = 0;

    if (strcmp(old_password, admin_password) != 0) {
        const char *error_msg = "Old password did not match.";
        send(sock, error_msg, strlen(error_msg), 0);
        return;
    }

    const char *new_prompt = "Password match! Enter new password: ";
    send(sock, new_prompt, strlen(new_prompt), 0);
    bytes = recv(sock, new_password, sizeof(new_password), 0);
    new_password[bytes] = '\0';
    new_password[strcspn(new_password, "\n")] = 0;

    // Update the password in memory
    strcpy(admin_password, new_password);

    const char *success_msg = "Password changed successfully!";
    send(sock, success_msg, strlen(success_msg), 0);
}


// Helper to load all managers
int _admin_load_managers_from_fd(int fd) {
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("Failed to lseek to start of manager file");
        return -1;
    }
    char buffer[BUFFER_SIZE * 4];
    char line[BUFFER_SIZE];
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

// Helper to save all managers
int _admin_save_managers_to_fd(int fd) {
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

// Helper to save one new manager
void save_new_manager(Manager new_manager) {
    int fd = open(MANAGER_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Failed to open managers file for appending");
        exit(1);
    }
    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock manager file");
        close(fd);
        exit(1);
    }
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s %s %d\n", 
             new_manager.username, new_manager.password, new_manager.id);
    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Failed to write new manager");
    }
    flock(fd, LOCK_UN);
    close(fd);
}


void add_Manager(int sock) {
    char message[BUFFER_SIZE];
    recv(sock, message, BUFFER_SIZE, 0);
    sscanf(message, "%s %s %d", new_manager.username, new_manager.password, &new_manager.id);
    
    // Check if manager exists (need is_manager_exists helper)
    // For now, we'll just add
    
    managers[manager_count++] = new_manager;
    save_new_manager(new_manager);
    const char *success_msg = "Manager added successfully!";
    send(sock, success_msg, strlen(success_msg), 0);
}

void remove_BankEmp(int sock) {
    char message[BUFFER_SIZE];
    int id_to_remove, found = 0;
    recv(sock, message, BUFFER_SIZE, 0);
    sscanf(message, "%d", &id_to_remove);

    int fd = open(EMPLOYEE_FILE, O_RDWR);
    if (fd == -1) { /* error */ return; }
    flock(fd, LOCK_EX);
    _admin_load_employees_from_fd(fd);

    for (int i = 0; i < emp_count; i++) {
        if (employees[i].id == id_to_remove) {
            found = 1;
            for (int j = i; j < emp_count - 1; j++) {
                employees[j] = employees[j + 1];
            }
            emp_count--;
            _admin_save_employees_to_fd(fd);
            break;
        }
    }
    flock(fd, LOCK_UN);
    close(fd);

    if (found) {
        send(sock, "Employee removed successfully!", 29, 0);
    } else {
        send(sock, "Employee ID not found!", 22, 0);
    }
}

void remove_Manager(int sock) {
    char message[BUFFER_SIZE];
    int id_to_remove, found = 0;
    recv(sock, message, BUFFER_SIZE, 0);
    sscanf(message, "%d", &id_to_remove);

    int fd = open(MANAGER_FILE, O_RDWR);
    if (fd == -1) { /* error */ return; }
    flock(fd, LOCK_EX);
    _admin_load_managers_from_fd(fd);

    for (int i = 0; i < manager_count; i++) {
        if (managers[i].id == id_to_remove) {
            found = 1;
            for (int j = i; j < manager_count - 1; j++) {
                managers[j] = managers[j + 1];
            }
            manager_count--;
            _admin_save_managers_to_fd(fd);
            break;
        }
    }
    flock(fd, LOCK_UN);
    close(fd);

    if (found) {
        send(sock, "Manager removed successfully!", 28, 0);
    } else {
        send(sock, "Manager ID not found!", 21, 0);
    }
}

void manage_user_roles(int sock) {
    char choice_buffer[4];
    recv(sock, choice_buffer, sizeof(choice_buffer), 0);
    int choice = atoi(choice_buffer);

    switch (choice) {
        case 1:
            add_BankEmp(sock);
            break;
        case 2:
            remove_BankEmp(sock);
            break;
        case 3:
            add_Manager(sock);
            break;
        case 4:
            remove_Manager(sock);
            break;
        default:
            send(sock, "Invalid choice.\n", 17, 0);
    }
}