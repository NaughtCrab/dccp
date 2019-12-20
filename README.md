# DCCP proctocol
To evaluate the performance MP-DCCP protocol in two path network. The code is changed based on DCCP protocol, it should be run under specified ubuntu kernel version 4.15.0-55, and install the supports for DCCP proctrol.  
Before the file transimission, net.py is used create two-host with two channels.  
-i: define the bandwidth of the first channel  
-o: define the bandwidth of the second channel  
## Create the network
    `sudo python net.py -i 8 -o 2`
DCCP is like udp + congestion control. It requires port and service code to build connection. The default value of port is 1337, and service code is 42, the example shows how to run the program.
## Send file
    ./client <server address> <port> <service code> <file name>
### Example
    ./client 10.0.0.1 1337 42 ~/TestFile/LICENSE

## Receive file
    ./server
After finishing the running, the files "data1.txt" and "data2.txt" will be created, data1.txt records the timestamp and transmit rate of path1, data2.txt records the timestamp and transmit rate of path2. The files can be used to evaluate the performance of MP-DCCP.    
    
