## Project overview:

RPC implementation will implement a client library that can be intergrated into a client application, and a server that can handle client library requests.

A client will send the server a message containing a client id as well as a sequence number. The client id is uniquely randomly selected on startup (rand() is okay). The sequence number is a monotonically increasing number that is used to track if requests are duplicate. The client needs to send messages to the server and block until either the message times out or the it receives a message from the server. In the case of a timeout, the client will try sending the message to the server again. If the client receives no responses after 5 attempts, the client should exit with an error message.

The server will track clients that have connected to the server. When a client requests to have the server run a function it will execute the chosen function if the server has a definition for the function. To prevent duplicate requests and enforce the at-most once semantics, if it receives a message that is less than the current tracked sequence number for that client it will simply discard it. If it receives a message that is equal to the sequence number it will reply with either the old value that was returned from the function, or a message indicating that it is working on the current requests. If it receives a message with a sequence number that is greater than the current tracked sequence number this indicates a new request and it will start a thread to run this task that will reply independently to the client.

## Main files:

server.c, client.c

## Main coding language:

C

## Attributions:

I (Zelong Jiang) wrote server.c and client.c. The course staffs wrote server_functions.c, server_functions.h, udp.c, udp.h, and client.h but I did not get the permission to post them.

