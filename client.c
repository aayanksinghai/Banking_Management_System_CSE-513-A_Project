#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void show_customer_menu(int sock, const char* username);
void show_admin_menu(int sock, const char* username);
void show_employee_menu(int sock, const char* username);
void show_manager_menu(int sock, const char* username);

int main() {
    int sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE], server_reply[BUFFER_SIZE];
    int choice;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket\n");
        return 1;
    }
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        return 1;
    }
    printf("Connected to server\n");
        printf("***************Banking Management System***************\n To get started select a role:\n1. Admin\n2. Manager\n3. Employee\n4. Customer\n");
        memset(message,0,BUFFER_SIZE);
        memset(server_reply,0,BUFFER_SIZE);
        scanf("%d", &choice);
        getchar(); 
        // Send the role choice to the server
        sprintf(message, "%d", choice);
        send(sock, message, strlen(message), 0);    
        // Handle admin login and menu interaction
        if (choice == 1) {
            // Receive and respond to server prompts for username and password
            while (1) {
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s", server_reply);
                fgets(message, BUFFER_SIZE, stdin);  // Enter username or password
                send(sock, message, strlen(message), 0);
                if (strstr(server_reply, "password") != NULL) break;
            }
            // Receive server response for login
            recv(sock, server_reply, BUFFER_SIZE, 0);
            printf("%s\n", server_reply);
            if (strstr(server_reply, "successful")) 
                show_admin_menu(sock, message);  
        }
        else if (choice==2){
            // Receive and respond to server prompts for username and password
            while (1) {
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s", server_reply);
                fgets(message, BUFFER_SIZE, stdin);  // Enter username or password
                send(sock, message, strlen(message), 0);
                if (strstr(server_reply, "password") != NULL) break;
            }

            // Receive server response for login
            recv(sock, server_reply, BUFFER_SIZE, 0);
            printf("%s\n", server_reply);

            if (strstr(server_reply, "successful"))
                show_manager_menu(sock, message);
        }

        else if (choice == 3){
            while (1) {
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s", server_reply);
                fgets(message, BUFFER_SIZE, stdin);  // Enter username or password
                send(sock, message, strlen(message), 0);
                if (strstr(server_reply, "password") != NULL) break;
            }
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply);
                if (strstr(server_reply, "successful"))
                    show_employee_menu(sock, message);  
        }
        // Handle customer login and menu interaction
        else if (choice == 4) {
            while (1) {
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s", server_reply);
                fgets(message, BUFFER_SIZE, stdin);  
                send(sock, message, strlen(message), 0);
                if (strstr(server_reply, "password") != NULL) break;
            }
            recv(sock, server_reply, BUFFER_SIZE, 0);
            printf("%s\n", server_reply);
            if (strstr(server_reply, "successful")) 
                show_customer_menu(sock, message);  
        }
    close(sock);
    return 0;
}

void show_customer_menu(int sock, const char* username) {
    int choice;
    char message[BUFFER_SIZE];
    char server_reply[BUFFER_SIZE];
    float amount;

    while (1) {
        printf("Customer Menu:\n"
               "1. View Account Balance\n"
               "2. Deposit Money\n"
               "3. Withdraw Money\n"
               "4. Transfer Funds\n"
               "5. Apply for a Loan\n"
               "6. Change Password\n"
               "7. Add Feedback\n"
               "8. View Transaction History\n"
               "9. Logout\n"
               "Enter your choice: ");
        scanf("%d", &choice);
        sprintf(message, "%d", choice); 
        send(sock, message, strlen(message), 0);

        switch (choice) {
            case 1:  // View Account Balance
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply); 
                memset(server_reply, 0, BUFFER_SIZE);
                break;

            case 2: // Deposit Money
                printf("Enter amount to deposit: ");
                scanf("%f", &amount);
                snprintf(message, sizeof(message), "%.2f", amount);
                send(sock, message, strlen(message), 0);
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply); 
                break;
            case 3: // Withdraw Money
                printf("Enter amount to withdraw: ");
                scanf("%f", &amount);
                snprintf(message, sizeof(message), "%.2f", amount);
                send(sock, message, strlen(message), 0);
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply); 
                break;
            case 4:  // Transfer Funds
                int target_account_number;
                float transfer_amount;
                printf("Enter target account number: ");
                scanf("%d", &target_account_number);
                printf("Enter amount to transfer: ");
                scanf("%f", &transfer_amount);
                snprintf(message, sizeof(message), "%d %.2f", target_account_number, transfer_amount);
                send(sock, message, strlen(message), 0);
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply);  // Display server response
                break;

            case 5:   // Apply for a Loan
                float loan_amount, monthly_income;
                char loan_purpose[50], employment_status[50];
                int contact_info;
                printf("Enter loan amount: ");
                scanf("%f", &loan_amount);
                getchar();  // Consume newline
                
                printf("Enter loan purpose: ");
                fgets(loan_purpose, sizeof(loan_purpose), stdin);
                loan_purpose[strcspn(loan_purpose, "\n")] = 0;  // Remove newline
                
                printf("Enter monthly income: ");
                scanf("%f", &monthly_income);
                getchar();  // Consume newline
                
                printf("Enter employment status: ");
                fgets(employment_status, sizeof(employment_status), stdin);
                employment_status[strcspn(employment_status, "\n")] = 0;  // Remove newline
                
                printf("Enter contact information: ");
                scanf("%d", &contact_info); 
                snprintf(message, sizeof(message), "%.2f %s %.2f %s %d", 
                        loan_amount, 
                        loan_purpose, 
                        monthly_income, 
                        employment_status, 
                        contact_info);
                send(sock, message, strlen(message), 0);

                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply); 

                break;

            case 6:   // Change Password
                char old_password[50], new_password[50];
                printf("Enter old password: ");
                scanf("%s", old_password);
                send(sock, old_password, strlen(old_password), 0);
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply);  // Password match or mismatch
                if (strstr(server_reply, "Password match")) {
                    printf("Enter new password: ");
                    scanf("%s", new_password);
                    send(sock, new_password, strlen(new_password), 0);

                    recv(sock, server_reply, BUFFER_SIZE, 0);
                    printf("%s\n", server_reply);  // Confirmation message
                }
                break;

            case 7:  // Add Feedback
                char feedback[500];
                printf("Enter your feedback: ");
                getchar();  // Consume newline
                fgets(feedback, sizeof(feedback), stdin);
                feedback[strcspn(feedback, "\n")] = 0;  // Remove newline

                send(sock, feedback, strlen(feedback), 0);

                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply);  // Confirmation message
                break;

            case 8:  // View Transaction History
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply);  // Display transaction history
                break;

            case 9:  // Logout
                printf("Logging out...\n");
                exit(0);  

            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }
    }
}

void show_admin_menu(int sock, const char* username){
    int choice,bytes_received,id_to_remove;
    char message[BUFFER_SIZE];
    char server_reply[BUFFER_SIZE];

    while (1) {
        printf("Admin Menu:\n"
               "1. View All Customers\n"
               "2. Add new Customer\n"
               "3. Remove Customer\n"
               "4. Add Bank Employee\n"
               "5. Logout\n"
               "Enter your choice: ");
        scanf("%d", &choice);
        sprintf(message, "%d", choice); 
        send(sock, message, strlen(message), 0);

        switch (choice) {
            case 1:  
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply); // Display customers received from server
                memset(server_reply, 0, BUFFER_SIZE);
                break;

            case 2: 
                printf("Enter new customer details (username password balance id): ");
                scanf(" %[^\n]", message); 
                send(sock, message, strlen(message), 0); 
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply); 
                memset(server_reply, 0, BUFFER_SIZE);
                break;            
            case 3:
                printf("Enter customer ID to remove: ");
                scanf("%d", &id_to_remove);
                snprintf(message, sizeof(message), "%d", id_to_remove);
                send(sock, message, strlen(message), 0); 
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply); 
                memset(server_reply, 0, BUFFER_SIZE);
                break;
            case 4:
                printf("Enter new Employee details (username password empid):");
                scanf(" %[^\n]", message); 
                message[strcspn(message, "\n")] = 0;
                send(sock, message, strlen(message), 0); 
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply); 
                memset(server_reply, 0, BUFFER_SIZE);
                break;
            case 5:
                printf("Logging Out...\n");
                exit(0);
            default:
                printf("Invalid choice, please try again.\n");
        }
    }
}

void show_employee_menu(int sock, const char* username){
    int choice;
    char message[BUFFER_SIZE];
    char server_reply[BUFFER_SIZE];

    while (1) {
        printf("Employee Menu:\n"
               "1. View All Customers\n"
               "2. Loan Processing\n"
               "3. Add Customer\n"
               "4. Remove Customer\n"
               "5. Update Customer\n"
               "6. Logout\n"
               "Enter your choice: ");
        scanf("%d", &choice);
        sprintf(message, "%d", choice);  
        send(sock, message, strlen(message), 0);

        switch (choice) {
            case 1:
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply);
                memset(server_reply, 0, BUFFER_SIZE);
                break;
            case 2:
                recv(sock, message, BUFFER_SIZE, 0);
                printf("%s", message); 

                while (1) {
                    memset(message, 0, sizeof(message));                    
                    // Receive loan application result (approval/rejection) from server
                    int bytes_received = recv(sock, message, BUFFER_SIZE, 0);
                    if (bytes_received <= 0 )
                        break;
                    printf("%s", message); 
                    if (strstr(message, "Loan processing completed")) 
                        break;
                }
                break; 

            case 3:
                printf("Enter new customer details (username password balance id): ");
                scanf(" %[^\n]", message);
                send(sock, message, strlen(message), 0); 
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply); 
                memset(server_reply, 0, BUFFER_SIZE);
                break;

            case 4:
                printf("Enter customer ID to remove: ");
                scanf("%s", message);
                send(sock, message, strlen(message), 0);
                recv(sock, server_reply, BUFFER_SIZE, 0);
                printf("%s\n", server_reply); 
                memset(server_reply, 0, BUFFER_SIZE);
                break;

            case 5:
                char old_username[50], new_username[50], new_password[50];
                printf("Enter the current username of the customer: ");
                scanf("%s", old_username);
                printf("Enter the new username: ");
                scanf("%s", new_username);
                printf("Enter the new password: ");
                scanf("%s", new_password);
                snprintf(message, sizeof(message), "UPDATE_CUSTOMER %s %s %s", old_username, new_username, new_password);
                send(sock, message, strlen(message), 0);
                while (1) {
                    memset(message, 0, sizeof(message));                    
                    int bytes_received = recv(sock, message, BUFFER_SIZE, 0);
                    if (bytes_received <= 0 )
                        break;
                    printf("%s", message); 
                    if (strstr(message, "Customer update completed.")) 
                        break;
                }
                break; 

            case 6:
                // Logout
                printf("Logging out...\n");
                exit(0);

            default:
                printf("Invalid choice, please try again.\n");
        }
    }
}
void show_manager_menu(int sock, const char* username){
    int choice;
    int customer_id, bytes_received;
    char message[BUFFER_SIZE];
    char server_reply[BUFFER_SIZE];

    while (1) {
        printf("Manager Menu:\n"
               "1. Activate Customer Account\n"
               "2. Deactivate Customer Account\n"
               "3. Assign Loan Application to Employee\n"
               "4. Review Feedback \n"
               "5. Logout\n"
               "Enter your choice: ");
        scanf("%d", &choice);
        sprintf(message, "%d", choice);  
        send(sock, message, strlen(message), 0);

        switch (choice) {
            case 1:  
                printf("Enter the username to activate: ");
                getchar(); // Clear buffer
                fgets(message, BUFFER_SIZE, stdin);
                message[strcspn(message, "\n")] = 0;  // Remove newline character
                send(sock, message, strlen(message), 0);  
                recv(sock, server_reply, BUFFER_SIZE, 0);  
                printf("%s\n", server_reply);
                break;
            case 2:
                printf("Enter the username to deactivate: ");
                getchar(); // Clear buffer
                fgets(message, BUFFER_SIZE, stdin);
                message[strcspn(message, "\n")] = 0;  // Remove newline character
                send(sock, message, strlen(message), 0);  
                recv(sock, server_reply, BUFFER_SIZE, 0);  
                printf("%s\n", server_reply);
                break;
            case 3:
                while ((bytes_received = recv(sock, server_reply, BUFFER_SIZE - 1, 0)) > 0) {
                    server_reply[bytes_received] = '\0';  
                    printf("%s", server_reply);
                    if (bytes_received < BUFFER_SIZE - 1) 
                        break;
                }
                memset(server_reply, 0, BUFFER_SIZE);
                break;
            case 4: 
                while ((bytes_received = recv(sock, server_reply, BUFFER_SIZE - 1, 0)) > 0) {
                    server_reply[bytes_received] = '\0';  
                    printf("%s", server_reply);
                    if (strstr(server_reply, "All feedbacks reviewed") != NULL)
                        break; 
                }
                break;
            case 5:
                // Logout
                printf("Logging out...\n");
                exit(0);
            default:
                printf("Invalid choice, please try again.\n");
        }
    }
}