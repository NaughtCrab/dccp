#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "dccp.h"
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512

int error_exit(const char *str)
{
	perror(str);
	exit(errno);
}

int main(int argc, char *argv[])
{
	if (argc < 4) {
		printf("Usage: ./client <server address> <port> <service code> " "<message 1> [message 2] ... \n");
		exit(-1);
	}
	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(atoi(argv[2])),
	};

	if (!inet_pton(AF_INET, argv[1], &server_addr.sin_addr.s_addr)) {
		printf("Invalid address %s\n", argv[1]);
		exit(-1);
	}

	int socket_fd = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP);
	if (socket_fd < 0)
		error_exit("socket");

	if (setsockopt(socket_fd, SOL_DCCP, DCCP_SOCKOPT_SERVICE, &(int){htonl(atoi(argv[3]))},
								 sizeof(int)))
		error_exit("setsockopt(DCCP_SOCKOPT_SERVICE)");

	if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
		error_exit("connect");

	// Get the maximum packet size
	uint32_t mps;
	socklen_t res_len = sizeof(mps);
	if (getsockopt(socket_fd, SOL_DCCP, DCCP_SOCKOPT_GET_CUR_MPS, &mps, &res_len))
		error_exit("getsockopt(DCCP_SOCKOPT_GET_CUR_MPS)");
	printf("Maximum Packet Size: %d\n", mps);
	

	// Input Filename, and put it in the buffer waiting to send
    	char file_name[FILE_NAME_MAX_SIZE];
    	char file_save_name[FILE_NAME_MAX_SIZE];
    	bzero(file_name, FILE_NAME_MAX_SIZE+1);
    	printf("Please Input File Name On Server:\t");
     	gets(file_name);
    	printf("Please Input File Name to save On Client:\t");
     	gets(file_save_name);
    
    	char buffer[BUFFER_SIZE];
    	bzero(buffer, BUFFER_SIZE);
    	strncpy(buffer, file_name, strlen(file_name)>BUFFER_SIZE?BUFFER_SIZE:strlen(file_name));

	// send buffer to server
	for (int i = 4; i < argc; i++) {
		if (send(socket_fd, buffer, BUFFER_SIZE, 0) < 0)
			error_exit("send");
	}

	// Wait for a while to allow all the messages to be transmitted
	// usleep(10 * 1000);
	// open the file, wait for writing
    	FILE *fp = fopen(file_save_name, "w");
    	if(NULL == fp){
        	printf("File:\t%s Can Not Open To Write\n", file_save_name);
        	exit(1);
    	}
    
    	// Receving data from server and storing in buffer
    	bzero(buffer, BUFFER_SIZE);
    	int length = 0;
    	int allCount =0;
    	while((length = (int)recv(socket_fd, buffer, BUFFER_SIZE, 0)) > 0){
        	if(fwrite(buffer, sizeof(char), length, fp) < length){
            	printf("File:\t%s Write Failed\n", file_name);
            	break;
        	}
        	allCount+=length;
        	bzero(buffer, BUFFER_SIZE);
    	}
    
    	// After receiving, close file and socket
    	printf("Receive File:\t%s From Server IP Successful! 共%dK\n", file_save_name,allCount);
    	fclose(fp);

	close(socket_fd);
	return 0;
}
