#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>


static int p_threshold = 0;
static int c_threshold;
static int count = 1;
#define PACKET_SIZE 1024

typedef struct
{
    char numPackets[10];
    char packetId[10];
    char ack[10];
    char rws[10];

    char payload[PACKET_SIZE + 1];

} MsgRec;

void print_rec(MsgRec rec)
{
	printf("---------\n");
	printf("rec->numPackets = [%s]\n",rec.numPackets);
	printf("rec->packetId =   [%s]\n",rec.packetId);
	printf("rec->ack =        [%s]\n",rec.ack);
	printf("rec->rws =        [%s]\n",rec.rws);
//	printf("rec->payload =    [%s]\n",rec.payload);
	printf("---------\n");
}

void init_net_lib(double f1, unsigned int seed)
{
	if ((f1 < 0) || (f1 > 1)) {
		printf("%f : Error in setting packet lost probability!\n",f1);
		exit(0);
	}
	else {
		srand(seed);
		p_threshold = (int)(f1 * 1000);
	}

}

int sendto_(int i1, void* c1, int i2, int i3, struct sockaddr* sa, int i4)
{
	int rnd;
	double rnd_max = (double)RAND_MAX;

	rnd = ((rand() / rnd_max) * 1000);
	if (1 > p_threshold) {
		printf("sending...\n");
		return sendto(i1, c1, i2, i3, sa, i4);
	}
	return i2;
}
