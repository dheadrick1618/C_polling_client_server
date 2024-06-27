#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include "connection.h"

#define LISTEN_BACKLOG_SIZE 3
#define NAME_SIZE_MAX 32
#define POLLING_TIMEOUT_MS 1000

/*
TODO
    - Implement this with multiple subsystem structs (multiple clients)
    - Define array of subsystem structs (what the message dispatcher is talking with)
*/

int main(int argc, char *argv[])
{
    char buffer[100];
    int ret; // used for assessing returns of various fxn calls

    int num_components = 2;

    component_struct *dfgm_handler = component_factory("dfgm_handler");
    component_struct *coms_handler = component_factory("coms_handler");

    component_struct *components[2] = {dfgm_handler, coms_handler};

    /* NOW implementing polling on conn socket for accepting connections */
    nfds_t nfds;         // num of fds we are polling
    struct pollfd *pfds; // fd we are polling
    int ready;           // how many fd are ready from the poll (have return event)

    nfds = num_components;
    pfds = (struct pollfd*) calloc(nfds, sizeof(struct pollfd));

    for (int i = 0; i < num_components; i++)
    {
        pfds[i].fd = components[i]->conn_socket_fd;
        printf("pfds %d : %d\n",i, pfds[i].fd);
        pfds[i].events = POLLIN;
    }

    for (;;)
    {
        ready = poll(pfds, nfds, POLLING_TIMEOUT_MS);
        if (ready == -1)
        {
            handle_error("polling failed\n");
        }
        // Loop over fds we are polling, check return event setting
        for (int i = 0; i < nfds; i++)
        {
            if (pfds[i].revents != 0)
            {
                // Check if POLLIN return even set for this particular fd
                if (pfds[i].revents & POLLIN)
                {
                    // IF we are waiting to make connection:
                    if (components[i]->connected == 0)
                    {
                        // Receive input request from dfgm conn client
                        //  Accept this conn request and get the data socket fd (returned from accept())
                        printf("WE GOT A CONNECTION \n");
                        socklen_t addrlen = sizeof(components[i]->conn_socket);
                        ret = accept(components[i]->conn_socket_fd, (struct sockaddr *)&components[i]->data_socket, &addrlen);
                        if (ret == -1)
                        {
                            perror("accept");
                            exit(EXIT_FAILURE);
                        }
                        components[i]->data_socket_fd = ret;
                        printf("%s data socket val: %d\n", components[i]->name, components[i]->data_socket_fd);
                        pfds[i].fd = components[i]->data_socket_fd; //set the pfd for the associated component to use the fd associated with the data socket
                        components[i]->connected = 1;
                    }
                    else
                    {
                        // Now that we are connected read data
                        ret = read(pfds[i].fd, buffer, sizeof(buffer));
                        if (ret == -1)
                        {
                            perror("read");
                            exit(EXIT_FAILURE);
                        }
                        // If a zero is read, then the connection is dropped. Set the polling fd back to the conn_socket_fd (go back to listening for connection)
                        if (ret == 0)
                        {
                            pfds[i].fd = components[i]->conn_socket_fd; // Go back to polling the conn socket fd to listen for client connections
                            components[i]->connected = 0;               // Reset the data socket fd (so we know we are looking for conn revents on the poll)
                            printf("Connection to socket: %s closed . {zero byte read indicates this}\n", components[i]->name);
                        }
                        printf("read %zd bytes: %.*s", ret, (int)ret, buffer);
                    }
                }
            }
        }
    }
    exit(EXIT_SUCCESS);
}