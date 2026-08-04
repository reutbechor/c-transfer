#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 5000
#define BUFFER_SIZE 1024

int main(void)
{
    int server_socket;
    int client_socket;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    socklen_t client_address_length = sizeof(client_address);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    FILE *output_file;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(
            server_socket,
            (struct sockaddr *)&server_address,
            sizeof(server_address)) == -1) {
        perror("bind");
        close(server_socket);
        return EXIT_FAILURE;
    }

    if (listen(server_socket, 1) == -1) {
        perror("listen");
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Receiver is waiting on port %d...\n", PORT);

    client_socket = accept(
        server_socket,
        (struct sockaddr *)&client_address,
        &client_address_length
    );

    if (client_socket == -1) {
        perror("accept");
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf(
        "Sender connected from %s\n",
        inet_ntoa(client_address.sin_addr)
    );

    output_file = fopen("received.txt", "wb");

    if (output_file == NULL) {
        perror("fopen");
        close(client_socket);
        close(server_socket);
        return EXIT_FAILURE;
    }

    while ((bytes_received = recv(
                client_socket,
                buffer,
                sizeof(buffer),
                0)) > 0) {

        size_t bytes_written = fwrite(
            buffer,
            1,
            bytes_received,
            output_file
        );

        if (bytes_written != (size_t)bytes_received) {
            perror("fwrite");
            fclose(output_file);
            close(client_socket);
            close(server_socket);
            return EXIT_FAILURE;
        }
    }

    if (bytes_received == -1) {
        perror("recv");
        fclose(output_file);
        close(client_socket);
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("File received successfully\n");

    fclose(output_file);
    close(client_socket);
    close(server_socket);

    return EXIT_SUCCESS;
}