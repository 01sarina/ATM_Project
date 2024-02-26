// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) 
{
    if (argc != 2) 
    {
        printf("Usage: %s <ID>\n", argv[0]);
        return 1;
    }

    int id = atoi(argv[1]);
    int server_sock;
    struct sockaddr_in server_addr;

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
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Replace with server IP address
    server_addr.sin_port = htons(12345); // Port number

    // Connect to the server
    if (connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Send ID to the server
    send(server_sock, &id, sizeof(id), 0);

    char msg1[BUFFER_SIZE];
    recv(server_sock, &msg1, sizeof(msg1), 0);
    printf("%s\n",msg1);
    if(strcmp(msg1,"Invalid ID")==0)
    {
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    int pin;
    printf("Enter the PIN\n");
    scanf("%d",&pin);
    send(server_sock, &pin, sizeof(pin), 0);

    char msg2[BUFFER_SIZE];
    recv(server_sock, &msg2, sizeof(msg2), 0);
    printf("%s\n",msg2);
    if(strcmp(msg2,"Invalid PIN")==0)
    {
        close(server_sock);
        exit(EXIT_FAILURE);
    }


    // Receive and process server response based on option selected
    int option;
    printf("Select an option:\n");
    printf("1. Check Balance\n");
    printf("2. Withdraw\n");
    printf("3. Deposit\n");
    scanf("%d", &option);
    send(server_sock, &option, sizeof(option), 0);

    switch (option) {
        case 1: {
            float balance;
            recv(server_sock, &balance, sizeof(balance), 0);
            printf("Balance: %.2f\n", balance);
            break;
        }
        case 2: {
            float amount;
            printf("Enter the amount to withdraw: ");
            scanf("%f", &amount);
            send(server_sock, &amount, sizeof(amount), 0);

            char msg[BUFFER_SIZE];
            recv(server_sock, msg, BUFFER_SIZE, 0);
            
            if((strcmp(msg,"Insufficient balance")==0) || (strcmp(msg,"Error in server")==0))
            printf("%s\n",msg);
            else
            printf("%.2f is debited\n",amount);
            break;
        }
        case 3: {
            float amount;
            printf("Enter the amount to deposit: ");
            scanf("%f", &amount);
            send(server_sock, &amount, sizeof(amount), 0);

            char msg[BUFFER_SIZE];
            recv(server_sock, msg, BUFFER_SIZE, 0);
            if(strcmp(msg,"Error in server")!=0)
            printf("%.2f is credited\n",amount);
            break;
        }
        default:
            printf("Invalid option\n");
            break;
    }

    close(server_sock);
    return 0;
}
