#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "Customer/customer.h"
#include "Admin/admin.h" 
#include "Employee/employee.h" 
#include "Manager/manager.h"

#define PORT 8080
#define BUFFER_SIZE 1024

void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    free(socket_desc);
    char client_message[BUFFER_SIZE];
    int choice; 
    recv(sock, client_message, BUFFER_SIZE, 0);
    choice = atoi(client_message);
    memset(client_message, 0, BUFFER_SIZE);
    
    if (choice == 1) {
        handle_admin_login(sock);  
    }
    else if(choice == 2){
        handle_manager_login(sock); 
    }
    else if (choice == 3) {
        handle_employee_login(sock); 
    }
    else if (choice == 4) {
        handle_customer_login(sock); 
    }
    close(sock);
    return NULL;
}

int main() {
    int server_socket, client_socket, *new_sock;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("Could not create socket\n");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }
    listen(server_socket, 3);
    printf("Waiting for incoming connections...\n");

    while ((client_socket = accept(server_socket, (struct sockaddr *)&client, &client_len))) {
        printf("Connection accepted\n");

        // Handle each client in a separate thread
        new_sock = malloc(1);
        *new_sock = client_socket;
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, client_handler, (void*)new_sock) < 0) {
            perror("Could not create thread");
            return 1;
        }
    }

    if (client_socket < 0) {
        perror("Accept failed");
        return 1;
    }

    close(server_socket);
    return 0;
}
