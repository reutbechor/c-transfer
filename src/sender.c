#include <arpa/inet.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 5000
#define BUFFER_SIZE 1024

static int send_all(int socket_fd, const void *data, size_t data_size)
{
    const char *current = data;
    size_t total_sent = 0;

    while (total_sent < data_size) {
        ssize_t bytes_sent = send(
            socket_fd,
            current + total_sent,
            data_size - total_sent,
            0
        );

        if (bytes_sent <= 0) {
            perror("send");
            return -1;
        }

        total_sent += (size_t)bytes_sent;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int socket_fd;
    struct sockaddr_in server_address;

    FILE *file;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *file_path = argv[1];

    file = fopen(file_path, "rb");

    if (file == NULL) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    /*
     * basename() may modify its argument, so we copy the path first.
     */
    char path_copy[1024];

    if (strlen(file_path) >= sizeof(path_copy)) {
        fprintf(stderr, "File path is too long\n");
        fclose(file);
        return EXIT_FAILURE;
    }

    strcpy(path_copy, file_path);

    const char *file_name = basename(path_copy);
    size_t file_name_length = strlen(file_name);

    if (file_name_length == 0 || file_name_length > UINT32_MAX) {
        fprintf(stderr, "Invalid file name\n");
        fclose(file);
        return EXIT_FAILURE;
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        perror("socket");
        fclose(file);
        return EXIT_FAILURE;
    }

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(
            AF_INET,
            SERVER_IP,
            &server_address.sin_addr) != 1) {
        fprintf(stderr, "Invalid server IP address\n");
        fclose(file);
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (connect(
            socket_fd,
            (struct sockaddr *)&server_address,
            sizeof(server_address)) == -1) {
        perror("connect");
        fclose(file);
        close(socket_fd);
        return EXIT_FAILURE;
    }

    printf("Connected to receiver\n");

    /*
     * First send the file-name length as a 32-bit unsigned integer.
     * htonl converts it to network byte order.
     */
    uint32_t network_name_length = htonl((uint32_t)file_name_length);

    if (send_all(
            socket_fd,
            &network_name_length,
            sizeof(network_name_length)) == -1) {
        fclose(file);
        close(socket_fd);
        return EXIT_FAILURE;
    }

    /* Now send the file name itself. */
    if (send_all(socket_fd, file_name, file_name_length) == -1) {
        fclose(file);
        close(socket_fd);
        return EXIT_FAILURE;
    }

    /* Finally send the file contents in chunks. */
    while ((bytes_read = fread(
                buffer,
                1,
                sizeof(buffer),
                file)) > 0) {

        if (send_all(socket_fd, buffer, bytes_read) == -1) {
            fclose(file);
            close(socket_fd);
            return EXIT_FAILURE;
        }
    }

    if (ferror(file)) {
        perror("fread");
        fclose(file);
        close(socket_fd);
        return EXIT_FAILURE;
    }

    printf("File '%s' sent successfully\n", file_name);

    fclose(file);
    close(socket_fd);

    return EXIT_SUCCESS;
}