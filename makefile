all : client server
.PHONY : all
send_file : client.c dccp.h
	gcc -Wall -O2 client.c -o client
receive_file : server.c dccp.h
	gcc -Wall -O2 server.c -o server
clean :
	rm client server
