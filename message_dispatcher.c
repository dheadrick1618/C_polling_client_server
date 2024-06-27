/*
Written by Devin Headrick
Summer 2024
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

#define LISTEN_BACKLOG_SIZE 3
#define NAME_SIZE_MAX 32
#define POLLING_TIMEOUT_MS 1000

int main(int argc, char *argv[])
{
    char buffer[MESSAGE_UNIT_SIZE];
    int ret; // used for assessing returns of various fxn calls

    int num_components = 2;
    ComponentStruct *dfgm_handler = component_factory("dfgm_handler", DFGM);
    ComponentStruct *coms_handler = component_factory("coms_handler", GS);
    //ComponentStruct *subsystem_monitor = component_factor("subsystem_monitor", SUBSYSTEM_MONITOR);

    // Array of pointers to components the message dispatcher interacts with
    ComponentStruct *components[2] = {dfgm_handler, coms_handler};

    nfds_t nfds;         // num of fds we are polling
    struct pollfd *pfds; // fd we are polling
    int ready;           // how many fd are ready from the poll (have return event)

    nfds = num_components;
    pfds = (struct pollfd *)calloc(nfds, sizeof(struct pollfd));

    for (int i = 0; i < num_components; i++)
    {
        pfds[i].fd = components[i]->conn_socket_fd;
        printf("pfds %d : %d\n", i, pfds[i].fd);
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
                if (pfds[i].revents & POLLIN)
                {
                    // IF we are waiting for a client to send a connection request 
                    if (components[i]->connected == 0)
                    {
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
                        pfds[i].fd = components[i]->data_socket_fd; // set the pfd for the associated component to use the fd associated with the data socket
                        components[i]->connected = 1;
                    }
                    // IF we are waiting for incoming data from a connected client
                    else
                    {
                        ret = read(pfds[i].fd, buffer, sizeof(buffer));
                        if (ret == -1)
                        {
                            perror("read");
                            exit(EXIT_FAILURE);
                        }
                        if (ret == 0)
                        {
                            pfds[i].fd = components[i]->conn_socket_fd; // Go back to polling the conn socket fd to listen for client connections
                            components[i]->connected = 0;               // Reset the conn flag (so we know we are back to looking for conn revents on the poll)
                            printf("Connection to socket: %s closed . {zero byte read indicates this}\n", components[i]->name);
                        }
                        printf("read %zd bytes: %.*s", ret, (int)ret, buffer);

                        int dest_id = get_msg_dest_id(buffer);
                        printf("Msg Dest ID: %d\n", dest_id);

                        // Now use the component ID to determine what component (socket) to send the message to
                        // loop over components array of pointers - whichever component id enum matches the read dest id is what we are writing to
                        int dest_comp_fd = get_fd_from_id(components, num_components, dest_id);
                        printf("Destination component fd: %d \n", dest_comp_fd);
                        if (dest_comp_fd > -1)
                        {
                            ret = write(dest_comp_fd, buffer, sizeof(buffer));
                            if (ret < 0){
                                printf("Write failed \n");
                            }
                        }

                        memset(buffer, 0, MESSAGE_UNIT_SIZE); // clear read buffer after handling data
                    }
                }
            }
        }
    }

    // printf("Freeing components in pointer array \n");
    // for(int i = 0; i < num_components; i++){
    //     free(components[i]);
    // }

    exit(EXIT_SUCCESS);
}