#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 5000

int main(void)
{
    int socket_fd;

    struct sockaddr_in server_address;

    const char *message = "Hello from sender!";

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        perror("socket");
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
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (connect(
            socket_fd,
            (struct sockaddr *)&server_address,
            sizeof(server_address)) == -1) {
        perror("connect");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    printf("Connected to receiver\n");

    ssize_t bytes_sent = send(
        socket_fd,
        message,
        strlen(message),
        0
    );

    if (bytes_sent == -1) {
        perror("send");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    printf("Message sent successfully\n");

    close(socket_fd);

    return EXIT_SUCCESS;
}