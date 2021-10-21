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
int timeout_i;
std::string timeout_s;
int blocksize_i;
std::string blocksize_s;
bool multicast;
std::string mode;
bool ipv4;
std::string address;
int port;



void paramSet(){

    readON = true;
    path = "";
    timeout_i = -1;
    timeout_s = "";
    blocksize_i = 512;//TODO este zistit
    blocksize_s = "512";
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
                timeout_i = std::stoi(value);
                timeout_s = std::to_string(timeout_i);
            }catch (...){
                std::cerr << "PARAM ERR: timeout value must be integer\n";
                exit(1);
            }
            //std::cout << "-t value is:" << timeout << ":\n";
        }else if (arg == "-s"){
            try{
                blocksize_i = std::stoi(value);
                blocksize_s = std::to_string(blocksize_i);
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

        Request(){
            this->size = 2 + strlen(path.c_str()) + 1 + strlen(mode.c_str()) + 1 + strlen("blksize") + 1 + strlen(blocksize_s.c_str()) + 1;
            if (timeout_i != -1){
                this->size += strlen("timeout") + 1 + strlen(timeout_s.c_str()) + 1;
            }
            this->message = (char*) malloc(this->size);
            memset(this->message, 0, this->size);

            if (readON){
                this->message[1] = 1;
            }else if (!readON){
                this->message[1] = 2;
            }

            int ptr = 2;

            std::cout << this->size << "\n";


            memcpy(&message[ptr++], path.c_str(), strlen(path.c_str()));
            ptr += strlen(path.c_str());
            memcpy(&message[ptr++], mode.c_str(), strlen(mode.c_str()));
            ptr += strlen(mode.c_str());
            memcpy(&message[ptr++], "blksize", strlen("blksize"));
            ptr += strlen("blksize");
            memcpy(&message[ptr++], blocksize_s.c_str(), strlen(blocksize_s.c_str()));
            ptr += strlen(blocksize_s.c_str());

            if (timeout_i != -1){
                memcpy(&message[ptr++], "timeout", strlen("timeout"));
                ptr += strlen("timeout");
                memcpy(&message[ptr++], timeout_s.c_str(), strlen(timeout_s.c_str()));
                ptr += strlen(timeout_s.c_str());
            }
            /*
            int i = 0;
            while(i<this->size){
                printf("%i\t%i\t%c\n", i, message[i], message[i]);
                i++;
            }
            */


            /*
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
            */
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
    strcat(result, "0");
    
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
    std::cout << tr_msg.size << "\n";

    for (int i = 0; i < tr_msg.size; i++){
        std::cout << (int) tr_msg.message[i] << "\t" << tr_msg.message[i] << "\n";
    }

    //sending request
    sendto(sockfd, (const char *) tr_msg.message, tr_msg.size, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

    char* tr_reply = (char*) malloc((4 + blocksize_i)*sizeof(char));

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    //ack
    char* ack = (char*) malloc(4 * sizeof (char));
    ack[0] = 0;
    ack[1] = 4;
    ack[2] = 0;
    ack[3] = 1;

    //printf("%i\t%i\t%i\t%i\n", ack[0], ack[1], ack[2], ack[3]);


    //std::cout << "Start recieving\n";
    
    unsigned int adrlen = sizeof(servaddr);
    int rec = 0;
    int i = 1;
    FILE* cfile = fopen(path.c_str(), "wb");

    do{
        rec = recvfrom(sockfd, tr_reply, 4+blocksize_i , 0, (struct sockaddr *)&servaddr, &adrlen);
        fwrite(&tr_reply[4], 1, rec - 4, cfile);
        ack[2] = i >> 8;
        ack[3] = i++;
        sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

    }while (rec == blocksize_i + 4);
    
    //printf("BYTES RECIEVED: %i\n", rec);
    //for (int i = 0; i < 30; i++){
    //    printf( "%i=\t%c\t%i\n", i, *(tr_reply + i), *(tr_reply + i));
    //}
    //std::cout << "Stop recieving\n";

    
    
    
    close(sockfd);

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