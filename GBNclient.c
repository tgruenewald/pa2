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

#define PACKET_SIZE 1024
//adding this comment line
typedef struct
{
    int numPackets;
    char payload[PACKET_SIZE + 1];

} MsgRec;
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
    int i = rec[0].numPackets;
    printf("Number of packets = %d\n", i);
    int x;
   	int nbytes;
    for (x = 0;x < i; x++)
    {
    	printf("rec[%d]->payload= [%s]\n", x, rec[x].payload);

    	nbytes = sendto_(sd, rec[x].payload, strlen( rec[x].payload),0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));

    	printf("Sent\n");
    }

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

    	memset(rec[i].payload,'\0', PACKET_SIZE + 1);
        strncpy(rec[i].payload, bufRead, PACKET_SIZE);
   //     printf("rec[%d]->payload= [%s]\n", i, rec[i].payload);
        int len = strlen(bufRead);
        if (len >= PACKET_SIZE)
        	bufRead = bufRead + PACKET_SIZE;
        i++;
    }
    rec[0].numPackets = i;
    free(buffer);
    return rec;
}




