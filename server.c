#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "dccp.h"

#define PORT 1337
#define SERVICE_CODE 42
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512

int error_exit(const char *str)
{
	perror(str);
	exit(errno);
}

int main(int argc, char **argv)
{
	int listen_sock = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP);
	if (listen_sock < 0)
		error_exit("socket");

	struct sockaddr_in servaddr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(PORT),
	};

	if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)))
		error_exit("setsockopt(SO_REUSEADDR)");

	if (bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)))
		error_exit("bind");

	// DCCP mandates the use of a 'Service Code' in addition the port
	if (setsockopt(listen_sock, SOL_DCCP, DCCP_SOCKOPT_SERVICE, &(int) {
		       htonl(SERVICE_CODE)}, sizeof(int)))
		error_exit("setsockopt(DCCP_SOCKOPT_SERVICE)");

	if (listen(listen_sock, 1))
		error_exit("listen");

	for (;;) {

		printf("Waiting for connection...\n");

		struct sockaddr_in client_addr;
		socklen_t addr_len = sizeof(client_addr);

		int conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
		if (conn_sock < 0) {
			perror("accept");
			continue;
		}

		printf("Connection received from %s:%d\n",
		       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		for (;;) {
			struct sockaddr_in client_addr;
        		socklen_t client_addr_length = sizeof(client_addr);
        		// 接受连接请求，返回一个新的socket(描述符)，这个新socket用于同连接的客户端通信
        		// accept函数会把连接到的客户端信息写到client_addr中
        		int new_server_socket_fd = accept(listen_sock, (struct sockaddr*)&client_addr, &addr_len);
        		if(new_server_socket_fd < 0){
            			perror("Server Accept Failed:");
            			break;
        		}


			char buffer[BUFFER_SIZE];
			bzero(buffer, BUFFER_SIZE);
			// Each recv() will read only one individual message.
			// Datagrams, not a stream!
			if ((conn_sock, buffer, BUFFER_SIZE, 0) < 0) {
				perror("Server Receive Data Failed");
				break;
			}


			// 然后从buffer(缓冲区)拷贝到file_name中
        		char file_name[FILE_NAME_MAX_SIZE+1];
        		bzero(file_name, FILE_NAME_MAX_SIZE+1);
        		strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer));
        		printf("%s\n", file_name);
        
        		// 打开文件并读取文件数据
        		FILE *fp = fopen(file_name, "r");
        		if(NULL == fp){
            			printf("File:%s Not Found\n", file_name);
        		}else{
        	    		bzero(buffer, BUFFER_SIZE);
            			int length = 0;
            			int allCount = 0;
            			// 每读取一段数据，便将其发送给客户端，循环直到文件读完为止
            			while((length = (int)fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0){
                			if(send(new_server_socket_fd, buffer, length, 0) < 0){
                    			printf("Send File:%s Failed./n", file_name);
                    			break;
                			}
                			allCount++;
                			bzero(buffer, BUFFER_SIZE);
            			}
            		// 关闭文件
            		fclose(fp);
            		printf("File:%s Transfer Successful! 共%dK\n", file_name,allCount);
        		}
        		// 关闭与客户端的连接
        		close(new_server_socket_fd);							


		}

		close(conn_sock);
	}
}
