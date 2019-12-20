#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h> // sin_addr
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include "dccp.h"
#include "common.h"


#define BUFFER_SIZE 2800

int writefile(int sock_fd, FILE *fp);

int error_exit(const char *str)
{
	perror(str);
	exit(errno);
}

int main(int argc, char **argv)
{	

	/************************/
	/*        Part I        */
	/************************/

	// Create a master socket
	int listen_sock = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP);	
	if (listen_sock < 0)
		error_exit("socket");
	
	// type of socket created
	struct sockaddr_in servaddr = {
		.sin_family = AF_INET,	
		.sin_addr.s_addr = htonl(INADDR_ANY),	
		.sin_port = htons(PORT),	
	};
	
	// set master socket to allow multiple connections
	if (setsockopt(listen_sock, SOL_DCCP, SO_REUSEADDR, &(int) {1}, sizeof(int)))
		error_exit("setsockopt(SO_REUSEADDR)");
	
	// bind socket with IP, port 
	if (bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)))
		error_exit("bind");

	// DCCP mandates the use of a 'Service Code' in addition the port
	if (setsockopt(listen_sock, SOL_DCCP, DCCP_SOCKOPT_SERVICE, &(int) {htonl(SERVICE_CODE)}, sizeof(int)))
		error_exit("setsockopt(DCCP_SOCKOPT_SERVICE)");
	
	// Using listen() to put socket into passive listening state, until client wake it up
	if (listen(listen_sock, 1))
		error_exit("listen");


	/************************/
	/*       Part II        */
	/************************/

	
	/* create the file to write in */
	FILE *fp = fopen("out.bin", "wb");
	if (fp == NULL) {
		error_exit("Cannot open the file");
	}
	
	/* create selet set */	
	fd_set readset, servset;
	struct timeval timeout = {0, 0};
	int maxfdp, result;

	FD_SET(listen_sock, &servset);
	for (;;) {

		printf("Waiting for connection...\n");
		
		/* create two client socket */
		struct sockaddr_in client_addr;
		struct sockaddr_in client_addr_2;

		socklen_t addr_len = sizeof(client_addr);
		socklen_t addr_len_2 = sizeof(client_addr_2);

		int conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
		if (conn_sock < 0) {
			perror("accept");
			continue;
		}

		int conn_sock_2 = accept(listen_sock, (struct sockaddr *)&client_addr_2, &addr_len_2);
		if (conn_sock_2 < 0) {
			perror("accept");
			continue;
		}

		printf("Connection1 received from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		printf("Connection2 received from %s:%d\n", inet_ntoa(client_addr_2.sin_addr), ntohs(client_addr_2.sin_port));
		
		/* select maxfdp setting, usually max(socketfd)+1 */
		maxfdp = conn_sock > conn_sock_2 ? conn_sock + 1 : conn_sock_2 + 1;
		printf("conn_sock is:%d, conn_sock_2 is: %d, listen_sock is: %d, maxfdp is: %d\n", conn_sock, conn_sock_2, listen_sock, maxfdp);
		
		while (1) {
			/* selet parameters setting*/
			FD_ZERO(&readset);
			FD_SET(conn_sock, &readset);
			FD_SET(conn_sock_2, &readset);
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			result = select(maxfdp, &readset, NULL, NULL, &timeout);

			if (result < 0) {	// don't find active connection
				perror("select error");
				break;
			} 
			else if (result == 0) {	// time is out
				printf("time out \n");
				continue;
			}

			/* Chosen socket is active*/
			if (FD_ISSET(conn_sock, &readset)) {
				printf("read from conection 1\n");
				if (writefile(conn_sock, fp) != 0) {
					break;
				}
			}
			if (FD_ISSET(conn_sock_2, &readset)){
				printf("read from conection 2\n");
				if (writefile(conn_sock_2, fp) != 0) {
					break;
				}
			}

		}
		
		puts("Receive successfully!\n");

		//fclose(fp);
		close(conn_sock);
		close(conn_sock_2);
	}


}

int writefile(int sock_fd, FILE *fp) {
	ssize_t n;
	char buffer[BUFFER_SIZE];
	bzero(buffer, BUFFER_SIZE);

	n = recv(sock_fd, buffer, BUFFER_SIZE, 0);
	if (n == 0) {
		printf("connection %d closed\n", sock_fd);
		return 1;
	} else if (n > 0) {
		printf("recv %ld\n", n);
		/*
		if(fwrite(buffer, sizeof(char), n, fp) < n) {
			perror("Write File Error");
			exit(1);
		}
		*/
		bzero(buffer, BUFFER_SIZE); // Clear the buffer
	} else {
		perror("recv");
		exit(1);
	}
	return 0;
}
