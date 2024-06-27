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

void handle_error(char *error_msg)
{
    fprintf(stderr, "Error: %s", error_msg);
    exit(EXIT_FAILURE);
}

int create_socket()
{
    int data_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (data_socket == -1)
    {
        handle_error("socket_creation");
    }
    return data_socket;
}

/// @brief Use byte offset of a read msg to get destination ID. 
/// @param data_buf 
/// @return 
int get_msg_dest_id(char *data_buf)
{
    // Use byte offset
    int dest_id = data_buf[2];

    //TEMP for now we are sending ascii chars, so we want to offset the numerical value
    dest_id -= '0';
    return dest_id;
}

ComponentStruct *component_factory(char *name, int component_id)
{
    int ret;
    ComponentStruct *c = malloc(sizeof(ComponentStruct));
    strcpy(c->name, name);
    c->component_id = component_id;
    c->connected = 0;
    c->conn_socket_fd = create_socket();
    printf("Created conn socket, with fd: %d\n", c->conn_socket_fd);

    // 2. Bind the conn socket fd [from socket() call] to an address in unix domain (fifo file)
    sprintf(c->fifo_path, "%s%s", SOCKET_PATH_PREPEND, c->name);
    strcpy(c->conn_socket.sun_path, c->fifo_path); // strcpy path into conn socket addr
    printf("Socket file path: %s\n", c->conn_socket.sun_path);
    unlink(c->conn_socket.sun_path); // remove socket if it already exists
    c->conn_socket.sun_family = AF_UNIX;
    ret = bind(c->conn_socket_fd, (const struct sockaddr *)&c->conn_socket, sizeof(c->conn_socket));
    if (ret < 0)
    {
        handle_error("binding conn socket\n");
    }
    printf("Bind conn socket for %s \n", name);

    // 3. Call listen on the bound socket for incomming conn requests from clients
    ret = listen(c->conn_socket_fd, LISTEN_BACKLOG_SIZE);
    if (ret == -1)
    {
        handle_error("conn socket listen\n");
    }
    printf("Listening conn socket for %s \n", name);
    // 4. Call accept to accept conn request from client - returns new descriptor for data between client and server
    // 5. Handle connection now with data socket

    return c;
}



int get_fd_from_id(ComponentStruct* cs[], int num_components, int id){
    //loop over components, get fd of one with matching id 
    for(int i = 0; i < num_components; i++){
        if (cs[i]->component_id == id){
            printf("Component match : %s, with id: %d \n", cs[i]->name, cs[i]->component_id);

            //if connected flag is low (component not connected) then return -1
            if (cs[i]->connected == 0){
                printf("Component not connected. Not writing \n");
                return -2;
            }
            return cs[i]->data_socket_fd;
        }
    }
    printf("No matching component found with id: %d \n", id);
    return -1;
}