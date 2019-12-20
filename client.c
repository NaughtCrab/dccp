#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <libgen.h>	//basename
#include <sys/time.h>

#include "dccp.h"
#include "common.h"

#define BUFFER_SIZE 1400
#define NAME_SIZE 512

struct tfrc_tx_info {
	uint64_t tfrctx_x;
	uint64_t tfrctx_x_recv;
	uint32_t tfrctx_x_calc;
	uint32_t tfrctx_rtt;
	uint32_t tfrctx_p;
	uint32_t tfrctx_rto;
	uint32_t tfrctx_ipi;
};

int error_exit(const char *str);

void sendfile(FILE *fp, FILE *data1, FILE *data2, int socket_fd, int socket_fd_2);


int main(int argc, char *argv[])
{
	if (argc != 4) {
		printf("Usage: ./client <server address1> <server address2> <file name> \n");
		exit(-1);
	}


	/************************/
	/*        Part I        */
	/************************/

	// type of socket 1 created
	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PORT),
	};
	
	// type of socket 2 created
	struct sockaddr_in server_addr_2 = {
		.sin_family = AF_INET,
		.sin_port = htons(PORT),
	};

	
	/* socket 1 configuration */
	if (!inet_pton(AF_INET, argv[1], &server_addr.sin_addr.s_addr)) {
		printf("Invalid address %s\n", argv[1]);
		exit(-1);
	}
	// create socket 1 
	int socket_fd = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP);
	if (socket_fd < 0)
		error_exit("socket");

	if (setsockopt(socket_fd, SOL_DCCP, DCCP_SOCKOPT_SERVICE, &(int){htonl(SERVICE_CODE)}, sizeof(int)))
		error_exit("setsockopt(DCCP_SOCKOPT_SERVICE)");

	if (setsockopt(socket_fd, SOL_DCCP, DCCP_SOCKOPT_CCID, &(uint8_t){3}, sizeof(uint8_t)))
		error_exit("setsockopt(DCCP_SOCKOPT_CCID)");
	
	// set socket 1 to allow multiple connection
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)))
		error_exit("setsockopt(SO_REUSEADDR)");
    
	// connect socket 1
	if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
		error_exit("connect1");


	/* socket 2 configuration */	
	if (!inet_pton(AF_INET, argv[2], &server_addr_2.sin_addr.s_addr)) {
		printf("Invalid address %s\n", argv[2]);
		exit(-1);
	}
	
	// create socket 2
	int socket_fd_2 = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP);
	if (socket_fd_2 < 0)
		error_exit("socket");

	if (setsockopt(socket_fd_2, SOL_DCCP, DCCP_SOCKOPT_SERVICE, &(int){htonl(SERVICE_CODE)}, sizeof(int)))
		error_exit("setsockopt(DCCP_SOCKOPT_SERVICE)");

	if (setsockopt(socket_fd_2, SOL_DCCP, DCCP_SOCKOPT_CCID, &(uint8_t){3}, sizeof(uint8_t)))
		error_exit("setsockopt(DCCP_SOCKOPT_CCID)");
	
	// set socket 2 to allow multiple connection
	if (setsockopt(socket_fd_2, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)))
		error_exit("setsockopt(SO_REUSEADDR)");
	
	// connect socket 2
	if (connect(socket_fd_2, (struct sockaddr *)&server_addr_2, sizeof(server_addr)))
		error_exit("connect2");	


	/************************/
	/*       Part II        */
	/************************/

	/* Create file to save data */
	FILE *data1 = fopen("data1.txt", "w+");
	if(data1 == NULL) {
		error_exit("Failed to create data1.txt");
	}

	FILE *data2 = fopen("data2.txt", "w+");
	if(data2 == NULL) {
		error_exit("Failed to create data2.txt");
	}
	/* Start file transfer */
	// get the file name and address
	char *filename = basename(argv[3]); 
	if(filename == NULL) {
		error_exit("File is not exist");
	}
	printf("The File Name is: %s\n", filename);

	/* send file name

	char file_name[NAME_SIZE];
	bzero(file_name, NAME_SIZE);
	strncpy(file_name, filename, strlen(filename));
	if(send(socket_fd, file_name, NAME_SIZE, 0) < 0) {
		error_exit("Failed to send file name");
	}
	*/
	
	// Open the file 
	FILE *fp = fopen(argv[3], "rb");
	if(fp == NULL) {
		error_exit("FOPEN");
	}

	// begin to send the file 
	struct timeval tp1, tp2;
	int time_start, time_end;
	gettimeofday(&tp1, NULL);
	time_start = tp1.tv_sec * 1000 + tp1.tv_usec/1000;

	sendfile(fp, data1, data2, socket_fd, socket_fd_2);
	puts("Send successfully");
	
	/* closing */
	gettimeofday(&tp2, NULL);
	time_end = tp2.tv_sec * 1000 + tp2.tv_usec/1000;
	printf("Start time is %d, End time is %d\n", time_start, time_end);
	printf("The total run time is: %d\n", (time_end - time_start));

	// close the transfer file
	fclose(fp);

	// close the data file 
	fclose(data1);
	fclose(data2);

	// close the socket 1 and 2
	close(socket_fd);
	close(socket_fd_2);
	return 0;
}


int error_exit(const char *str)
{
	perror(str);
	exit(errno);
}

void sendfile(FILE *fp, FILE *data1, FILE *data2, int socket_fd, int socket_fd_2) {
	struct tfrc_tx_info tx_info;
	unsigned int tx_info_size;

	int n = 0;		// data size each time
	int total = 0;	// total data size 
	int count = 0;	// choose destination
	int ret1, ret2;

	char sendline[BUFFER_SIZE];		// buffer to store data
	bzero(sendline, BUFFER_SIZE);	// clear the buffer

	tx_info_size = sizeof(tx_info);

	// time stamp of socket 1 and 2
	struct timeval tp1, tp2;
	// time variables of socket 1
	int s1_tx1 = 0;
	int s1_tx2 = 0;
	
	// time variables of socket 2
	int s2_tx1 = 0;
	int s2_tx2 = 0;

	while((n = fread(sendline, sizeof(char), BUFFER_SIZE, fp)) > 0) {
		while (1) {
			count += 1;
			if (count % 2 != 0) {
				ret1 = send(socket_fd, sendline, n, 0);		
				if (ret1 < 0) {
					if (errno == EAGAIN) {
						usleep(3000);
						continue;
					}
					printf("ret %d\n", ret1);
					perror("Socket1: Failed to send file");
					exit(1);
				} else {
					break;
				}

			
			} else {
				ret2 = send(socket_fd_2, sendline, n, 0);		
				if (ret2 < 0) {
					if (errno == EAGAIN) {
						usleep(3000);	//set retry time, not sure???
						continue;
					}
					perror("Socket2: Failed to send file");
					exit(1);
				} else {
					break;
				}
			}
		}
		
		/* get the data of socket 1 */
		if (getsockopt(socket_fd, SOL_DCCP, DCCP_SOCKOPT_CCID_TX_INFO, &tx_info, &tx_info_size))
			perror("getsockopt(DCCP_SOCKOPT_CCID_TX_INFO)");

		s1_tx1 = tx_info.tfrctx_x;
		// get time stamp for each transfer in socket 1
		if (s1_tx1 != s1_tx2) {
			s1_tx2 = s1_tx1; 
			gettimeofday(&tp1, NULL);			
			fprintf(data1, "%ld ", (tp1.tv_sec % 1000) * 1000000 + tp1.tv_usec);
			fprintf(data1, "%lu\n", tx_info.tfrctx_x);
		}
		fprintf(stderr, "Socket1 rate: %lu rtt: %u loss: %u\n", tx_info.tfrctx_x, tx_info.tfrctx_rtt, tx_info.tfrctx_p);
		
		/* get the data of socket 2 */
		if (getsockopt(socket_fd_2, SOL_DCCP, DCCP_SOCKOPT_CCID_TX_INFO, &tx_info, &tx_info_size))
			perror("getsockopt(DCCP_SOCKOPT_CCID_TX_INFO)");

		s2_tx1 = tx_info.tfrctx_x;
		// get time stamp for each transfer in socket 2
		if (s2_tx1 != s2_tx2) {
			s2_tx2 = s2_tx1;
			gettimeofday(&tp2, NULL);
			fprintf(data2, "%ld ", (tp2.tv_sec % 1000) * 1000000 + tp2.tv_usec);
			fprintf(data2, "%lu\n", tx_info.tfrctx_x);
		}
		fprintf(stderr, "Socket2 rate: %lu rtt: %u loss: %u\n", tx_info.tfrctx_x, tx_info.tfrctx_rtt, tx_info.tfrctx_p);
		
		total += n;
		fprintf(stderr, "total: %d\n", total);

		usleep(1);
		// clear the buffer for next transfer
		bzero(sendline, BUFFER_SIZE); 
	}
}


