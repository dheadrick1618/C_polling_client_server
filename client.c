/*
 * File client.c
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include "connection.h"

int main(int argc, char *argv[])
{
    struct sockaddr_un addr;
    int ret;
    int data_socket;
    char buffer[BUFFER_SIZE];

    /* Create local socket. */

    data_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (data_socket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));

    /* Connect socket to socket address. */
    // Arg 1 is the client 'num' which determines what named pipe to associated with the socket
    char fifo_name[32];
    // Create a name of the fifo based on this iter val
    char fifo_name_pre[] = "fifo_socket_num_";
    sprintf(fifo_name, "%s%d", fifo_name_pre, atoi(argv[1]));

    printf("Connecting to %s\n", fifo_name);

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, fifo_name, sizeof(addr.sun_path) - 1);

    ret = connect(data_socket, (const struct sockaddr *)&addr,
                  sizeof(addr));
    if (ret == -1)
    {
        fprintf(stderr, "The server is down.\n");
        exit(EXIT_FAILURE);
    }

    // Setup polling for non blocking read
    struct sockaddr_un *s_name = calloc(0, sizeof(struct sockaddr_un));
    nfds_t nfds;
    struct pollfd *pfds;
    nfds = 1;

    pfds = calloc(nfds, sizeof(struct pollfd));

    pfds[0].fd = data_socket;
    pfds[0].events = POLLIN;

    /* Send arguments. */
    char user_input[100];

    int ready;

    while (1)
    {
        fgets(user_input, 100, stdin);

        ret = write(data_socket, user_input, strlen(user_input) + 1);
        if (ret == -1)
        {
            perror("write");
            printf("Error writing \n");
            break;
        }

        /* Poll the socket */
        ready = poll(pfds, nfds, 10); // 10 milli sec timeout
        if (ready == -1)
        {
            perror("poll");
            exit;
        }
        if (pfds[0].revents != 0)
        {
            if (pfds[0].revents & POLLIN)
            {
                ret = read(data_socket, buffer, sizeof(buffer));
                if (ret == -1)
                {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    printf("Received: %s", buffer);
                }
            }
        }
    }

    close(data_socket);

    exit(EXIT_SUCCESS);
}