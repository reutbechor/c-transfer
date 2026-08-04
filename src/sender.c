#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 5000
#define BUFFER_SIZE 1024

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

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        perror("socket");
        fclose(file);
        return EXIT_FAILURE;
    }

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

    printf("File '%s' sent successfully\n", file_path);

    while ((bytes_read = fread(
                buffer,
                1,
                sizeof(buffer),
                file)) > 0) {

        ssize_t bytes_sent = send(
            socket_fd,
            buffer,
            bytes_read,
            0
        );

        if (bytes_sent == -1) {
            perror("send");
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

    printf("File sent successfully\n");

    fclose(file);
    close(socket_fd);

    return EXIT_SUCCESS;
}