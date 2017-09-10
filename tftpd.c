#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
//TFTP Formats

//Read requst
typedef struct request {
    unsigned short opcode;
    char filename[512];
    char mode[8];
}request;

//Data
typedef struct data{
    unsigned short opcode;
    unsigned short blocknumber;
    char data[512];
}data;

//Acknowledgment (ACK)
typedef struct ack
{
    unsigned short opcode;
    unsigned short blocknumber;
}ack;

//Error
typedef struct error
{
    unsigned short opcode;
    unsigned short errorcode;
    char errmessage[100];
    unsigned char padding;
}error;

//Reading from RRQ packet
request readRequest(char* message)
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

//Reading from data packet
data readData(char* message)
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

//Reading from ack packet
ack readAck(char* message)
{
    struct ack newAck;
    newAck.opcode=htons(((struct ack*)message)->opcode);
    printf("%d\n", newAck.opcode);
    newAck.blocknumber=htons(((struct ack*)message)->blocknumber); 
    printf("%d\n", newAck.blocknumber);
}

//Combine directories
char* combDir(char requestedFile[], char dir[])
{
    char* filepath = malloc(sizeof(char)*200);
    char* cwd;
    char buff[200];
    cwd = getcwd(buff, 200);
    if(filepath!= 0)
    {
	printf("My working directory is %s.\n", cwd);
    }
	
    strcat(filepath, cwd);
    strcat(filepath, "/");
    strcat(filepath, dir);
    strcat(filepath, "/");
    strcat(filepath, requestedFile);
    strcat(filepath, "\0");	
    printf("Filepath %s \n", filepath ); 
    return filepath;
}

//Check if filepath exists
bool fileExists(char requestedFile[], char dir[])
{
    char* filepath;
    filepath = combDir(requestedFile, dir);
    printf("2.filepath = %s\n", filepath); 
	 
    if(access(filepath, F_OK) != -1)
    {
	//file exists
	return true;	
    }
    else
    {
	//file does not exist
	return false;
    }
    free(filepath);    
}

//Sending error
void serror(int socketfd, struct sockaddr_in client, char errorMessage[]){
    error newerror;
    newerror.opcode = 5;
    newerror.errorcode = -1;
    memset(newerror.errmessage, 0, sizeof(newerror.errmessage));
    strncpy(newerror.errmessage, errmessage, strlen(errmessage));
    sendto(socketfd, &newerror, 512, 0, (struct sockaddr *) &client, sizeof(newerror));
}

bool packetIsSmallerThen512 = false; 
void sendDataPacket(int socketfd, struct sockaddr_in client, int blocknr, FILE *fp) {
    
    int sizeLeft = 0;
    data packet1;
    packet1.opcode = htons(3);
    packet1.blocknumber = htons(blocknr);
    
    //read to packet1.data while the size is 512, when < 512 we do
    // it once more and send the last packet 
    if ((sizeLeft = fread(packet1.data, 1, 512, fp)) < 512) {

        packet1.data[sizeLeft] = '\0';
       
        //send last part of the requested file with the right size 
        if(sendto(socketfd, &packet1, sizeLeft+4, 0,
                  (struct sockaddr *) &client,
                  (socklen_t) sizeof(client)) < 0) {
            perror("error in sendto\n");
        }
        
        //close the file 
        fclose(fp);
        
        //set global bool variable to true 
        packetIsSmallerThen512 = true;
        
        return;
    }
    
    //send DATA packet to client 
    if(sendto(socketfd, &packet1, sizeof(packet1), 0,
              (struct sockaddr *) &client,
              (socklen_t) sizeof(client)) < 0) {
        perror("error in sendto\n");
    }
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
	//RRQ
	 if(message[1] == 0x1)
	{
		char net[50] = "netascii";
		char oct[50] = "octet";
		request initialRequest = readRequest(&message);
		readRequest(message);
		printf("%s/n",initialRequest.opcode);
		printf("%s/n",initialRequest.filename);
		printf("%s/n",initialRequest.mode);
        	printf("%s/n", "I am inside of readRequest!");
		char directory[10];
		strcpy(directory, argv[2]);
		//checking if file exists, if not print errors
		if(fileExists(readRequest.filename, directory) == false)
		{
			//**error handling needed**
			serror(sockfd, client, "File does not exist");
			printf("file does not exist \n");
		}
		//before opening the file we want to check in which mode we are in
		char* filepath = combDir(initialRequest.filename,directory);
		FILE *fp = 0;
		//checking if it is in the mode netascii
		if(strstr(initialRequest.mode, net) != NULL)
		{
			printf("Netascii mode! \n");
			fp = fopen(filepath, "r");
			//seek to the beginning of the file
			fseek(fp, SEEK_SET,0); 
		}
		else if(strstr(initialRequest.mode, oct) != NULL)
		{
			printf("octed mode! \n");
			fp = fopen(filepath, "rb");
			//seek to the beginning of the file
			fseek(fp, SEEK_SET, 0);
		}
		else
		{
			printf("Not supposed to get here \n");
		}

		//seek to the beginning of the file
		fseek(fp, SEEK_SET, 0);

		//send DATA packet to client
		fprintf(stdout, "Sending file: %s\n", initialRequest.filename);
		sendDataPacket(sockfd, client, blocknumber, fp); 
	}
  
	//else if(initialRequest.opcode==3)
	//Data
	else if(message[1] == 0x3)
        {
		//readData(message);
		error newerror;
		newerror.opcode = 0x5;
		newerror.errorcode = 0x4;
		char* snt = "Sending a data?";
		strncpy(newerror.errmessage, src, 20);
       		printf("%s/n", "I am inside of readData!");
 	}
	else if(message[1] == 0x4)
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
