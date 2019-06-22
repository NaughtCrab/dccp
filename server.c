#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h> // sin_addr
#include <arpa/inet.h>
#include <errno.h>

#include "dccp.h"

#define PORT 1337
#define SERVICE_CODE 42
#define BUFFER_SIZE 512

void writefile(int sock_fd, FILE *fp);

int error_exit(const char *str)
{
	perror(str);
	exit(errno);
}

int main(int argc, char **argv)
{	

	/* ========== Part I ========= */

	// Create socket and return value
	int listen_sock = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP);	// AF_INET is IPV4 address, IPPROTO_DCCP is dccp protocol
	if (listen_sock < 0)
		error_exit("socket");
	
	// 1. Create sockaddr_in structure
	struct sockaddr_in servaddr = {
		.sin_family = AF_INET,	//Address Familiy, also is Address type like IPV4 and IPV6
		.sin_addr.s_addr = htonl(INADDR_ANY),	// 32 bits IP address
		.sin_port = htons(PORT),	// 16 bits port address
	};

	//?
	if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)))
		error_exit("setsockopt(SO_REUSEADDR)");
	
	// 2. bind socket with IP, port 
	if (bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)))
		error_exit("bind");

	// DCCP mandates the use of a 'Service Code' in addition the port
	if (setsockopt(listen_sock, SOL_DCCP, DCCP_SOCKOPT_SERVICE, &(int) {
		       htonl(SERVICE_CODE)}, sizeof(int)))
		error_exit("setsockopt(DCCP_SOCKOPT_SERVICE)");
	
	// 3. Using listen() to put socket into passive listening state, until client wake it up
	if (listen(listen_sock, 1))
		error_exit("listen");



	/* ========== Part II ==========*/
	for (;;) {

		printf("Waiting for connection...\n");

		struct sockaddr_in client_addr;
		socklen_t addr_len = sizeof(client_addr);

		int conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
		if (conn_sock < 0) {
			perror("accept");
			continue;
		}

		printf("Connection received from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		
		//Get the file name
		char filename[BUFFER_SIZE]; // file name 
		bzero(filename, BUFFER_SIZE); // set all are zero

		int ret = recv(conn_sock, filename, BUFFER_SIZE, 0);
		if( ret < 0) {
			error_exit("Cannot receive the file name");
		}


		// Create the file 
		FILE *fp = fopen(filename, "wb");
		if (fp == NULL) {
			error_exit("Cannot open the file");
		}

		// Write data in to file 
		writefile(conn_sock, fp);
		puts("Receive successfully!\n");

		// Finish 
		fclose(fp);
		close(conn_sock);

		/*
		for (;;) {
			char buffer[1024];
			// Each recv() will read only one individual message.
			// Datagrams, not a stream!
			int ret = recv(conn_sock, buffer, sizeof(buffer), 0);
			if (ret > 0)
				printf("Received: %s\n", buffer);
			else
				break;

		}
		*/
	}
}

void writefile(int sock_fd, FILE *fp) {
	ssize_t n;
	char buffer[BUFFER_SIZE];
	bzero(buffer, BUFFER_SIZE);

	while((n = recv(sock_fd, buffer, BUFFER_SIZE, 0)) > 0) {
		if(fwrite(buffer, sizeof(char), n, fp) < n) {
			perror("Write File Error");
			exit(1);
		}
		bzero(buffer, BUFFER_SIZE); // Clear the buffer
	}

}
