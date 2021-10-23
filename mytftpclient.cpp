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
#include <filesystem>

bool readON;
std::filesystem::path path;
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
            std::cout << "-d value is:" << path << ":\n";
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

    if (!readON && !std::filesystem::exists(path)){
        std::cerr << "PARAM ERR: " << path << "does not exist\n";
        exit(1);
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

            std::string t_size;
            if (readON){
                t_size = "0";
            }else if (!readON){
                t_size = std::to_string(std::filesystem::file_size(path));
            }
            this->size += strlen("tsize") + 1 + strlen(t_size.c_str()) + 1;

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

            memcpy(&message[ptr++], "tsize", strlen("tsize"));
            ptr += strlen("tsize");
            memcpy(&message[ptr++], t_size.c_str(), strlen(t_size.c_str()));
            ptr += strlen(t_size.c_str());

            /*
            int i = 0;
            while(i<this->size){
                printf("%i\t%i\t%c\n", i, message[i], message[i]);
                i++;
            }
            */
        }
};

class OACK{
    public:
        std::string blksize;
        std::string timeout;
        std::string tsize;

        OACK(char* message, int rec){

            blksize = "";
            timeout = "";
            tsize = "";

            for (int ptr = 2; ptr != rec;){
            if (!strcmp((char*)&message[ptr], "blksize")){
                //std::cout << (char*)&message[ptr] << "\t\t";
                ptr += strlen("blksize") + 1;
                blksize = (char*)&message[ptr];
                //std::cout << (char*)&message[ptr] << std::endl;
                ptr += strlen((char*)&message[ptr]) + 1;

            }
            if (!strcmp((char*)&message[ptr], "timeout")){
                //std::cout << (char*)&message[ptr] << "\t\t";
                ptr += strlen("timeout") + 1;
                timeout = (char*)&message[ptr];
                //std::cout << (char*)&message[ptr] << std::endl;
                ptr += strlen((char*)&message[ptr]) + 1;

            }
            if (!strcmp((char*)&message[ptr], "tsize")){
                //std::cout << (char*)&message[ptr] << "\t\t";
                ptr += strlen("tsize") + 1;
                tsize = (char*)&message[ptr];
                //std::cout << (char*)&message[ptr] << std::endl;
                ptr += strlen((char*)&message[ptr]) + 1;

            }
        }
    }
};

void readRequest(){

    int sockfd;
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(address.c_str()); 
    servaddr.sin_port = htons(port);


    Request req;
    std::cout << req.size << "\n";

    for (int i = 0; i < req.size; i++){
        std::cout << (int) req.message[i] << "\t" << req.message[i] << "\n";
    }

    //sending request
    sendto(sockfd, (const char *) req.message, req.size, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    unsigned int adrlen = sizeof(servaddr);
    int rec = 0;

    //OACK
    char* oackBuff = (char*) malloc((4 + 512)*sizeof(char));
    memset(oackBuff, 0, 516);
    rec = recvfrom(sockfd, oackBuff, 4+512 , 0, (struct sockaddr *)&servaddr, &adrlen);

    

    if (oackBuff[1] == 6){

        OACK oackVals(oackBuff, rec);
        //TODO vyriesit edge cases upravit bloxksize, timout, skontrolovat free space alebo ukoncit program
        blocksize_s = oackVals.blksize;
        blocksize_i = std::stoi(blocksize_s);
    }else if (oackBuff[1] == 8){
        //TODO options refused by server
    }else if (oackBuff[1] == 5){
        //TODO normalny err
    }

    int ackNum = 0;
    FILE* cfile = fopen(path.filename().c_str(), "wb");

    char* tr_reply = (char*) malloc((4 + blocksize_i)*sizeof(char));
    memset(tr_reply, 0, blocksize_i + 4);

    //ack
    char* ack = (char*) malloc(4 * sizeof (char));
    ack[0] = 0;
    ack[1] = 4;
    ack[2] = 0;
    ack[3] = ackNum;

    if (oackBuff[1] == 3){
        fwrite(&oackBuff[4], 1, rec - 4, cfile);
        ack[3] = ++ackNum;
    }

    sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

    if (oackBuff[1] == 6 || rec == blocksize_i + 4){
        do{
            rec = recvfrom(sockfd, tr_reply, 4+blocksize_i , 0, (struct sockaddr *)&servaddr, &adrlen);
            fwrite(&tr_reply[4], 1, rec - 4, cfile);
            memset(tr_reply, 0, blocksize_i + 4);
            ack[2] = ++ackNum >> 8;
            ack[3] = ackNum;
            sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

        }while (rec == blocksize_i + 4);
    }

    printf("BYTES RECIEVED: %i\n", rec);
    for (int i = 0; i < 30; i++){
        printf( "%i=\t%c\t%i\n", i, *(tr_reply + i), *(tr_reply + i));
    }
    
    close(sockfd);

    fclose(cfile);
}

void writeRequest(){

    int sockfd;
    struct sockaddr_in servaddr; 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(address.c_str()); 
    servaddr.sin_port = htons(port);


    Request req;
    std::cout << req.size << "\n";

    for (int i = 0; i < req.size; i++){
        std::cout << (int) req.message[i] << "\t" << req.message[i] << "\n";
    }

    //sending request
    sendto(sockfd, (const char *) req.message, req.size, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    unsigned int adrlen = sizeof(servaddr);
    int rec = 0;

    //OACK
    char* oackBuff = (char*) malloc((4 + 512)*sizeof(char));
    memset(oackBuff, 0, 516);
    rec = recvfrom(sockfd, oackBuff, 4+512 , 0, (struct sockaddr *)&servaddr, &adrlen);

    if (oackBuff[1] == 6){

        OACK oackVals(oackBuff, rec);
        //TODO vyriesit edge cases upravit bloxksize, timout, skontrolovat free space alebo ukoncit program
        blocksize_s = oackVals.blksize;
        blocksize_i = std::stoi(blocksize_s);
    }else if (oackBuff[1] == 8){
        //TODO options refused by server
    }else if (oackBuff[1] == 5){
        //TODO normalny err
    }else if (oackBuff[1] == 3){
        
    }

    int ackNum = 0;
    int blockNum = 0;
    int num = 0;
    FILE* cfile = fopen(path.filename().c_str(), "r");

    char* dataBlock = (char*) malloc((4 + blocksize_i)*sizeof(char));
    memset(dataBlock, 0, blocksize_i + 4);
    dataBlock[1] = 3;

    //ack
    char* ack = (char*) malloc(4 * sizeof (char));
    memset(ack, 0, 4);

    do{
        dataBlock[2] = ++blockNum >> 8;
        dataBlock[3] = blockNum;
        num = fread(&dataBlock[4], 1, blocksize_i, cfile);
        sendto(sockfd, (const char *) dataBlock, num + 4, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        rec = recvfrom(sockfd, ack, 4, 0, (struct sockaddr *)&servaddr, &adrlen);
        //restransmission in case of none/incorrect ack
        /*
        if (rec == -1 || (unsigned short) ack[2] != blockNum){
            sendto(sockfd, (const char *) dataBlock, num + 4, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
            rec = recvfrom(sockfd, ack, 4, 0, (struct sockaddr *)&servaddr, &adrlen);
            if (rec == -1 || (unsigned short) ack[2] != blockNum){
                std::cerr << "Retransmission failed\n";
                exit(1);
            }
        }*/
        memset(&dataBlock[4], 0, blocksize_i);
        
    }while (num == blocksize_i);

    printf("BYTES RECIEVED: %i\n", rec);
    for (int i = 0; i < 30; i++){
        printf( "%i=\t%c\t%i\n", i, *(dataBlock + i), *(dataBlock + i));
    }
    
    close(sockfd);

    fclose(cfile);
}

int main(){

    paramSet();

    std::cout << ">";
    std::string input;
    getline(std::cin, input);

    paramCheck(input);

    if (readON){
        readRequest();
    } else if (!readON){
        writeRequest();
    }





    return 0;
}