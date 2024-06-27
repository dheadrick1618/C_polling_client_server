
/*
 * File connection.h
 */

#ifndef CONNECTION_H
#define CONNECTION_H


#define LISTEN_BACKLOG_SIZE 3
#define SOCKET_PATH_PREPEND "/tmp/fifo_socket_"
#define BUFFER_SIZE 32

/// @brief Each architecture component the message dispatcher will talk to has its own struct
typedef struct component_struct
{
    char name[32];      // Name of subsystem / payload
    int poll_id;        // Used by the pollfds array to identify what struct its polling
    char fifo_path[32]; // Path to fifo used by Unix Domain Socket
    int connected;      // connection state [for now either con / discon] (connected / waiting for conn / ignore)
    int conn_socket_fd; // connection socket fd for polling for connection
    int data_socket_fd; // data socket fd for polling when there is a connection
    struct sockaddr_un conn_socket;
    struct sockaddr_un data_socket;
} component_struct;

void handle_error(char *error_msg);
int create_socket();

component_struct* component_factory(char* name);

#endif
