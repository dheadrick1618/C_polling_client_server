/*
 * File server.c
 */

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
    // Number of sockets to listen on defined as first arg

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <num_sockets>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_sockets = atoi(argv[1]);

    if (num_sockets < 1)
    {
        fprintf(stderr, "Number of sockets must be greater than 0\n");
        exit(EXIT_FAILURE);
    }

    // Create an array of sockets
    struct sockaddr_un *s_name = calloc(num_sockets, sizeof(struct sockaddr_un));
    int con_sockets[num_sockets];  // fd for socket listening for connections
    int data_sockets[num_sockets]; // fd for sockets that have a connection
    int down_flag = 0;
    int ret;
    char buffer[BUFFER_SIZE]; // For storing the message read from a socket during a POLLIN revent

    nfds_t nfds;
    ssize_t s;
    struct pollfd *pfds;
    int ready; // How many sockets are ready for polling

    nfds = num_sockets;

    pfds = calloc(nfds, sizeof(struct pollfd));

    for (int i = 0; i < num_sockets; i++)
    {
        /* Create a socket struct for listening for connection */
        con_sockets[i] = socket(AF_UNIX, SOCK_SEQPACKET, 0);
        if (con_sockets[i] == -1)
        {
            perror("con socket");
            exit(EXIT_FAILURE);
        }
        /*
         * For portability clear the whole structure, since some
         * implementations have additional (nonstandard) fields in
         * the structure.
         */
        memset(&s_name[i], 0, sizeof(s_name));

        /* Bind socket to socket name. */
        s_name[i].sun_family = AF_UNIX;
        char fifo_name[32];
        // Create a name of the fifo based on this iter val
        char fifo_name_pre[] = "fifo_socket_num_";
        sprintf(fifo_name, "%s%d", fifo_name_pre, i);
        printf("Fifo name: %s\n", fifo_name);

        // Copy the name of the socket (fifo abs path) into the socket name struct
        strncpy(s_name[i].sun_path, fifo_name, sizeof(s_name[i].sun_path) - 1);
        unlink(s_name[i].sun_path); // Called before bind to remove socket it if already exists

        /* Bind to the socket */
        ret = bind(con_sockets[i], (const struct sockaddr *)&s_name[i],
                   sizeof(s_name[i]));
        if (ret == -1)
        {
            perror("bind");
            exit(EXIT_FAILURE);
        }

        //  * Prepare for accepting connections. The backlog size is set
        //  * to 20. So while one request is being processed other requests
        //  * can be waiting.
        //  */
        ret = listen(con_sockets[i], 20);
        if (ret == -1)
        {
            perror("listen");
            exit(EXIT_FAILURE);
        }
    }

    /* Blocking accept for expected number of client connections */
    for (int i = 0; i < num_sockets; i++)
    {
        data_sockets[i] = accept(con_sockets[i], NULL, NULL);
        if (data_sockets[i] == -1)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        pfds[i].fd = data_sockets[i];
        pfds[i].events = POLLIN;
    }

    int closed_data_socket_num = -1;

    int dest_id = -1;

    for (;;)
    {
        /* Wait for incoming connection. */
        while (closed_data_socket_num == -1)
        {
            printf("About to poll()\n");
            ready = poll(pfds, nfds, -1);
            if (ready == -1)
                perror("poll");

            printf("Ready: %d\n", ready);

            for (nfds_t i = 0; i < nfds; i++)
            {
                if (pfds[i].revents != 0)
                {
                    // printf("  fd=%d; events: %s%s%s\n", pfds[i].fd,
                    //        (pfds[i].revents & POLLIN) ? "POLLIN " : "",
                    //        (pfds[i].revents & POLLHUP) ? "POLLHUP " : "",
                    //        (pfds[i].revents & POLLERR) ? "POLLERR " : "");

                    if (pfds[i].revents & POLLIN)
                    {
                        s = read(pfds[i].fd, buffer, sizeof(buffer));
                        if (s == -1)
                            perror("read");
                            exit;
                        if (s == 0)
                        {
                            closed_data_socket_num = i;
                            printf("Connection to socket: %d closed . {zero byte read indicates this}\n", i);
                            exit;
                        }
                        printf("read %zd bytes: %.*s\n",
                               s, (int)s, buffer);

                        // First byte is the destination socket num
                        dest_id = buffer[0] - '0'; // convert ascii char to int equivalent ('0' is 48)
                        printf("Destination ID: %d \n", dest_id);
                        if (dest_id > -1 && dest_id < num_sockets)
                        {
                            s = write(pfds[dest_id].fd, buffer, strlen(buffer) + 1);
                            if (s > 0)
                            {
                                printf("Wrote: %d bytes to socket num: %d \n", s, dest_id);
                            }
                        }
                    }
                    else
                    { /* POLLERR | POLLHUP */
                        printf("    closing fd %d\n", pfds[i].fd);
                        // get the value of the fd for the closed data socket -
                        closed_data_socket_num = i;
                        if (close(pfds[i].fd) == -1)
                            perror("closing socket");
                        exit;
                    }
                }
            }
        }

        // Attempt re-connect with closed socket with a timeout
        printf("Listening again for reconnect \n");
        data_sockets[closed_data_socket_num] = accept(con_sockets[closed_data_socket_num], NULL, NULL);
        if (data_sockets[closed_data_socket_num] == -1)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        pfds[closed_data_socket_num].fd = data_sockets[closed_data_socket_num];
        pfds[closed_data_socket_num].events = POLLIN;
        closed_data_socket_num = -1;
    }

    // exit(EXIT_SUCCESS);
}
