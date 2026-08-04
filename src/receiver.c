#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 5000
#define BUFFER_SIZE 1024
#define MAX_FILE_NAME_LENGTH 255

static int receive_all(int socket_fd, void *data, size_t data_size)
{
    char *current = data;
    size_t total_received = 0;

    while (total_received < data_size) {
        ssize_t bytes_received = recv(
            socket_fd,
            current + total_received,
            data_size - total_received,
            0
        );

        if (bytes_received == 0) {
            fprintf(stderr, "Connection closed unexpectedly\n");
            return -1;
        }

        if (bytes_received < 0) {
            perror("recv");
            return -1;
        }

        total_received += (size_t)bytes_received;
    }

    return 0;
}

int main(void)
{
    int server_socket;
    int client_socket;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    socklen_t client_address_length = sizeof(client_address);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int reuse_address = 1;

    if (setsockopt(
            server_socket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse_address,
            sizeof(reuse_address)) == -1) {
        perror("setsockopt");
        close(server_socket);
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

    /*
     * Receive the file-name length first.
     */
    uint32_t network_name_length;

    if (receive_all(
            client_socket,
            &network_name_length,
            sizeof(network_name_length)) == -1) {
        close(client_socket);
        close(server_socket);
        return EXIT_FAILURE;
    }

    uint32_t file_name_length = ntohl(network_name_length);

    if (file_name_length == 0 ||
        file_name_length > MAX_FILE_NAME_LENGTH) {
        fprintf(stderr, "Invalid file-name length\n");
        close(client_socket);
        close(server_socket);
        return EXIT_FAILURE;
    }

    /*
     * Receive the file name and terminate it as a C string.
     */
    char file_name[MAX_FILE_NAME_LENGTH + 1];

    if (receive_all(
            client_socket,
            file_name,
            file_name_length) == -1) {
        close(client_socket);
        close(server_socket);
        return EXIT_FAILURE;
    }

    file_name[file_name_length] = '\0';

    /*
     * Add a prefix so the received file does not overwrite
     * the original when testing on the same computer.
     */
    char output_path[MAX_FILE_NAME_LENGTH + 20];

    int result = snprintf(
        output_path,
        sizeof(output_path),
        "received_%s",
        file_name
    );

    if (result < 0 || (size_t)result >= sizeof(output_path)) {
        fprintf(stderr, "Output file name is too long\n");
        close(client_socket);
        close(server_socket);
        return EXIT_FAILURE;
    }

    FILE *output_file = fopen(output_path, "wb");

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
            (size_t)bytes_received,
            output_file
        );

        if (bytes_written != (size_t)bytes_received) {
            fprintf(stderr, "Could not write the complete data\n");
            fclose(output_file);
            close(client_socket);
            close(server_socket);
            return EXIT_FAILURE;
        }
    }

    if (bytes_received < 0) {
        perror("recv");
        fclose(output_file);
        close(client_socket);
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("File saved as '%s'\n", output_path);

    fclose(output_file);
    close(client_socket);
    close(server_socket);

    return EXIT_SUCCESS;
}