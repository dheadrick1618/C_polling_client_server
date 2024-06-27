CC=gcc
CFLAGS=-Werror

message_dispatcher: message_dispatcher.c connection.c
	$(CC) message_dispatcher.c connection.c -o message_dispatcher $(CFLAGS)
	
dispatcher_dummy_client: dispatcher_dummy_client.c connection.c 
	$(CC) dispatcher_dummy_client.c connection.c -o dispatcher_dummy_client $(CFLAGS)

clean:
	rm -f message_dispatcher dispatcher_dummy_client