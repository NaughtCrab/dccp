# DCCP proctocol
To evaluate MP-DCCP protocol in two path network. Before the file transimission, net.py is used create two host with two channels. 
## Create the network
    sudo python net.py -i 8 -o 2
## Send file
    ./client <server address> <port> <service code> <file name>
### Example
    ./client 10.0.0.1 1337 42 ~/TestFile/LICENSE

## Receive file
    ./server
