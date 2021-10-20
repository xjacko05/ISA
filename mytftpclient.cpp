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
#include <fstream>


bool readON;
std::string path;
int timeout;
int blocksize;
bool multicast;
std::string mode;
bool ipv4;
std::string address;
int port;



void paramSet(){

    readON = true;
    path = "";
    timeout = -1;
    blocksize = 512;//TODO este zistit
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
                blocksize = std::stoi(value);
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

class Request{
    public:
        int size;
        char* message;

    void makeRead(){
        this->size = 4 + strlen(path.c_str()) + strlen(mode.c_str());
        this->message = (char*) malloc(this->size * sizeof(char));
	    memset(this->message, 0, size);
	    strcat(this->message, "00");
        strcat(this->message, path.c_str());
        strcat(this->message, "0");
        strcat(this->message, mode.c_str());
        message[0] = 0;
        message[1] = 1;
        message[2 + strlen(path.c_str())] = 0;

    }
};

char* readRequest(){

    int size = 4 + strlen(path.c_str()) + strlen(mode.c_str());
    char *result = (char*) malloc(size * sizeof(char));
	memset(result, 0, size);
	strcat(result, "00");
    strcat(result, path.c_str());
    strcat(result, "0");
    strcat(result, mode.c_str());
    
    return result;
}

void processRequest(){

    int sockfd;
    struct sockaddr_in servaddr; 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(address.c_str()); 
    servaddr.sin_port = htons(port);


    Request tr_msg;
    tr_msg.makeRead();
    std::cout << tr_msg.size << "\n";

    for (int i = 0; i < tr_msg.size; i++){
        std::cout << (int) tr_msg.message[i] << "\t" << tr_msg.message[i] << "\n";
    }

    sendto(sockfd, (const char *) tr_msg.message, tr_msg.size, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

    char* tr_reply = (char*) malloc((4 + blocksize)*sizeof(char));

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));


    std::cout << "Start recieving\n";
    
    unsigned int adrlen = sizeof(servaddr);
    int rec = recvfrom(sockfd, tr_reply, 4+blocksize , 0, (struct sockaddr *)&servaddr, &adrlen);
    printf("BYTES RECIEVED: %i\n", rec);
    for (int i = 0; i < 30; i++){
        printf( "%i=\t%c\t%i\n", i, *(tr_reply + i), *(tr_reply + i));
    }
    std::cout << "Stop recieving\n";

    //ack
    char* ack = (char*) malloc(4 * sizeof (char));
    ack[0] = 0;
    ack[1] = 4;
    ack[2] = 0;
    ack[3] = 1;
    sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
    
    close(sockfd);

    FILE* cfile = fopen(path.c_str(), "wb");
    fwrite(&tr_reply[4], 1, rec - 4, cfile);
    fclose(cfile);
}

int main(){

    paramSet();

    std::cout << ">";
    std::string input;
    getline(std::cin, input);

    paramCheck(input);

    processRequest();





    return 0;
}