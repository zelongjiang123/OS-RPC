///////////////////////////////////////////////////////////////////////////////
// Main File:        server.c
// This File:        server.c
// Semester:         CS 537 Spring 2023
// Instructor:       Shivaram
//
// Author:           Zelong Jiang
// wisc ID:          9082157588
// Email:            zjiang287@wisc.edu
// CS Login:         zjiang dipaksi
//
/////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include "server_functions.h"
#include "udp.h"
#define CALL_TABLE_SIZE 100
struct client_call
{
    int client_id;
    int last_seq_num;
    int last_result;
};
void handle_rpc_call(struct socket s, struct packet_info packet, struct client_call call_table[], int *num_clients)
{
    int client_id;
    int seq_num;
    int func_id;
    int arg1, arg2;
    //int payload_length = sizeof(client_id) + sizeof(seq_num) + sizeof(func_id);

    // Extract information from the payload
    // parse the response from the server
    char message  [packet.recv_len + 1];
    memcpy(message, packet.buf, packet.recv_len);
    message[packet.recv_len] = '\0';
    //printf("message %s\n", message);

    char ack_type[BUFLEN], other[BUFLEN];
    sscanf(message, "%s %s", ack_type, other);

    if (strcmp(ack_type, "IDLE") == 0)
    {
        //printf("IDLE\n");
        func_id = 1;
        sscanf(message, "%s %d %d %d", ack_type, &seq_num, &client_id, &arg1);
    }
    else if (strcmp(ack_type, "GET") == 0)
    {
        //printf("GET\n");
        func_id = 2;
        sscanf(message, "%s %d %d %d", ack_type, &seq_num, &client_id, &arg1);
    }
    else if (strcmp(ack_type, "PUT") == 0)
    {
        //printf("PUT\n");
        func_id = 3;
        sscanf(message, "%s %d %d %d %d", ack_type, &seq_num, &client_id, &arg1, &arg2);
    }
    else
    {
        printf("wrong call message\n");
        return;
    }

    //printf("arg1 %d arg2 %d\n", arg1, arg2);
    // Find the client's call entry in the call table
    struct client_call *client_call = NULL;
    for (int i = 0; i < *num_clients; i++)
    {
        if (call_table[i].client_id == client_id)
        {
            client_call = &call_table[i];
            break;
        }
    }

    // Handle the sequence number based on the client's previous sequence number
    if (client_call == NULL)
    {
	//printf("new client with seq number %d\n", seq_num);
        // New client, add a new call entry
        if (*num_clients >= CALL_TABLE_SIZE)
        {
            // Call table is full, reject the request
            printf("Call table is full, rejecting request from client %u\n", client_id);
            return;
        }
        client_call = &call_table[*num_clients];
        client_call->client_id = client_id;
        client_call->last_seq_num = -1;
        client_call->last_result = 0;
        (*num_clients)++;
    }
    if (seq_num > client_call->last_seq_num) // new request - execute RPC and update call table entry
    {
        // New request
        client_call->last_seq_num = seq_num;
        switch (func_id)
        {
        case 1:
            idle(arg1);
            // client_call->last_result = idle(arg1);
            break;
        case 2:
            client_call->last_result = get(arg1);
            break;
        case 3:
            client_call->last_result = put(arg1, arg2);
            break;
        default:
            printf("Invalid function ID %d\n", func_id);
            return;
        }
      //  printf("Executed function %d for client %u with seq num %u, result %d\n", func_id, client_id, seq_num, client_call->last_result);
        char payload[100];
        sprintf(payload, "%s %d %d %d", "ACK", seq_num, client_id, client_call->last_result);
	send_packet(s, packet.sock, packet.slen, payload, sizeof(payload));
    }
    else if (seq_num == client_call->last_seq_num)
    // duplicate of last RPC or duplicate of in progress RPC.Either resend result or send acknowledgement that RPC is being worked on.
    {
	char payload[100];
        sprintf(payload, "%s %d %d %d", "ACK", seq_num, client_id, client_call->last_result);
	send_packet(s, packet.sock, packet.slen, payload, sizeof(payload));
    }
    else // old RPC, discard message and do not reply
    {
      // printf("no response\n");
       // char payload[100];
        //sprintf(payload, "%s", "No");
       // send_packet(s, packet.sock, packet.slen, payload, sizeof(payload));
    }
}
int main(int argc, char **argv)
{

    if (argc != 2)
    {
        printf("expected usage: ./server port\n");
        exit(0);
    }
    int port = atoi(argv[1]);
    //int seq_num = 0; // sequence number
    const int max_client = 100;
    struct client_call call_table[max_client];
    int num_client = 0;
    struct socket sock = init_socket(port); // initialize the socket with the given port number
    printf("socket: %d\n", port);
    while (1)
    {
        struct packet_info packet = receive_packet(sock);
        //printf("packet: %s, length %d\n", packet.buf, packet.recv_len);
	handle_rpc_call(sock, packet, call_table, &num_client);
    }
}
