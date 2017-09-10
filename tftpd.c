/* A TCP echo server.
 *
 * Receive a message on port 32000, turn it into upper case and return
 * it to the sender.
 *
 * Copyright (c) 2016, Marcel Kyas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Reykjavik University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MARCEL
 * KYAS NOR REYKJAVIK UNIVERSITY BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

typedef struct request {
    unsigned short opcode;
    char filename[512];
    char mode[8];
}request;

typedef struct data{
    unsigned short opcode;
    unsigned short blocknumber;
    char data[512];
}data;

typedef struct ack
{
    unsigned short opcode;
    unsigned short blocknumber;
}ack;

typedef struct error
{
    unsigned short opcode;
    unsigned short errorcode;
    char errmessage[100];
    unsigned char padding;
}error;

struct request readRequest(char* message)
{
    int i = 0;
    struct request newRequest;
    newRequest.opcode=htons(((struct request*)message)->opcode);
    printf("%d\n", newRequest.opcode);
    char* c = message + 2;
    while(c!='\0'&& i<512)    	    
    {
   	newRequest.filename[i]= *c;
	c++;
	i++;
    } 
    printf("%s\n", newRequest.filename);
    c++;
    i = 0;
    while(c!='\0'&& i<8)    	    
    {
   	newRequest.mode[i]= *c++;
	i++;
    } 
    printf("%s\n", newRequest.mode);
    return newRequest;
}

struct data readData(char* message)
{
    int i = 0;
    struct data newData;
    newData.opcode=htons(((struct data*)message)->opcode);
    printf("%d\n", newData.opcode);
    newData.blocknumber=htons(((struct data*)message)->blocknumber);
    printf("5d\n", newData.blocknumber);
    char* d = message + 4;
    while(d!='\0'&& i<512)
    {
	newData.data[i]= *d;
	d++;
	i++; 
    }
    printf("%s\n",newData.data);
    return newData;
}

struct ack readAck(char* message)
{
    struct ack newAck;
    newAck.opcode=htons(((struct ack*)message)->opcode);
    printf("%d\n", newAck.opcode);
    newAck.blocknumber=htons(((struct ack*)message)->blocknumber); 
    printf("%d\n", newAck.blocknumber);
}


int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in server, client;
    char message[512];

    // Create and bind a TCP socket.
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Network functions need arguments in network byte order instead of
    // host byte order. The macros htonl, htons convert the values.
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    int portnumber = atoi(argv[1]); 
    server.sin_port = htons(portnumber);
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

    // Before the server can accept messages, it has to listen to the
    // welcome port. A backlog of one connection is allowed.
    listen(sockfd, 1);
    printf("Hello world\n");
    for (;;) {
        // We first have to accept a TCP connection, connfd is a fresh
        // handle dedicated to this connection.
        printf("before accept\n");
        socklen_t len = (socklen_t) sizeof(client);
        int connfd = accept(sockfd, (struct sockaddr *) &client, &len);
	printf("before recieve\n");
        // Receive from connfd, not sockfd.
        ssize_t n = recv(connfd, message, sizeof(message) - 1, 0);
		printf("after recieve\n");
		message[n] = '\0';
        fprintf(stdout, "Received:\n%s\n", message);
	
        //printf("%d/n",initialRequest.opcode);
	//printf("%s/n",initialRequest.filename);
	 if(message[1] == 0x1)
	{
		request initialRequest = readRequest(&message);
		readRequest(message);
		printf("%s/n",initialRequest.opcode);
		printf("%s/n",initialRequest.filename);
		printf("%s/n",initialRequest.mode);
        	printf("%s/n", "I am inside of readRequest!");
	}
  
	else if(initialRequest.opcode==3)
        {
	readData(message);
        printf("%s/n", "I am inside of readData!");
 	}
	else if(initialRequest.opcode==4)
        {
	readAck(message);

        printf("%s/n", "I am inside of readAck!");
        }

	// Send the message back.
        send(connfd, message, (size_t) n, 0);

        // Close the connection.
        shutdown(connfd, SHUT_RDWR);
        close(connfd);
    }
}
