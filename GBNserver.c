/*
 *  server.c - network systems project 1
 *  By Terry Gruenewald
 *  Based on the network systems udp_client.c and udp_server.c examples.
 *  http://beej.us/ for examples for getting the file size and sendto,recvfrom, socket and bind syntax
 *  man pages for directory listing help
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <dirent.h>
#include "sendto_.h"



#define MAXBUFSIZE 10000

int main (int argc, char * argv[] )
{


	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}
    printf("Server starting up!\n");

	/******************
     This code populates the sockaddr_in struct with
     the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("client: socket");
        exit(1);
    }

	/******************
     Once we've created a socket, we must bind that socket to the
     local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
        exit(1);
	}

	remote_length = sizeof(remote);
    struct sockaddr_in from_addr;
    int addr_length = sizeof(struct sockaddr);
    bzero(buffer,sizeof(buffer));

    char response[MAXBUFSIZE];
    int done = 0;
    MsgRec *rec;
    int i = 0;
    int rws = 6;
    int laf = rws;
    int lfr = 0;
    int totalExpectedPackets;
    MsgRec *receivedMsg;
    int prevPacketId = 0;
    int packetId = 0;
    while (!done)
    {
        // wait for command from client
        int numbytes;
        bzero(buffer,sizeof(buffer));
        if ((numbytes = recvfrom(sock, buffer, sizeof(buffer)-1, 0,
                                 (struct sockaddr *)&from_addr, &addr_length)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        printf("Got: %s\n", buffer);
        rec = (MsgRec *) buffer;
        if (i == 0)
        {
        	// then read the num packets
        	printf("Total expected number of packets [%s]\n", rec->numPackets);
        	totalExpectedPackets = atoi(rec->numPackets);
        	receivedMsg = malloc(totalExpectedPackets*sizeof(MsgRec));
        }
        memcpy(&receivedMsg[i],rec,sizeof(MsgRec));
        packetId = atoi(rec->packetId);
        if ((packetId - prevPacketId) > 1)
        {
        	// then packets arrive out of order
        	printf("Packets arrive out of order, just send the last ack back");
        	int len = sizeof(rec[lfr]);
			char *buf = malloc(len);
			memcpy(buf, &receivedMsg[lfr], len);
            if ((numbytes = sendto_(sock,buf, len,0,(struct sockaddr *)&from_addr, addr_length)) == -1) {
                perror("talker: sendto");
                exit(1);
            }
            free(buf);
        }
        else
        {
        	// packets are in order, proceed
            prevPacketId = packetId;
            laf++;
            lfr = i;
            i++;
            printf("sending ack\n");
            sprintf(rec->rws,"%d", rws - lfr);
            sprintf(rec->ack,"%d",lfr);
            int len = sizeof(rec[lfr]);
			char *buf = malloc(len);
			printf("the ack i think i'm sending %s\n", rec->ack);
			memcpy(buf, rec, len);
	        if ((numbytes = sendto_(sock,buf, len,0,(struct sockaddr *)&from_addr, addr_length)) == -1) {
	            perror("talker: sendto");
	            exit(1);
	        }
	        free(buf);
        }




    }

	close(sock);
}


