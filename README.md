# Message Dispatcher in C

This is a rudimentary implementation of the message dispatcher architectural component with the Ex-Alta3 Flight Software (FSW), in C.

## Architectural Description

 Right now the messge dispatcher is the first arch component to start following the OS boot. It can be thought of as the 'post office' of the FSW, and is responsible for routing messages within the On Board Computer (OBC), based on a 'destination ID' field within each message.

 The message dispatcher acts as a central server within the FSW, facilitating communication between other architectural components.

## Implementation / Design 

The message dispatcher acts a central server, setting up and listening to unix domain sockets to establish connections with other architectural components in the FSW. Polling is used so that various sockets may communicate in a non-blocking fashion in the main process thread.

It polls on a 'conn socket' file descriptor awaiting a client connection request, and upon accepting the request, then polls the 'data socket' file descriptor to read incoming data.

## Notes

The server code functions in the following order to establish connection with a client.

  1. Create connection (unix) sockets
  2. Bind the conn socket fd [from socket() call] to an address in unix domain (fifo file)
  3. Call listen on the bound socket for incomming conn requests from clients
  4. Call accept to accept conn request from client - returns new descriptor for data between client and server
  5. Handle connection now with data socket
