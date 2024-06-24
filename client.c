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

    /*
     * For portability clear the whole structure, since some
     * implementations have additional (nonstandard) fields in
     * the structure.
     */

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

    /* Send arguments. */

    char input[100];

    while (1)
    {
        fgets(input, 100, stdin);

        ret = write(data_socket, input, strlen(input) + 1);
        if (ret == -1)
        {
            perror("write");
            break;
        }

        ret = read(data_socket, buffer, sizeof(buffer));
        if (ret == -1)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if(ret > 0){
            printf("Received: %s", buffer);
        }
    }

    /* Request result. */
    // while (1)
    // {
    //     strcpy(buffer, "END");
    //     ret = write(data_socket, buffer, strlen(buffer) + 1);
    //     if (ret == -1)
    //     {
    //         perror("write");
    //         exit(EXIT_FAILURE);
    //     }
    //     sleep(1);
    // }

    /* Receive result. */

    ret = read(data_socket, buffer, sizeof(buffer));
    if (ret == -1)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }

    /* Ensure buffer is 0-terminated. */

    buffer[sizeof(buffer) - 1] = 0;

    printf("Result = %s\n", buffer);

    /* Close socket. */

    close(data_socket);

    exit(EXIT_SUCCESS);
}