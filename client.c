
////////////////////////////////////////////////////////////////////////////////
// Main File:        client.c
// This File:        client.c
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
#include "client.h"
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX_SEQ_NUM 10000

// initializes the RPC connection to the server
struct rpc_connection RPC_init(int src_port, int dst_port, char dst_addr[])
{
    //printf("RPC init\n");
    // Create a socket to receive responses from the server
    struct socket recv_socket = init_socket(src_port);

    // Populate the destination address struct
    struct sockaddr_storage dst;
    socklen_t dst_len;
    populate_sockaddr(AF_INET, dst_port, dst_addr, &dst, &dst_len);

    // Create the RPC connection struct
    struct rpc_connection rpc;
    rpc.recv_socket = recv_socket;
    //memcpy(&rpc.dst_addr, &dst, sizeof(struct sockaddr));
    rpc.dst_addr = *((struct sockaddr *)(&dst));
    rpc.dst_len = dst_len;
    rpc.seq_number = 0;
    // Initialize the random number generator with the current time as the seed
    //srand(time(NULL));
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);
    rpc.client_id = rand() % MAX_SEQ_NUM;
    printf("client id: %d\n", rpc.client_id);
    return rpc;
}

// Sleeps the server thread for a few seconds
void RPC_idle(struct rpc_connection *rpc, int time)
{
    //printf("RPC idle\n");
    char payload[BUFLEN];
    int payload_length = sprintf(payload, "IDLE %d %d %d", rpc->seq_number, rpc->client_id, time);
    //struct packet_info response = send_request(rpc, payload, payload_length);
    //printf("seq_num: %d, id: %d\n", rpc->seq_number, rpc->client_id);
    rpc->seq_number++;
    
    // attempt to send the packet to the server RETRY_COUNT times
    for (int i = 0; i < RETRY_COUNT; i++)
    {
	//printf("client %d sent message\n", rpc->client_id);
        send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, payload, payload_length);

        // wait for a response from the server for TIMEOUT_TIME seconds
        struct packet_info response = receive_packet_timeout(rpc->recv_socket, time + TIMEOUT_TIME);
        if (response.recv_len <= 0)
        {
            // no response was received, retry the request
            continue;
        }

	//printf("response: %s\n", response.buf);
        // parse the response from the server
        int ack_seq_number, ack_client_id, ack_result;
        char ack_type[BUFLEN];
        sscanf(response.buf, "%s %d %d %d", ack_type, &ack_seq_number, &ack_client_id, &ack_result);

	//printf("ask_seq_numnber %d, rpc->seq_number %d, client id %d\n", ack_seq_number, rpc->seq_number, rpc->client_id);
        // check if the response is for our request and from the correct client id
        if (ack_seq_number != rpc->seq_number - 1 || ack_client_id != rpc->client_id)
        {
	   // printf("wrong seq number\n");
            // response is not for our request, retry the request
            continue;
        }

        // check if the response is an ACK
        if (strcmp(ack_type, "ACK") == 0)
        {
	    // printf("Acknowledged client id %d\n", rpc->client_id);
            // ACK received, delay for 1 second
            sleep(1);
            return;
        }
        else
        {
	    // printf("not Acknowledged\n");
            // response is not an ACK, retry the request
            continue;
        }
    }
}

// gets the value of a key on the server store
int RPC_get(struct rpc_connection *rpc, int key)
{
    //printf("RPC get\n");
    char payload[BUFLEN];
    int payload_length = sprintf(payload, "GET %d %d %d", rpc->seq_number, rpc->client_id, key);
    //printf("seq_num: %d, id: %d\n", rpc->seq_number, rpc->client_id);
    // printf("payload: %s\n", payload);
    rpc->seq_number++;
    // struct packet_info response = send_request(rpc, payload, payload_length);

    // attempt to send the packet to the server RETRY_COUNT times
    for (int i = 0; i < RETRY_COUNT; i++)
    {
        send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, payload, payload_length);

        // wait for a response from the server for TIMEOUT_TIME seconds
        struct packet_info response = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        if (response.recv_len <= 0)
        {
            // no response was received, retry the request
            continue;
        }

//	printf("response: %s\n", response.buf);
        // parse the response from the server
        int ack_seq_number, ack_client_id, ack_result;
        char ack_type[BUFLEN];
        sscanf(response.buf, "%s %d %d %d", ack_type, &ack_seq_number, &ack_client_id, &ack_result);

  //      printf("ask_seq_numnber %d, rpc->seq_number %d, client id %d\n", ack_seq_number, rpc->seq_number, rpc->client_id);
	// check if the response is for our request and from the correct client id
        if (ack_seq_number != rpc->seq_number - 1 || ack_client_id != rpc->client_id)
        {
            // response is not for our request, retry the request
	   // printf("wrong seq number\n");
            continue;
        }

        // check if the response is an ACK
        if (strcmp(ack_type, "ACK") == 0)
        {
	    // printf("Acknowledged\n");
            // ACK received, delay for 1 second
            sleep(1);
            return ack_result;
        }
        else
        {
	   // printf("not Acknowledged\n");
            // response is not an ACK, retry the request
            continue;
        }
    }
    // maximum number of retries reached, return failure
    return -1;
}

// sets the value of a key on the server store
int RPC_put(struct rpc_connection *rpc, int key, int value)
{
    //printf("RPC put\n");
    char payload[BUFLEN];
    int payload_length = sprintf(payload, "PUT %d %d %d %d", rpc->seq_number, rpc->client_id, key, value);
    //printf("seq_num: %d, id: %d\n", rpc->seq_number, rpc->client_id);
    rpc->seq_number++;

    // attempt to send the packet to the server RETRY_COUNT times
    for (int i = 0; i < RETRY_COUNT; i++)
    {
        send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, payload, payload_length);

        // wait for a response from the server for TIMEOUT_TIME seconds
        struct packet_info response = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        if (response.recv_len <= 0)
        {
            // no response was received, retry the request
            continue;
        }

	//printf("response: %s\n", response.buf);
        // parse the response from the server
        int ack_seq_number, ack_client_id, ack_result;
        char ack_type[BUFLEN];
        sscanf(response.buf, "%s %d %d %d", ack_type, &ack_seq_number, &ack_client_id, &ack_result);

       // printf("ask_seq_numnber %d, rpc->seq_number %d, client id %d\n", ack_seq_number, rpc->seq_number, rpc->client_id);
	// check if the response is for our request and from the correct client id
        if (ack_seq_number != rpc->seq_number - 1 || ack_client_id != rpc->client_id)
        {
	 //   printf("wrong seq number\n");
            // response is not for our request, retry the request
            continue;
        }

        // check if the response is an ACK
        if (strcmp(ack_type, "ACK") == 0)
        {
	   // printf("Acknowledged\n");
            // ACK received, delay for 1 second
            sleep(1);
            return ack_result;
        }
        else
        {
	   // printf("not Acknowledged\n");
            // response is not an ACK, retry the request
            continue;
        }
    }

    // maximum number of retries reached, return failure
    return -1;
}

// closes the RPC connection to the server
void RPC_close(struct rpc_connection *rpc)
{
  // printf("RPC close\n");
   close_socket(rpc->recv_socket);
}
