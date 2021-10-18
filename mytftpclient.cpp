#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>


bool readON;
std::string path;
int timeout;
int size;
bool multicast;
std::string mode;
bool ipv4;
std::string address;
int port;



void paramSet(){

    readON = true;
    path = "";
    timeout = -1;
    size = 512;//TODO este zistit
    multicast = false;
    mode = "binary";
    ipv4 = true;
    address = "127.0.0.1";
    port = 69;
}


int paramCheck(std::string arguments){

    arguments = arguments.append(" ");
    //whitespace reduction
    int i = 0;
    while (i < (int) arguments.length()){
        if (isspace(arguments[i])){
            if (i == 0){
                arguments.erase(arguments.begin());
                continue;
            }
            if (isspace(arguments[i-1])){
                arguments.erase(arguments.begin()+i);
                continue;
            }
            arguments[i] = ' ';
        }
        i++;
    }
    if (arguments.find(",") != std::string::npos){
        int pos = arguments.find(",");
        if (isspace(arguments[pos-1])){
            arguments.erase(arguments.begin()+pos-1);
        }
        pos = arguments.find(",");
        if (isspace(arguments[pos+1])){
            arguments.erase(arguments.begin()+pos+1);
        }
    }

    while (arguments.length() != 0){
        std::string arg = arguments.substr(0, arguments.find(' '));
        arguments = arguments.substr(arguments.find(' ')+1);
        std::string value = arguments.substr(0, arguments.find(' '));
        arguments = arguments.substr(arguments.find(' ')+1);
        
        if (arg == "-W"){
            readON = false;
            value = value.append(" ");
            arguments = value.append(arguments);
        }else if (arg == "-R"){
            readON = true;
            value = value.append(" ");
            arguments = value.append(arguments);
        }else if (arg == "-m"){
            multicast = true;
            value = value.append(" ");
            arguments = value.append(arguments);
            //std::cout << "-m value is:" << multicast << ":\n";
        }else if (arg == "-d"){
            path = value;
            //std::cout << "-d value is:" << path << ":\n";
        }else if (arg == "-t"){
            try{
                timeout = std::stoi(value);
            }catch (...){
                std::cerr << "PARAM ERR: timeout value must be integer\n";
                exit(1);
            }
            //std::cout << "-t value is:" << timeout << ":\n";
        }else if (arg == "-s"){
            try{
                size = std::stoi(value);
            }catch (...){
                std::cerr << "PARAM ERR: size value must be integer\n";
                exit(1);
            }
            //std::cout << "-s value is:" << size << ":\n";
        }else if (arg == "-c"){
            mode = value;
            if (mode != "ascii" && mode != "netascii" && mode != "binary" && mode != "octet"){
                std::cerr << "PARAM ERR: unknown mode\n";
                exit(1);
            }
            //std::cout << "-c value is:" << mode << ":\n";
        }else if (arg == "-a"){
            address = value.substr(0, value.find(','));
            try{
                port = std::stoi(value.substr(value.find(',')+1));
            }catch (...){
                std::cerr << "PARAM ERR: port number must be integer\n";
                exit(1);
            }
            //std::cout << "-a value is:" << address << "," << port << ":\n";
        }
        //std::cout << "The rest is:" << arguments << ":\n\n";
    }

    return 1;
}

char* readRequest(){

    char *result = (char*) malloc(3+strlen(path.c_str())+strlen(mode.c_str()));
	memset(result, 0, sizeof(result));
	strcat(result, "01");
    strcat(result, path.c_str());
    strcat(result, "0");
    strcat(result, mode.c_str());
    
    return result;
}

int main(){

    paramSet();

    std::cout << ">";
    std::string input;
    getline(std::cin, input);

    paramCheck(input);


    int sockfd;
    struct sockaddr_in servaddr; 
  
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(address.c_str()); 
    servaddr.sin_port = htons(port);

    //connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    //char* tr_msg = "01test.txt\0ascii";
    char *tr_msg = (char*) malloc(2+strlen("test.txt0netascii"));
	memset(tr_msg, 0, sizeof(tr_msg));
	strcat(tr_msg, "01");//opcode
	strcat(tr_msg, "test.txtknetascii");
    tr_msg[0] = 0;
    tr_msg[1] = 1;
    tr_msg[10] = 0;

    //std::cout << strlen(tr_msg) << "\n";

    //send(sockfd, tr_msg, sizeof(tr_msg), 0);
    sendto(sockfd, (const char *)tr_msg, 20, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
    sleep(1);

    

    char* tr_reply = (char*) malloc(2000*sizeof(char));

    struct timeval timeout;      
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    std::cout << "Start recieving\n";
    //recv(sockfd, tr_reply, 2000 , 0);
    unsigned int adrlen = sizeof(servaddr);
    int rec = recvfrom(sockfd, tr_reply, 2000 , 0, (struct sockaddr *)&servaddr, &adrlen);
    printf("%i%s\n", rec, tr_reply);
    for (int i = 0; i < 30; i++){
        printf( "%i=\t%c\t%i\n", i, *(tr_reply + i), *(tr_reply + i));
    }
    std::cout << "Stop recieving\n";
    close(sockfd);




    return 0;
}