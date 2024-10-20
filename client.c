#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUF_SIZE 1024

// Function to decode the message using the key
void decode_message(char *message, int key) {
    for (int i = 0; message[i] != '\0'; i++) {
        message[i] = message[i] - key;
    }
}

// Thread function to receive messages from the server
void *receive_messages(void *socket_desc) {
    int sock = *(int *)socket_desc;
    char buffer[BUF_SIZE];
    char message[BUF_SIZE];
    int key;

    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int bytes_received = recv(sock, buffer, BUF_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected.\n");
            close(sock);
            exit(EXIT_FAILURE);
        }

        buffer[bytes_received] = '\0';
        
        // Split the received message and the key (format: "encoded_message|key")
        sscanf(buffer, "%[^|]|%d", message, &key);
        
        // Print the encoded message and key
        printf("Encoded message received: %s\n", message);
        printf("Key received: %d\n", key);
        
        // Decode the message using the received key
        decode_message(message, key);
        printf("Decoded message: %s\n", message);
    }

    return NULL;
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char message[BUF_SIZE];
    pthread_t tid;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    // Get the server IP address from the user
    char server_ip[INET_ADDRSTRLEN];
    printf("Enter server IP address: ");
    fgets(server_ip, sizeof(server_ip), stdin);
    server_ip[strcspn(server_ip, "\n")] = 0;  // Remove newline character

    // Convert the IP address from string to binary form
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Type 'exit' to disconnect.\n");

    // Start receiving messages from the server in a separate thread
    pthread_create(&tid, NULL, receive_messages, (void *)&sock);

    // Main loop for sending messages
    while (1) {
        printf("Enter message: ");
        fgets(message, BUF_SIZE, stdin);
        message[strlen(message) - 1] = '\0';  // Remove newline character

        // Send message to the server
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Send failed");
            break;
        }

        // If the message is "exit", break out of the loop
        if (strcmp(message, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }
    }

    close(sock);
    return 0;
}
