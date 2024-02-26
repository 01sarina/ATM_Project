// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

struct Account 
{
    int ID;
    int PIN;
    float Balance;
};

// Function prototypes
void *client_handler(void *arg);
bool verify_id(int id);
bool verify_pin(int id, int pin);
void handle_balance(int client_sock, int id);
void handle_withdraw(int client_sock, int id);
void handle_deposit(int client_sock, int id);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t tid;

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) 
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Prepare the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345); // Port number

    // Bind the socket to the specified address and port
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_sock, MAX_CLIENTS) == -1) 
    {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening...\n");

    while (1) 
    {
        // Accept a new connection
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock == -1) 
        {
            perror("Accept failed");
            continue;
        }

        printf("New client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Create a new thread to handle the client
        if (pthread_create(&tid, NULL, client_handler, (void *)&client_sock) != 0) 
        {
            perror("Thread creation failed");
            close(client_sock);
            continue;
        }

        // Detach the thread, as we won't be joining it later
        pthread_detach(tid);
    }

    return 0;
}

void *client_handler(void *arg) 
{
    int client_sock = *(int *)arg;
    int id, pin, option;

    // Receive ID from client
    if (recv(client_sock, &id, sizeof(id), 0) == -1) 
    {
        perror("Receive ID failed");
        close(client_sock);
        return NULL;
    }

    // Verify ID from the database
    if (!verify_id(id)) 
    {
        char error_msg[] = "Invalid ID";
        send(client_sock, error_msg, sizeof(error_msg), 0);
        close(client_sock);
        return NULL;
    }
    else
    {
        char msg1[] = "Given ID is Correct";
        send(client_sock, msg1, sizeof(msg1), 0);
    }

    // Receive PIN from the client
    if (recv(client_sock, &pin, sizeof(pin), 0) == -1) 
    {
        perror("Receive PIN failed");
        close(client_sock);
        return NULL;
    }

    // Verify PIN from the database
    if (!verify_pin(id, pin)) {
        char error_msg[] = "Invalid PIN";
        send(client_sock, error_msg, sizeof(error_msg), 0);
        close(client_sock);
        return NULL;
    }
    else
    {
        char msg2[] = "Given PIN is Correct"; 
        send(client_sock, msg2, sizeof(msg2), 0);
    }

    // Receive option from the client
    if (recv(client_sock, &option, sizeof(option), 0) == -1) {
        perror("Receive option failed");
        close(client_sock);
        return NULL;
    }

    char error_msg[] = "Invalid option";
    switch (option) {
        case 1:
            handle_balance(client_sock, id);
            break;
        case 2:
            handle_withdraw(client_sock, id);
            break;
        case 3:
            handle_deposit(client_sock, id);
            break;
        default:
            
            send(client_sock, error_msg, sizeof(error_msg), 0);
            break;
    }

    close(client_sock);
    return NULL;
}

bool verify_id(int id) 
{
    FILE *file = fopen("database.txt", "r");
    if (!file) 
    {
        perror("Error opening database file");
        return false;
    }

    int file_id;
    while (fscanf(file, "%d %*d %*f", &file_id) == 1) 
    {
        if (file_id == id) 
        {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

bool verify_pin(int id, int pin) 
{
    FILE *file = fopen("database.txt", "r");
    if (!file) 
    {
        perror("Error opening database file");
        return false;
    }

    int file_id, file_pin;
    while (fscanf(file, "%d %d %*f", &file_id, &file_pin) == 2) 
    {
        if (file_id == id && file_pin == pin) 
        {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

void handle_balance(int client_sock, int id) 
{
    // Read balance from the database.txt file
    FILE *file = fopen("database.txt", "r");
    if (!file) 
    {
        perror("Error opening database file");
        close(client_sock);
        return;
    }

    int file_id;
    float balance;
    while (fscanf(file, "%d %*d %f", &file_id, &balance) == 2) 
    {
        if (file_id == id) 
        {
            send(client_sock, &balance, sizeof(balance), 0);
            fclose(file);
            return;
        }
    }

    fclose(file);
}

void handle_withdraw(int client_sock, int id) {
    // Receive withdrawal amount from the client
    float amount;int pin;
    if (recv(client_sock, &amount, sizeof(amount), 0) == -1) {
        perror("Receive amount failed");
        char error_msg[]="Error in server";
        send(client_sock, error_msg, sizeof(error_msg), 0);
        close(client_sock);
        return;
    }

    // Read balance from the database.txt file
    FILE *file = fopen("database.txt", "r");
    if (!file) {
        perror("Error opening database file");
        char error_msg[]="Error in server";
        send(client_sock, error_msg, sizeof(error_msg), 0);
        close(client_sock);
        return;
    }

    int file_id;
    float balance;
    while (fscanf(file, "%d %*d %f", &file_id, &balance) == 2) {
        if (file_id == id) {
            if (balance >= amount) {
                balance -= amount;
                // Update the balance in the database.txt file
                FILE *update_file = fopen("database.txt", "r");
                if (!update_file) {
                    perror("Error opening database file");
                    char error_msg[]="Error in server";
                    send(client_sock, error_msg, sizeof(error_msg), 0);
                    close(client_sock);
                    fclose(file);
                    return;
                }

                FILE *temp_file = fopen("temp.txt", "w");
                if (!temp_file) {
                    perror("Error opening temp file");
                    char error_msg[]="Error in server";
                    send(client_sock, error_msg, sizeof(error_msg), 0);
                    close(client_sock);
                    fclose(file);
                    fclose(update_file);
                    return;
                }

                int update_id;
                float update_balance;
                while (fscanf(update_file, "%d %d %f", &update_id, &pin, &update_balance) != EOF) {
                    if (update_id == id) {
                        fprintf(temp_file, "%d %d %.2f\n", id, pin, balance);
                    } else {
                        fprintf(temp_file, "%d %d %.2f\n", update_id, pin, update_balance);
                    }
                }

                fclose(update_file);
                fclose(temp_file);

                if (remove("database.txt") != 0) {
                    perror("Error deleting database file");
                    char error_msg[]="Error in server";
                    send(client_sock, error_msg, sizeof(error_msg), 0);
                    close(client_sock);
                    fclose(file);
                    return;
                }

                if (rename("temp.txt", "database.txt") != 0) {
                    perror("Error renaming file");
                    char error_msg[]="Error in server";
                    send(client_sock, error_msg, sizeof(error_msg), 0);
                    close(client_sock);
                    return;
                }

               // send(client_sock, &amount, sizeof(amount), 0);
            } else {
                char error_msg[] = "Insufficient balance";
                send(client_sock, error_msg, sizeof(error_msg), 0);
            }
            break;
        }
    }

    fclose(file);
}

void handle_deposit(int client_sock, int id) {
    // Receive deposit amount from the client
    float amount;int pin;
    if (recv(client_sock, &amount, sizeof(amount), 0) == -1) {
        perror("Receive amount failed");
        char error_msg[]="Error in server";
        send(client_sock, error_msg, sizeof(error_msg), 0);
        close(client_sock);
        return;
    }

    // Read balance from the database.txt file
    FILE *file = fopen("database.txt", "r");
    if (!file) {
        perror("Error opening database file");
        char error_msg[]="Error in server";
        send(client_sock, error_msg, sizeof(error_msg), 0);
        close(client_sock);
        return;
    }

    int file_id;
    float balance;
    while (fscanf(file, "%d %*d %f", &file_id, &balance) == 2) {
        if (file_id == id) {
            balance += amount;
            // Update the balance in the database.txt file
            FILE *update_file = fopen("database.txt", "r");
            if (!update_file) {
                perror("Error opening database file");
                char error_msg[]="Error in server";
                send(client_sock, error_msg, sizeof(error_msg), 0);
                close(client_sock);
                fclose(file);
                return;
            }

            FILE *temp_file = fopen("temp.txt", "w");
            if (!temp_file) {
                perror("Error opening temp file");
                char error_msg[]="Error in server";
                send(client_sock, error_msg, sizeof(error_msg), 0);
                close(client_sock);
                fclose(file);
                fclose(update_file);
                return;
            }

            int update_id;
            float update_balance;
            while (fscanf(update_file, "%d %d %f", &update_id, &pin, &update_balance) != EOF) {
                if (update_id == id) {
                    fprintf(temp_file, "%d %d %.2f\n", id, pin, balance);
                } else {
                    fprintf(temp_file, "%d %d %.2f\n", update_id, pin, update_balance);
                }
            }

            fclose(update_file);
            fclose(temp_file);

            if (remove("database.txt") != 0) {
                perror("Error deleting database file");
                char error_msg[]="Error in server";
                send(client_sock, error_msg, sizeof(error_msg), 0);
                close(client_sock);
                fclose(file);
                return;
            }

            if (rename("temp.txt", "database.txt") != 0) {
                perror("Error renaming file");
                char error_msg[]="Error in server";
                send(client_sock, error_msg, sizeof(error_msg), 0);
                close(client_sock);
                return;
            }

            //send(client_sock, &amount, sizeof(amount), 0);
            break;
        }
    }

    fclose(file);
}