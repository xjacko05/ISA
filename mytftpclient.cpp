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
#include <regex>
#include <sys/ioctl.h>
#include <net/if.h>

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
bool lastCR;



void paramSet(){

    readON = true;
    path = "";
    timeout_i = -1;
    timeout_s = "";
    blocksize_i = -1;//TODO este zistit
    blocksize_s = "-1";
    multicast = false;
    mode = "octet";
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
            //https://www.tutorialspoint.com/validate-ipv4-address-using-regex-patterns-in-cplusplus
            std::regex ipv4Re("^(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])$");
            //https://stackoverflow.com/questions/53497/regular-expression-that-matches-valid-ipv6-addresses
            std::regex ipv6Re("([a-fA-F0-9:]+:+)+[a-fA-F0-9]+$");
            if (std::regex_match(address, ipv4Re)){
                ipv4 = true;
            }else if (std::regex_match(address, ipv6Re)){
                ipv4 = false;
            }else{
                std::cerr << "PARAM ERR: invalid address\n";
                exit(1);
            }
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

void writeFile(char* buff, int num, FILE* file){

    if(buff[2] == 0 && buff[3] == 1){
        lastCR = false;
    }
    buff = &buff[4];

    if (mode == "octet"){
        fwrite(buff, 1, num, file);
    }else{
        int counter = 0;
        char* dest = (char*) malloc(num*sizeof(char));
        memset(dest, 0, num);
        for (int i = 0; i < num; i++){
            if (i == num - 1  && buff[i-1] == 13){
                lastCR = true;
                counter++;
                continue;
            }
            if (i == 0){
                if (buff[0] == 0 && lastCR){
                    buff[i] = 13;
                }
                lastCR = false;
            }
            if (i != num - 1){
                if(buff[i] == 13 && buff[i+1] == 10){
                    dest[i-counter] = 10;
                    counter++;
                    i++;
                    continue;
                    //std::cout << "REWRITE CRLF\n";
                }else if(buff[i] == 13 && buff[i+1] == 0){
                    dest[i-counter] = 13;
                    counter++;
                    i++;
                    continue;
                    //std::cout << "REWRITE CRLF\n";
                }
            }
            dest[i-counter] = buff[i];
        }
        /*std::cout << "WRITING THIS";
        for (int i = 0; i< num - counter; i++){

            printf("%i\t%c\n", dest[i], dest[i]);
        }*/
        fwrite(dest, 1, num - counter, file);
        free(dest);
    }
}

void readRequest(){

    int sockfd;
    struct sockaddr_in servaddrivp4;
    struct sockaddr_in6 servaddrivp6;
    //bzero(&servaddr, sizeof(servaddr));

    if (ipv4){
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        servaddrivp4.sin_family = AF_INET;
        servaddrivp4.sin_addr.s_addr = inet_addr(address.c_str()); 
        servaddrivp4.sin_port = htons(port);
    }else{
        sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
        servaddrivp6.sin6_family = AF_INET6;
        //servaddrivp6.sin6_addr.s6_addr = inet_addr(address.c_str());
        inet_pton(AF_INET6, address.c_str(), &servaddrivp6.sin6_addr);
        servaddrivp6.sin6_port  = htons(port);
    }
    



    Request req;
    std::cout << req.size << "\n";

    for (int i = 0; i < req.size; i++){
        std::cout << (int) req.message[i] << "\t" << req.message[i] << "\n";
    }

    //sending request
    if (ipv4){
        sendto(sockfd, (const char *) req.message, req.size, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
    }else{
        sendto(sockfd, (const char *) req.message, req.size, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
    }

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    //OACK
    char* oackBuff = (char*) malloc((4 + 512)*sizeof(char));
    memset(oackBuff, 0, 516);


    unsigned int adrlen = 0;
    int rec = 0;

    if (ipv4){
        adrlen = sizeof(servaddrivp4);
        rec = recvfrom(sockfd, oackBuff, 4+512 , 0, (struct sockaddr *)&servaddrivp4, &adrlen);
    }else{
        adrlen = sizeof(servaddrivp6);
        rec = recvfrom(sockfd, oackBuff, 4+512 , 0, (struct sockaddr *)&servaddrivp6, &adrlen);
    }


    

    if (oackBuff[1] == 6){

        OACK oackVals(oackBuff, rec);
        //TODO vyriesit edge cases upravit bloxksize, timout, skontrolovat free space alebo ukoncit program
        if (timeout_i != -1){
            if (oackVals.timeout == ""){
                std::cout << "NOTICE: server refused timeout value, using default value\n";
            }else if (oackVals.timeout == timeout_s){
                timeout.tv_sec = std::stoi(oackVals.timeout);
                setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
                setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
            }else{
                std::cerr << "SERVER MALFUNCTION proposed different timeout\n";
                exit(1);
            }
        }
        if (blocksize_i != -1){
            if (oackVals.blksize == ""){
                std::cout << "NOTICE: server refused blksize value, using default TFTP value (512)\n";
                blocksize_s = "512";
                blocksize_i = 512;
            }else{// if (oackVals.blksize == blocksize_s){
                blocksize_s = oackVals.blksize;
                blocksize_i = std::stoi(blocksize_s);
            }
        }else{
            blocksize_i = 512;
            blocksize_s = "512";
        }
        if (oackVals.tsize != ""){
            long unsigned tsize = 0;
            try{
                tsize = std::stoi(oackVals.tsize);
            }catch (...){
                std::cerr << "SERVER MALFUNCTION: responded with non numerical tsize value\n";
                exit(1);
            }
            std::filesystem::path p = "./";
            std::filesystem::space_info inf = std::filesystem::space(p);
            if (tsize > inf.capacity){
                std::cerr << "Not enough available storage\n";
                exit(1);
            }
            
        }
    }else if (oackBuff[1] == 8){
        //TODO options refused by server
    }else if (oackBuff[1] == 5){
        std::cerr << &oackBuff[4] << "\n";
        exit(1);
    }

    int ackNum = 0;
    FILE* cfile = fopen(path.filename().c_str(), "wb+");

    char* tr_reply = (char*) malloc((4 + blocksize_i)*sizeof(char));
    memset(tr_reply, 0, blocksize_i + 4);

    //ack
    char* ack = (char*) malloc(4 * sizeof (char));
    ack[0] = 0;
    ack[1] = 4;
    ack[2] = 0;
    ack[3] = ackNum;

    if (oackBuff[1] == 3){
        writeFile(oackBuff, rec - 4, cfile);
        ack[3] = ++ackNum;
    }

    if (ipv4){
        sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
    }else{
        sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
    }
    

    if (oackBuff[1] == 6 || rec == blocksize_i + 4){
        do{
            if(ipv4){
                rec = recvfrom(sockfd, tr_reply, 4+blocksize_i , 0, (struct sockaddr *)&servaddrivp4, &adrlen);
            }else{
                rec = recvfrom(sockfd, tr_reply, 4+blocksize_i , 0, (struct sockaddr *)&servaddrivp6, &adrlen);
            }
            writeFile(tr_reply, rec - 4, cfile);
            memset(tr_reply, 0, blocksize_i + 4);
            ack[2] = ++ackNum >> 8;
            ack[3] = ackNum;
            if (ipv4){
                sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
            }else{
                sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
            }

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
    struct sockaddr_in servaddrivp4;
    struct sockaddr_in6 servaddrivp6;
    //bzero(&servaddr, sizeof(servaddr));

    if (ipv4){
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        servaddrivp4.sin_family = AF_INET;
        servaddrivp4.sin_addr.s_addr = inet_addr(address.c_str()); 
        servaddrivp4.sin_port = htons(port);
    }else{
        sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
        servaddrivp6.sin6_family = AF_INET6;
        //servaddrivp6.sin6_addr.s6_addr = inet_addr(address.c_str()); 
        inet_pton(AF_INET6, address.c_str(), &servaddrivp6.sin6_addr);
        servaddrivp6.sin6_port  = htons(port);
    }


    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

    Request req;
    std::cout << req.size << "\n";

    for (int i = 0; i < req.size; i++){
        std::cout << (int) req.message[i] << "\t" << req.message[i] << "\n";
    }

    //sending request
    if (ipv4){
        sendto(sockfd, (const char *) req.message, req.size, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
    }else{
        sendto(sockfd, (const char *) req.message, req.size, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
    }
/*
    struct ifreq ifr;
    int rr;
    //rr = ioctl(sockfd, SIOCGIFMTU, (caddr_t)&ifr);
    std::cout << "MAX MTU is\t" << rr << "\t" << ifr.ifr_ifrn.ifrn_name << "\n";
*/

    //OACK
    char* oackBuff = (char*) malloc((4 + 512)*sizeof(char));
    memset(oackBuff, 0, 516);

    unsigned int adrlen = 0;
    int rec = 0;

    if (ipv4){
        adrlen = sizeof(servaddrivp4);
        rec = recvfrom(sockfd, oackBuff, 4+512 , 0, (struct sockaddr *)&servaddrivp4, &adrlen);
    }else{
        adrlen = sizeof(servaddrivp6);
        rec = recvfrom(sockfd, oackBuff, 4+512 , 0, (struct sockaddr *)&servaddrivp6, &adrlen);
    }

    

    if (oackBuff[1] == 6){

        OACK oackVals(oackBuff, rec);
        //TODO vyriesit edge cases upravit bloxksize, timout, skontrolovat free space alebo ukoncit program
        if (timeout_i != -1){
            if (oackVals.timeout == ""){
                std::cout << "NOTICE: server refused timeout value, using default value\n";
            }else if (oackVals.timeout == timeout_s){
                timeout.tv_sec = std::stoi(oackVals.timeout);
                setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
                setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
            }else{
                std::cerr << "SERVER MALFUNCTION proposed different timeout\n";
                exit(1);
            }
        }
        if (blocksize_i != -1){
            if (oackVals.blksize == ""){
                std::cout << "NOTICE: server refused blksize value, using default TFTP value (512)\n";
                blocksize_s = "512";
                blocksize_i = 512;
            }else{// if (oackVals.blksize == blocksize_s){
                blocksize_s = oackVals.blksize;
                blocksize_i = std::stoi(blocksize_s);
            }
        }else{
            blocksize_i = 512;
            blocksize_s = "512";
        }
        if (oackVals.tsize != std::to_string(std::filesystem::file_size(path))){
            std::cerr << "SERVER MALFUNCTION: -W did not echo tsize correctly\n";
            exit(1);
        }
    }else if (oackBuff[1] == 8){
        //TODO options refused by server, asi skusit bez options
    }else if (oackBuff[1] == 5){
        std::cerr << &oackBuff[4] << "\n";
        exit(1);
    }

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
        if (ipv4){
            sendto(sockfd, (const char *) dataBlock, num + 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
            rec = recvfrom(sockfd, ack, 4, 0, (struct sockaddr *)&servaddrivp4, &adrlen);
        }else{
            sendto(sockfd, (const char *) dataBlock, num + 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
            rec = recvfrom(sockfd, ack, 4, 0, (struct sockaddr *)&servaddrivp6, &adrlen);
        }
            printf("%i\t%i\n%i\t%i\n\n", ack[2], ack[3], dataBlock[2], dataBlock[3]);
        //restransmission in case of none/incorrect ack
        if (rec == -1 || ack[2] != dataBlock[2] || ack[3] != dataBlock[3]){
            printf("%i\t%i\n%i\t%i\n\n", ack[2], ack[3], dataBlock[2], dataBlock[3]);
            if (ipv4){
                sendto(sockfd, (const char *) dataBlock, num + 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
                rec = recvfrom(sockfd, ack, 4, 0, (struct sockaddr *)&servaddrivp4, &adrlen);
            }else{
                sendto(sockfd, (const char *) dataBlock, num + 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
                rec = recvfrom(sockfd, ack, 4, 0, (struct sockaddr *)&servaddrivp6, &adrlen);
            }
            if (rec == -1 || ack[2] != blockNum >> 8 || ack[3] != blockNum){
                std::cerr << "Retransmission failed\n";
                exit(1);
            }
        }
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