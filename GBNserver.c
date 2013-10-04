/*
 *  GBNServer.c - network systems project 2
 *  By Terry Gruenewald and Nimisha Ranganath
 *  Based on the slides from Sept 9th
 *  http://beej.us/ for examples for getting the file size and sendto,recvfrom, socket and bind syntax
 *
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
//FILE *fp;
#define LOGFILE stdout
int main (int argc, char * argv[] )
{

	FILE *outputFile;
	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	if(argc<5)
	{
		printf("usage : %s <server_port> <error_rate> <random_seed> <output_file> <receive_log> \n", argv[0]);
		exit(1);
	}


/*
    if ((fp = fopen(argv[5],"w")) == NULL)
    {
    	perror("Unable to open log file %s\n", argv[5]);
    	exit(1);
    }
    */
    if ((outputFile = fopen(argv[4],"w")) == NULL)
    {
    	printf("Unable to open output file %s\n", argv[4]);
    	exit(1);
    }
    fprintf(LOGFILE,"Server starting up!\n");

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
		perror("unable to bind socket\n");
        exit(1);
	}

	remote_length = sizeof(remote);
    struct sockaddr_in from_addr;
    int addr_length = sizeof(struct sockaddr);
    bzero(buffer,sizeof(buffer));

    MsgRec *rec;
    int i = 0;
    int rws = 6;
    int laf = rws;
    int lfr = 0;
    int totalExpectedPackets = -1;
    MsgRec *receivedMsg;
    int prevPacketId = 0;
    int packetId = 0;
    int expectedPacket = 0;
    int totalReceivedPackets = 0;
    int increasing_mode = 0;
	struct tm *curTime;
	time_t storeTime;


    while (totalReceivedPackets != totalExpectedPackets)
    {
    	fprintf(LOGFILE,"Packet structure:\n");
    	int c;
        for (c = lfr-1; c < laf + 3;c++ )
        {
        	if (c == lfr)
        		fprintf(LOGFILE,"[lfr -> %-2d] ", c);
        	else if (c == laf)
        		fprintf(LOGFILE,"[laf -> %-2d] ", c);
        	else
        		fprintf(LOGFILE,"[%-2d] ", c);
        }
        fprintf(LOGFILE,"\n rws = %d\n", laf - lfr);

        // wait for command from client
        int numbytes;
        bzero(buffer,sizeof(buffer));
        if ((numbytes = recvfrom(sock, buffer, sizeof(buffer)-1, 0,
                                 (struct sockaddr *)&from_addr, &addr_length)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
    	if (increasing_mode && (laf - lfr) < rws)
    	{
    		laf++;  // keep increasing until it is back up to RWS
    		fprintf(LOGFILE,"Increasing mode activated.   LAF is now at %d\n", laf);
    	}
        rec = (MsgRec *) buffer;
        if (i == 0)
        {
        	// then read the num packets
        	fprintf(LOGFILE,"Total expected number of packets [%s]\n", rec->numPackets);
        	totalExpectedPackets = atoi(rec->numPackets);
        	receivedMsg = malloc(totalExpectedPackets*sizeof(MsgRec));
        }
        fprintf(LOGFILE,"Receiving:\n");
        print_rec(*rec);
		storeTime = time(NULL);
		curTime = localtime(&storeTime);
		fprintf(LOGFILE,"Receive seq=%s free_slots=%d LFRead=%d LFRcvd=%d LAF=%d time=%s\n", rec->packetId, laf - lfr, lfr-1,lfr,laf,asctime(curTime));



        packetId = atoi(rec->packetId);
        i++;

        if (i > 8 && !increasing_mode)
        {
        	if (laf > 0 && (laf - lfr) >= 0)
        	{
        		fprintf(LOGFILE,"reducing frame size %d\n", --laf);
        	}
        }
        if (packetId != expectedPacket)
        {
        	// then packets arrive out of order
        	fprintf(LOGFILE,"Packets arrive out of order, just send the last ack back.  Duplicate ack sent");
        	sprintf(rec->rws,"%d", laf - lfr);
            sprintf(rec->ack,"%d",lfr-1);
            fprintf(LOGFILE,"Sending:\n");
            print_rec(*rec);
        	int len = sizeof(MsgRec);
			char *buf = malloc(len);
			memcpy(buf, rec, len);
			// <Send | Resend | Receive> <Seq #> [Free Slots] <LFRead> <LFRcvd> <LAF> <Time>
			storeTime = time(NULL);
			curTime = localtime(&storeTime);
			fprintf(LOGFILE,"Resend seq=%s free_slots=%d LFRead=%d LFRcvd=%d LAF=%d time=%s\n", rec->packetId, laf - lfr, lfr-1,lfr,laf,asctime(curTime));
            if ((numbytes = sendto_(sock,buf, len,0,(struct sockaddr *)&from_addr, addr_length)) == -1) {
                perror("talker: sendto");
                exit(1);
            }
            free(buf);
        }
        else if (packetId == expectedPacket && packetId <= laf)
        {
        	fprintf(LOGFILE,"packets are in order, proceed\n");
        	// packets are in order, proceed
        	expectedPacket++;
        	totalReceivedPackets++;
            prevPacketId = packetId;

            memcpy(&receivedMsg[lfr],rec,sizeof(MsgRec));

            sprintf(rec->rws,"%d", laf - lfr);
            sprintf(rec->ack,"%d",lfr);
            lfr++;
            laf++;
            int len = sizeof(MsgRec);
			char *buf = malloc(len);
			memcpy(buf, rec, len);
            fprintf(LOGFILE,"Sending:\n");
            print_rec(*rec);
			storeTime = time(NULL);
			curTime = localtime(&storeTime);
			fprintf(LOGFILE,"send seq=%s free_slots=%d LFRead=%d LFRcvd=%d LAF=%d time=%s\n", rec->packetId, laf - lfr, lfr-1,lfr,laf,asctime(curTime));
	        if ((numbytes = sendto_(sock,buf, len,0,(struct sockaddr *)&from_addr, addr_length)) == -1) {
	            perror("talker: sendto");
	            exit(1);
	        }
	        free(buf);
        }
        else if (packetId > laf)
		{
			// then it is outside of the excepted frame, send it back
			fprintf(LOGFILE,"Packets arrive faster than can be processed");
			sprintf(rec->rws,"%d", 0);
			sprintf(rec->ack,"%d",0);

			int len = sizeof(MsgRec);
			char *buf = malloc(len);
			memcpy(buf, rec, len);
			fprintf(LOGFILE,"Sending:\n");
			print_rec(*rec);
			storeTime = time(NULL);
			curTime = localtime(&storeTime);
			fprintf(LOGFILE,"resend seq=%s free_slots=%d LFRead=%d LFRcvd=%d LAF=%d time=%s\n", rec->packetId, laf - lfr, lfr-1,lfr,laf,asctime(curTime));
			if ((numbytes = sendto_(sock,buf, len,0,(struct sockaddr *)&from_addr, addr_length)) == -1) {
				perror("talker: sendto");
				exit(1);
			}
			free(buf);
			fprintf(LOGFILE,"Sleeping 10 millseconds...\n");
			fflush(LOGFILE);
			fd_set select_fds;
			struct timeval timeout;

			FD_ZERO(&select_fds);
			FD_SET(sock, &select_fds);

			timeout.tv_sec = 0;
			timeout.tv_usec = 10000;  // 10ms

			select(sock+1, &select_fds, NULL,NULL, &timeout);
			fprintf(LOGFILE,"awake!!\n");
			increasing_mode = 1;
		}
        // print out the packets stored
        fprintf(LOGFILE,"Packet storage:\n");
        for (c = 0; c < totalExpectedPackets; c++)
        {
        	if (((c+1) % 10) == 0 )
        	{
        		fprintf(LOGFILE,"[%-2s]\n",receivedMsg[c].packetId);
        	}
        	else
        	{
        		fprintf(LOGFILE,"[%-2s]-",receivedMsg[c].packetId);
        	}
        }
        fprintf(LOGFILE,"\n");


    } // end while


    // then write the packets out to the out file
    fprintf(LOGFILE,"Writing out output file...\n");
    int c;
    for (c = 0; c < totalExpectedPackets; c++)
    {
    	fwrite(receivedMsg[c].payload, sizeof(char), PACKET_SIZE,outputFile);
    }
    fclose(outputFile);

    free(receivedMsg);
	close(sock);
}



