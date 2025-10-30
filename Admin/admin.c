#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/file.h>
#include <fnctl.h>
#include "admin.h"
#include "../Employee/employee.h"
#include "../Customer/customer.h"

#define BUFFER_SIZE 1024
#define MAX_CUSTOMERS 100
#define CUSTOMER_FILE "Customer/customers.txt"
#define EMPLOYEE_FILE "Employee/employees.txt"

const char admin_username[] = "root";
const char admin_password[] = "root";

Employee employees[100];
int emp_count = 0;
Employee new_employee;


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
        if(errono == ENOENT){
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

void load_customers() {

    int fd = open(CUSTOMER_FILE, O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) { // File not found
             fd = open(CUSTOMER_FILE, O_CREAT, 0644); // Create file
             if(fd != -1) close(fd);
             customer_count = 0; // No customers yet
             return;
        }
        perror("Failed to open customers file");
        send(sock, "Error: Could not open customer database.\n", 40, 0);
        return;
    }

    if (flock(fileno(file), LOCK_SH) == -1) {
        perror("Failed to lock customer file");
        close(fd);
        send(sock, "Error: Could not lock customer database.\n", 40, 0);
        return;
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

    flock(fd, LOCK_UN);
    close(fd);

    /*
    FILE *file = fopen(CUSTOMER_FILE, "r");
    if (file == NULL) {
        perror("Failed to open customers file");
        exit(1);
    }

    flock(fileno(file), LOCK_SH);

    customer_count = 0;
    while (fscanf(file, "%s %s %f %d %d", customers[customer_count].username, customers[customer_count].password, 
                  &customers[customer_count].balance, &customers[customer_count].id, &customers[customer_count].is_active) != EOF) {
        customer_count++;
    }
    flock(fileno(file), LOCK_UN);
    fclose(file);

    */
}

void save_customers() {

    // Open with flags: Write-Only, Create if not exist, Truncate (clear) file
    int fd = open(CUSTOMER_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Failed to open customers file for writing");
        exit(1);
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Failed to lock customers file");
        close(fd);
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    for (int i = 0; i < customer_count; i++) {
        snprintf(buffer, sizeof(buffer), "%s %s %.2f %d %d\n", 
                 customers[i].username, customers[i].password, 
                 customers[i].balance, customers[i].id, customers[i].is_active);
        
        if (write(fd, buffer, strlen(buffer)) == -1) {
            perror("Failed to write customer to file");
            // Decide if we should break or continue
        }
    }

    flock(fd, LOCK_UN);
    close(fd);

    /*
    FILE *file = fopen(CUSTOMER_FILE, "w");
    if (file == NULL) {
        perror("Failed to open customers file for writing");
        exit(1);
    }

    flock(fileno(file), LOCK_EX);

    for (int i = 0; i < customer_count; i++) {
        fprintf(file, "%s %s %.2f %d %d\n", customers[i].username, customers[i].password, 
                customers[i].balance, customers[i].id, customers[i].is_active);
    }
    flock(fileno(file), LOCK_UN);
    fclose(file);
    */
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

    load_customers(sock);
    // Check if load_customers set count to 0 (e.g., file was empty)
    if (customer_count == 0) {
        const char *no_cust_msg = "No customers in the database.\n";
        send(sock, no_cust_msg, strlen(no_cust_msg), 0);
        return;
    }

    const char *header = "Customers:\n";
    send(sock, header, strlen(header), 0);
        for (int i = 0; i < customer_count; i++) {
        snprintf(buffer, BUFFER_SIZE, "%s %s %.2f %d %d\n", customers[i].username, customers[i].password, 
                 customers[i].balance, customers[i].id, customers[i].is_active);
        send(sock, buffer, strlen(buffer), 0);
    }
}

void remove_customer(int sock) {
    char message[BUFFER_SIZE];
    int id_to_remove, found = 0;

    recv(sock, message, BUFFER_SIZE, 0);
    sscanf(message, "%d", &id_to_remove);

    load_customers(sock);

    for (int i = 0; i < customer_count; i++) {
        if (customers[i].id == id_to_remove) {
            found = 1;
            for (int j = i; j < customer_count - 1; j++) {
                customers[j] = customers[j + 1];
            }
            customer_count--;
            save_customers();
            const char *success_msg = "Customer removed successfully!";
            send(sock, success_msg, strlen(success_msg), 0);
            return;
        }
    }

    if (!found) {
        const char *error_msg = "Customer ID not found!";
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
                    add_BankEmp(sock);
                    break;
                case 5:
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