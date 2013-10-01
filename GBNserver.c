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
    while (!done)
    {
        // wait for command from client
        int numbytes;
        bzero(buffer,sizeof(buffer));
        if ((numbytes = recvfrom(sock, buffer, sizeof(buffer)-1 , 0,
                                 (struct sockaddr *)&from_addr, &addr_length)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        printf("Got: %s\n", buffer);
    }
    /*
        if (strcmp("exit", buffer) == 0)
        {
            strcpy(response,"Exiting");
            done = 1;
            printf("Sending %s\n", response);

            if ((numbytes = sendto(sock,response, strlen(response),0,(struct sockaddr *)&from_addr,  addr_length)) == -1)
            {
                perror("talker: sendto");
                exit(1);
            }
        }
        else if (strcmp("ls", buffer) == 0)
        {
            printf("Listing directory contents\n");
            // send back the list
            struct dirent *de=NULL;
            DIR *d=NULL;

            // open the current directory
            d=opendir(".");
            if(d == NULL)
            {
                perror("Couldn't open directory");
                return(2);
            }
            strcpy(response,"");

            // Loop while not NULL
            while((de = readdir(d)))
            {
                if ( (strlen(response) + strlen(de->d_name)) > MAXBUFSIZE)
                {
                    strcpy(response,"Buffer exceeded.  More files than can be listed.");
                    printf("%s\n",response);
                    break;
                }
                strcat(response, de->d_name);
                strcat(response, "\n");

            }
            closedir(d);

            printf("Sending dir listing to client\n");

            // send dir listing to the client
            if ((numbytes = sendto(sock,response, strlen(response),0,(struct sockaddr *)&from_addr,  addr_length)) == -1) {
                perror("talker: sendto");
                exit(1);
            }

        }
        else if (strlen(buffer) > 3 && strncmp("put", buffer,3) == 0)
        {
            char command[MAXBUFSIZE];
            // send ack to client
            printf("Sending ack back to client\n");
            strcpy(command,"ack");
            if ((numbytes = sendto(sock,command, strlen(command),0,(struct sockaddr *)&from_addr, addr_length)) == -1) {
                perror("talker: sendto");
                exit(1);
            }

            printf("Waiting to get file name and file size\n");
            // receive the file name and size now
            if ((numbytes = recvfrom(sock, command, sizeof command , 0,
                                     (struct sockaddr *)&from_addr, &addr_length)) == -1)
            {
                perror("recvfrom");
                exit(1);
            }

            // then parse the length and filename
            char *fileName = strtok(command,"/"); // filename

            long fileSize = atol(strtok(NULL," "));  // file size


            // send ack to client
            char ack[4];
            strcpy(ack,"ack");
            if ((numbytes = sendto(sock,ack, strlen(ack),0,(struct sockaddr *)&from_addr, addr_length)) == -1) {
                perror("talker: sendto");
                exit(1);
            }

            char *fileContentsBuffer;
            fileContentsBuffer = malloc(fileSize);


            // receive the actual file contents from the server
            if ((numbytes = recvfrom(sock, fileContentsBuffer, fileSize , 0,
                                     (struct sockaddr *)&from_addr, &addr_length)) == -1)
            {
                perror("recvfrom");
                exit(1);
            }

            // send response to server requesting the file contents now.
            printf("opening file for writing...[%s]\n",fileName);
            FILE *fp = fopen(fileName, "w");
            fwrite(fileContentsBuffer, sizeof(char), numbytes,fp);
            fclose(fp);
            printf("Done writing\n");
            free(fileContentsBuffer);
        }
        else if (strlen(buffer) > 3 && strncmp("get", buffer,3) == 0)
        {

            char *fileName = strtok(buffer," "); // ignore the "get" token
            fileName = strtok(NULL," "); // get the second token which is the file name.
            printf("Sending file %s\n", fileName);

            // open file and find out its length
            FILE *fp = fopen(fileName, "r");
            fseek(fp, 0L, SEEK_END);
            int fileSize = ftell(fp);
            fseek(fp, 0L, SEEK_SET);
            printf("File is size = %d\n", fileSize);

            // read file into buffer
            unsigned char *buffer;
            buffer = malloc(fileSize);
            if (fread(buffer, sizeof(*buffer), fileSize,fp) != fileSize)
            {
                perror("Mismatched read.  Not all bytes were read");
                exit(1);
            }
            fclose(fp);

            // first send the file name and size to the client
            // then send a file over to the client.
            char fileAndSize[MAXBUFSIZE];
            sprintf(fileAndSize,"%s/%d",fileName,fileSize);
            if ((numbytes = sendto(sock,fileAndSize, strlen(fileAndSize),0,(struct sockaddr *)&from_addr,  addr_length)) == -1)
            {
                perror("Unable to send file");
                free(buffer);
                exit(1);
            }

            // wait for client to respond
            char ack[MAXBUFSIZE];

            if ((numbytes = recvfrom(sock, ack, sizeof(ack)-1 , 0,
                                     (struct sockaddr *)&from_addr, &addr_length)) == -1)
            {
                perror("recvfrom");
                exit(1);
            }



            // then send a file over to the client.
            if ((numbytes = sendto(sock,buffer, fileSize,0,(struct sockaddr *)&from_addr,  addr_length)) == -1)
            {
                perror("Unable to send file");

            }
            free(buffer);

        }
        else
        {
            strcpy(response,"Unknown command:  ");
            strcat(response, buffer);
            printf("Sending %s\n", response);

            if ((numbytes = sendto(sock,response, strlen(response),0,(struct sockaddr *)&from_addr,  addr_length)) == -1) {
                perror("sendto error");
                exit(1);
            }

        }


    } // end while
*/
	close(sock);
}


