/* GBNclient.c */
/* This is a sample UDP client/sender using "sendto_.h" to simulate dropped packets.  */
/* This code will not work unless modified. */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>   /* memset() */
#include <sys/time.h> /* select() */
#include <signal.h>
#include <unistd.h>
#include "sendto_.h"


//adding this comment line

MsgRec *open_file(char *fileName);
int main(int argc, char *argv[]) {

    /*
	// check command line args.
	if(argc<7)
	{
		printf("usage : %s <server_ip> <server_port> <error rate> <random seed> <send_file> <send_log> \n", argv[0]);
		exit(1);
	}

	// Note: you must initialize the network library first before calling sendto_().  The arguments are the <errorrate> and <random seed>
	init_net_lib(atof(argv[3]), atoi(argv[4]));
	printf("error rate : %f\n",atof(argv[3]));
*/
	// socket creation
	int sd;
    if ((sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
		printf("%s: cannot create socket \n",argv[0]);
		exit(1);
    }


	// get server IP address (input must be IP address, not DNS name)
	struct sockaddr_in remoteServAddr;
	bzero(&remoteServAddr,sizeof(remoteServAddr));               //zero the struct
	remoteServAddr.sin_family = AF_INET;                 //address family
	remoteServAddr.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remoteServAddr.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address
	printf("%s: sending data to '%s:%s' \n", argv[0], argv[1], argv[2]);

	//Call sendto_ in order to simulate dropped packets


    // open the file
    MsgRec *rec = open_file("test.txt");
    int totalPackets = atoi(rec[0].numPackets);
    printf("Number of packets = %d\n", totalPackets);
    int x;
   	int nbytes;
   	int sws = 9;
   	int lfs = 1;
   	int lar = 0;
   	fd_set select_fds;
   	struct timeval timeout;
    for (x = 0;x < totalPackets; x++)
    {

    	while (lfs < sws)
    	{
    		sprintf(rec[x].packetId,"%d", lfs);
			printf("rec[%d]->payload= [%s]\n", x, rec[x].payload);
			int len = sizeof(rec[x]);
			char *buf = malloc(len);
			memcpy(buf, &rec[x], len);
			nbytes = sendto_(sd, buf, len,0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
			free(buf);
			printf("Sent\n");
			lfs++;
    	}


    	// now wait for acknowledgements
    	FD_ZERO(&select_fds);
    	FD_SET(sd, &select_fds);

    	timeout.tv_sec = 30; // 3 second timeout
    	timeout.tv_usec = 0;

		// set timeout to 50ms
    	if (select(sd + 1, &select_fds, NULL,NULL, &timeout) == 0)
    	{
			// during a timeout, this is the time to run through the SWS window
			// and see what has yet to be acknowledged.  If you get to this and 
			// you see that there is a "hole" in the acks, then go ahead resend them
			// all.
			
			// so now we have to handle the situation where the RWS was set to 
			// zero, then we need to stop sending packets, and send a "probe" packet (i.e. 
			// basically a packet that has like a -1 packetId.)   Eventually you will
			// get a RWS > 0, then you know you can start sending again.
    		printf("timeout occurred\n");
    	}
    	else
    	{
			// then this means that we got an ack back, so go ahead and based on 
			// packet number, find it in the array, and mark it ack'd.
    	    struct sockaddr_in from_addr;
    	    int addr_length = sizeof(struct sockaddr);
			int len = sizeof(MsgRec);
			char *buf = malloc(len);
			int numbytes;
            if ((numbytes = recvfrom(sd, buf, len, 0,
            		 (struct sockaddr *)&from_addr, &addr_length)) == -1)
            {
                perror("recvfrom");
                exit(1);
            }
            rec = (MsgRec *) buf;
            printf("rec %s\n", rec->ack);
    	}
    }

	// so we also have to handle the case where the LAST ack doesn't come back.  
/*
Question #7: What should the client do if the last ACK is lost?
On the client side, the last ACK could be lost. In this case, the client will retransmit, but your
server will have already terminated. To terminate the client cleanly, your program should
implement the following solution to the last ACK problem: the client should retransmit the last
data packet at most ten times, and then should close any open files such as the client log file
and then terminate. Clarify your solution in the README that you turn in.	*/
	
    // create an array of structs that will be what will be sent, and keep track of the acks coming back

    // go into a loop that sends each of the parts of the file according to the SWS

    free(rec);
}

MsgRec *open_file(char *fileName)
{
    FILE *fp = fopen(fileName, "r");
    fseek(fp, 0L, SEEK_END);
    int fileSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    unsigned char *buffer;
    buffer = malloc(fileSize);
    if (fread(buffer, sizeof(*buffer), fileSize,fp) != fileSize)
    {
        perror("Mismatched read.  Not all bytes were read");
        free(buffer);
        exit(1);
    }
    fclose(fp);


    int numberOfPackets = fileSize/PACKET_SIZE + 1;

    MsgRec *rec = malloc(numberOfPackets*sizeof(*rec));

    char *bufRead = buffer;
    int i = 0;
    while (i < numberOfPackets)
    {

    	memset(&rec[i], '\0', sizeof(MsgRec));
    	memset(rec[i].payload,'\0', PACKET_SIZE + 1);
        strncpy(rec[i].payload, bufRead, PACKET_SIZE);
   //     printf("rec[%d]->payload= [%s]\n", i, rec[i].payload);
        int len = strlen(bufRead);
        if (len >= PACKET_SIZE)
        	bufRead = bufRead + PACKET_SIZE;

        sprintf(rec[i].packetId,"%d", i);
        i++;
    }
    sprintf(rec[0].numPackets,"%d", i);  // the very first packet contains the number of packets to expect
    free(buffer);
    return rec;
}




