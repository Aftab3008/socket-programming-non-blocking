#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUF_SIZE 1024
#define MAX_CLIENTS 10

int clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void encode_message(char *message, int key) {
    for (int i = 0; message[i] != '\0'; i++) {
        message[i] = message[i] + key;
    }
}

void send_message_to_all_clients(char *message, int key, int sender_sock) {
    pthread_mutex_lock(&clients_mutex);
    char encoded_msg[BUF_SIZE];
    char final_msg[BUF_SIZE];

    strcpy(encoded_msg, message);
    encode_message(encoded_msg, key);

    // Print the encoded message and the key
    printf("Encoded message: %s\n", encoded_msg);
    printf("Encoding key: %d\n", key);
    
    // Create a string containing the encoded message and the key safely
    int bytes_written = snprintf(final_msg, BUF_SIZE, "%s|%d", encoded_msg, key);

    // Check if snprintf truncated the output
    if (bytes_written < 0 || bytes_written >= BUF_SIZE) {
        printf("Error: Message truncated. Encoding might not be sent properly.\n");
        pthread_mutex_unlock(&clients_mutex);
        return;
    }

    // Send to all clients, including the sender
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0) {
            send(clients[i], final_msg, strlen(final_msg), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    char buffer[BUF_SIZE];
    int key = 3; // Simple key for encoding/decoding

    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int bytes_received = recv(client_sock, buffer, BUF_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected.\n");
            close(client_sock);
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == client_sock) {
                    clients[i] = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            break;
        }

        buffer[bytes_received] = '\0';
        if (strcmp(buffer, "exit") == 0) {
            printf("Client requested exit.\n");
            close(client_sock);
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == client_sock) {
                    clients[i] = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            break;
        }

        printf("Received message: %s\n", buffer);
        send_message_to_all_clients(buffer, key, client_sock);
    }

    free(arg);
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    pthread_t tid;

    // Initialize clients array
    memset(clients, 0, sizeof(clients));

    // Create server socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Allow connections from any IP
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, 3) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        client_addr_size = sizeof(client_addr);
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_size)) < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == 0) {
                clients[i] = client_sock;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        pthread_create(&tid, NULL, handle_client, (void *)new_sock);
    }

    return 0;
}
