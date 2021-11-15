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
#include <ctime>
#include <boost/algorithm/string.hpp>

bool readON;
std::filesystem::path path;
int timeout_i;
std::string timeout_s;
long long blocksize_i;
std::string blocksize_s;
bool multicast;
std::string mode;
bool ipv4;
std::string address;
int port;
bool lastCR;
char lastC;


//sets parameters to their default values
void paramSet(){

    readON = true;
    path = "";
    timeout_i = -1;
    timeout_s = "";
    blocksize_i = -1;
    blocksize_s = "512";
    multicast = false;
    mode = "octet";
    ipv4 = true;
    address = "127.0.0.1";
    port = 69;
    lastC = 69;
}


//check if parameters are present and valid
bool paramCheck(std::string arguments){

    if(feof(stdin)){
        std::cout << "\n";
        return false;
    }

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

    bool readwriteSet = false;

    //argument parsing
    while (arguments.length() != 0){
        std::string arg = arguments.substr(0, arguments.find(' '));
        arguments = arguments.substr(arguments.find(' ')+1);
        std::string value = arguments.substr(0, arguments.find(' '));
        arguments = arguments.substr(arguments.find(' ')+1);
        
        if (arg == "-W"){
            if(readwriteSet){
                std::cerr << "PARAM ERR: -W and -R parameters can't be used simultaneously\n";
                return false;
            }else{
                readwriteSet = true;
            }
            readON = false;
            value = value.append(" ");
            arguments = value.append(arguments);
        }else if (arg == "-R"){
            if(readwriteSet){
                std::cerr << "PARAM ERR: -W and -R parameters can't be used simultaneously\n";
                return false;
            }else{
                readwriteSet = true;
            }
            readON = true;
            value = value.append(" ");
            arguments = value.append(arguments);
        }else if (arg == "-m"){
            multicast = true;
            value = value.append(" ");
            arguments = value.append(arguments);
        }else if (arg == "-d"){
            path = value;
        }else if (arg == "-t"){
            try{
                timeout_i = std::stoi(value);
                timeout_s = std::to_string(timeout_i);
            }catch (...){
                std::cerr << "PARAM ERR: timeout value must be integer\n";
                return false;
            }
        }else if (arg == "-s"){
            try{
                blocksize_i = std::stoll(value);
                blocksize_s = std::to_string(blocksize_i);
            }catch (...){
                std::cerr << "PARAM ERR: size value must be integer\n";
                return false;
            }
        }else if (arg == "-c"){
            mode = value;
            boost::algorithm::to_lower(mode);
            if (mode != "ascii" && mode != "netascii" && mode != "binary" && mode != "octet"){
                std::cerr << "PARAM ERR: unknown mode\n";
                return false;
            }
        }else if (arg == "-a"){
            address = value.substr(0, value.find(','));
            std::regex ipv4Re("^(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])$");
            std::regex ipv6Re("^[0123456789a-fA-F\\.:]{2,}$");
            if (std::regex_match(address, ipv4Re)){
                ipv4 = true;
            }else if (std::regex_match(address, ipv6Re)){
                ipv4 = false;
            }else{
                std::cerr << "PARAM ERR: invalid address\n";
                return false;
            }
            try{
                port = std::stoi(value.substr(value.find(',')+1));
            }catch (...){
                std::cerr << "PARAM ERR: port number must be integer\n";
                return false;
            }
        }
    }

    if(!readwriteSet || path == ""){
        std::cerr << "PARAM ERR: " << "invalid param(s)\n";
        return false;
    }

    if (!readON && !std::filesystem::exists(path)){
        std::cerr << "PARAM ERR: " << path << "does not exist\n";
        return false;
    }

    if (multicast && !ipv4){
        std::cerr << "ipv6 Multicast not supported :(\n";
        return false;
    }

    return true;
}


//class used to parse arguments into the request packet
class Request{
    public:
        int size;
        char* message;

        Request(){
            this->size = 2 + strlen(mode.c_str()) + 1 + strlen("blksize") + 1 + strlen(blocksize_s.c_str()) + 1;
            if (timeout_i != -1){
                this->size += strlen("timeout") + 1 + strlen(timeout_s.c_str()) + 1;
            }
            if (multicast){
                this->size += strlen("multicast") + 1;
            }

            std::string t_size;
            if (readON){
                t_size = "0";
                this->size += strlen(path.c_str()) + 1;
            }else if (!readON){
                t_size = std::to_string(std::filesystem::file_size(path));
                this->size += strlen(path.filename().c_str()) + 1;
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

            if (readON){
                memcpy(&message[ptr++], path.c_str(), strlen(path.c_str()));
                ptr += strlen(path.c_str());
            }else{
                memcpy(&message[ptr++], path.filename().c_str(), strlen(path.filename().c_str()));
                ptr += strlen(path.filename().c_str());
            }
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
            if (multicast){
                memcpy(&message[ptr++], "multicast", strlen("multicast"));
                ptr += strlen("multicast");
            }

            memcpy(&message[ptr++], "tsize", strlen("tsize"));
            ptr += strlen("tsize");
            memcpy(&message[ptr++], t_size.c_str(), strlen(t_size.c_str()));
            ptr += strlen(t_size.c_str());
        }

        ~Request(){
            free(this->message);
        }
};


//prints timestamp
void printTime(){

    std::time_t result = std::time(nullptr);
    char tmp[100];
    std::strftime(tmp, sizeof(tmp), "[%Y-%m-%d %H:%M:%S] " ,std::localtime(&result));
    std::cout << tmp;
}


//class used to parse Option acknowledgement packet
class OACK{
    public:
        std::string blksize;
        std::string timeout;
        std::string tsize;
        std::string address;
        std::string port;

        void parse(char* message, int rec){

            blksize = "";
            timeout = "";
            tsize = "";
            address = "";
            port = "";

            for (int i = 2; i != rec; i++){
                //printf("%i\t%c\t%i\n", i, message[i], message[i]);
                if (message[i] > 64 && message[i] < 91){
                    message[i] += 32;
                }
            }

            for (int ptr = 2; ptr != rec;){
            if (!strcmp((char*)&message[ptr], "blksize")){
                ptr += strlen("blksize") + 1;
                blksize = (char*)&message[ptr];
                ptr += strlen((char*)&message[ptr]) + 1;

            }else if (!strcmp((char*)&message[ptr], "timeout")){
                ptr += strlen("timeout") + 1;
                timeout = (char*)&message[ptr];
                ptr += strlen((char*)&message[ptr]) + 1;

            }else if (!strcmp((char*)&message[ptr], "tsize")){
                ptr += strlen("tsize") + 1;
                tsize = (char*)&message[ptr];
                ptr += strlen((char*)&message[ptr]) + 1;

            }else if (!strcmp((char*)&message[ptr], "multicast")){
                ptr += strlen("multicast") + 1;
                while (message[ptr] != ','){
                    address += message[ptr++];
                }
                ptr += 1;
                while (message[ptr] != ','){
                    port += message[ptr++];
                }
                //ptr+=3;
            }else{
                ptr++;
            }
        }
    }
};


//writes block of data into the file, converts data from telnet spec if necessary
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
            if (i == num - 1  && buff[i] == 13){
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
                }else if(buff[i] == 13 && buff[i+1] == 0){
                    dest[i-counter] = 13;
                    counter++;
                    i++;
                    continue;
                }
            }
            dest[i-counter] = buff[i];
        }
        fwrite(dest, 1, num - counter, file);
        free(dest);
    }
}


//closes socket and file, deallocates memory
void cleanup(int sockfd, FILE* file, char* arr[3]){

    close(sockfd);
    if (file != nullptr){
        fclose(file);
    }
    for (int i = 0; i < 3; i++){
        if (arr[i] != nullptr){
            free(arr[i]);
        }
    }
}


//function executing read request
int readRequest(){

    //socket creation
    int sockfd;
    struct sockaddr_in servaddrivp4;
    struct sockaddr_in6 servaddrivp6;

    if (ipv4){
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        servaddrivp4.sin_family = AF_INET;
        servaddrivp4.sin_addr.s_addr = inet_addr(address.c_str()); 
        servaddrivp4.sin_port = htons(port);
    }else{
        sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
        servaddrivp6.sin6_family = AF_INET6;
        inet_pton(AF_INET6, address.c_str(), &servaddrivp6.sin6_addr);
        servaddrivp6.sin6_port  = htons(port);
    }
    
    if (sockfd == -1){
        printTime();
        std::cerr << "ERROR: Socket creation failed\n";
        return 1;
    }

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    if (
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0
        ||
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0
    ){
        printTime();
        std::cerr << "ERROR: Socket options setting failed\n";
        close(sockfd);
        return 1;
    }

    Request req;

    printTime();
    std::cout << "Requesting READ of " << path << " from " << address << ":" << port << "\n";

    //sending request itself
    if (ipv4){
        sendto(sockfd, (const char *) req.message, req.size, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
    }else{
        sendto(sockfd, (const char *) req.message, req.size, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
    }

    //OACK
    char* oackBuff = (char*) malloc((4 + 512)*sizeof(char));
    memset(oackBuff, 0, 516);
    char* toclean[3];
    toclean[0] = oackBuff;
    toclean[1] = nullptr;
    toclean[2] = nullptr;

    unsigned int adrlen = 0;
    int rec = 0;

    if (ipv4){
        adrlen = sizeof(servaddrivp4);
        rec = recvfrom(sockfd, oackBuff, 4+512 , 0, (struct sockaddr *)&servaddrivp4, &adrlen);
    }else{
        adrlen = sizeof(servaddrivp6);
        rec = recvfrom(sockfd, oackBuff, 4+512 , 0, (struct sockaddr *)&servaddrivp6, &adrlen);
    }

    if (rec == -1){
        printTime();
        std::cerr << "ABORTING TRANSFER: no reply recieved from " << address << ":" << port << "\n";
        cleanup(sockfd, nullptr, toclean);
        return 1;
    }
    
    unsigned long long tsize = 0;

    //parsing server reply
    OACK oackVals;
    if (oackBuff[1] == 6){

        oackVals.parse(oackBuff, rec);
        if (timeout_i != -1){
            if (oackVals.timeout == ""){
                std::cout << "NOTICE: server refused timeout value, using default value\n";
            }else if (oackVals.timeout == timeout_s){
                try{
                    timeout.tv_sec = std::stoi(oackVals.timeout);
                }catch (...){
                    std::cerr << "ABORTING TRANSFER: server responded with non numerical timeout value\n";
                    cleanup(sockfd, nullptr, toclean);
                    return 1;
                }
                if(
                    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0
                    ||
                    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0
                ){
                    printTime();
                    std::cerr << "ERROR: Socket options setting failed\n";
                    cleanup(sockfd, nullptr, toclean);
                    return 1;
                }
            }else{
                std::cerr << "ABORTING TRANSFER: server proposed different timeout\n";
                cleanup(sockfd, nullptr, toclean);
                return 1;
            }
        }
        if (blocksize_i != -1){
            if (oackVals.blksize == ""){
                std::cout << "NOTICE: server refused blksize value, using default TFTPv2 value (512)\n";
                blocksize_s = "512";
                blocksize_i = 512;
            }else{
                try{
                    blocksize_i = std::stoll(blocksize_s);
                    if(oackVals.blksize != blocksize_s){
                        std::cout << "NOTICE: using blksize value proposed by server (" << blocksize_i << ")\n";
                    }
                    blocksize_s = oackVals.blksize;
                }catch (...){
                    std::cerr << "ABORTING TRANSFER: responded with non numerical blksize value\n";
                    cleanup(sockfd, nullptr, toclean);
                    return 1;
                }
            }
        }else{
            blocksize_i = 512;
            blocksize_s = "512";
        }
        if (oackVals.tsize != ""){
            try{
                tsize = std::stoull(oackVals.tsize);
            }catch (...){
                std::cerr << "ABORTING TRANSFER: server responded with non numerical tsize value\n";
                cleanup(sockfd, nullptr, toclean);
                return 1;
            }
            std::filesystem::path p = "./";
            std::filesystem::space_info inf = std::filesystem::space(p);
            if (tsize > inf.capacity){
                std::cerr << "Not enough available storage\n";
                exit(1);
            }
            
        }
    }else if(oackBuff[1] == 4){
        std::cout << "NOTICE: server does not support option negotiation, using default TFTPv2 values\n";
        blocksize_i = 512;
        blocksize_s = "512";
    }else if (oackBuff[1] == 8){
        std::cerr << "ABORTING TRANSFER: (err code 8) - terminated by server due to option negotiation (" << &oackBuff[4] << ")\n";
        cleanup(sockfd, nullptr, toclean);
        return 1;
    }else if (oackBuff[1] == 5){
        std::cerr << "ABORTING TRANSFER: (err code 5) -" << &oackBuff[4] << "\n";
        cleanup(sockfd, nullptr, toclean);
        return 1;
    }

    u_int yes = 1;
    struct sockaddr_in m4addr;
    struct ip_mreq mreq;
    int msock;
    unsigned int m4len = 0;
    if (multicast){
        msock = socket(AF_INET, SOCK_DGRAM, 0);
        if (msock == -1){
            printTime();
            std::cerr << "ERROR: Socket creation failed\n";
            cleanup(sockfd, nullptr, toclean);
            return 1;
        }
        if (setsockopt(msock, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0){
            printTime();
            std::cerr << "ERROR: Address reuse failed\n";
            cleanup(sockfd, nullptr, toclean);
            close(msock);
            return 1;
        }

        memset(&m4addr, 0, sizeof(m4addr));
        m4addr.sin_family = AF_INET;
        m4addr.sin_addr.s_addr = htonl(INADDR_ANY);
        m4addr.sin_port = htons(std::stoi(oackVals.port));
        m4len = sizeof(m4addr);

        if (bind(msock, (struct sockaddr*) &m4addr, sizeof(m4addr)) < 0){
            printTime();
            std::cerr << "ERROR: Address binding failed\n";
            cleanup(sockfd, nullptr, toclean);
            close(msock);
            return 1;
        }

        mreq.imr_multiaddr.s_addr = inet_addr(oackVals.address.c_str());
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(msock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0){
            printTime();
            std::cerr << "ERROR: Joining multicast group failed\n";
            cleanup(sockfd, nullptr, toclean);
            close(msock);
            return 1;
        }
    }

    unsigned long ackNum = 0;
    FILE* cfile = fopen(path.filename().c_str(), "wb+");

    char* tr_reply = (char*) malloc((4 + blocksize_i)*sizeof(char));
    memset(tr_reply, 0, blocksize_i + 4);
    toclean[1] = tr_reply;

    //ack
    char* ack = (char*) malloc(4 * sizeof (char));
    toclean[2] = ack;
    ack[0] = 0;
    ack[1] = 4;
    ack[2] = 0;
    ack[3] = ackNum;

    if (oackBuff[1] == 3){
        writeFile(oackBuff, rec - 4, cfile);
        ack[3] = ++ackNum;
    }

    printTime();
    std::cout << "Starting transport...\n";

    if (ipv4){
        sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
    }else{
        sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
    }
    
    //actual data transport
    if (oackBuff[1] == 6 || rec == blocksize_i + 4){
        do{
            if (multicast){
                rec = recvfrom(msock, tr_reply, 4+blocksize_i , 0, (struct sockaddr *)&m4addr, &m4len);
            }else if(ipv4){
                rec = recvfrom(sockfd, tr_reply, 4+blocksize_i , 0, (struct sockaddr *)&servaddrivp4, &adrlen);
            }else{
                rec = recvfrom(sockfd, tr_reply, 4+blocksize_i , 0, (struct sockaddr *)&servaddrivp6, &adrlen);
            }
            //restransmission in case of none/incorrect ack
            if (rec == -1 || (ack[2] == tr_reply[2] && ack[3] == tr_reply[3])){
                std::cout << "NOTICE: block no. " << ackNum + 1 << " not recieved, retransmitting ack...\n";
                if (multicast){
                    sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
                    rec = recvfrom(msock, tr_reply, 4+blocksize_i , 0, (struct sockaddr *)&m4addr, &m4len);
                }else if(ipv4){
                    sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
                    rec = recvfrom(sockfd, tr_reply, 4+blocksize_i , 0, (struct sockaddr *)&servaddrivp4, &adrlen);
                }else{
                    rec = recvfrom(sockfd, tr_reply, 4+blocksize_i , 0, (struct sockaddr *)&servaddrivp6, &adrlen);
                    sendto(sockfd, (const char *) ack, 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
                }
                if (rec == -1 || ack[2] + 1 != tr_reply[2] || ack[3] + 1 != tr_reply[3]){
                    std::cerr << "ABORTING TRANSFER: connection interrupted\n";
                    cleanup(sockfd, cfile, toclean);
                    if (multicast){close(msock);}
                    return 1;
                }
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

    printTime();
    std::cout << "Transport finished sucessfully\n";

    cleanup(sockfd, cfile, toclean);
    if (multicast){close(msock);}

    return 0;
}


//read block of data from the file, converts data to telnet spec if necessary
int readFile(char* buff, int num, FILE* file){

    int bytesRead = 0;

    if(mode == "octet"){
        bytesRead = fread(buff, 1, num, file);
    }else{
        char c;
        for (int i = 0; i < num; i++){
            if (i == 0){
                if(lastC == 0 || lastC == 10){
                    buff[i] = lastC;
                    lastC = 69;
                    continue;
                }
            }
            c = fgetc(file);
            if(feof(file)){
                return i;
            }
            if (i == num - 1){
                if(c == 10){
                    buff[i] = 13;
                    lastC = 10;
                    continue;
                }else if (c == 13){
                    buff[i] = 13;
                    lastC = 0;
                    continue;
                }
            }
            if (c == 10){
                buff[i] = 13;
                buff[i+1] = 10;
                i++;
            }else if (c == 13){
                buff[i] = 13;
                buff[i+1] = 0;
                i++;
            }else{
                buff[i] = c;
            }
        }
        bytesRead = num;
    }
    return bytesRead;
}


//function executing write request
int writeRequest(){

    //socket creation
    int sockfd;
    struct sockaddr_in servaddrivp4;
    struct sockaddr_in6 servaddrivp6;

    if (ipv4){
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        servaddrivp4.sin_family = AF_INET;
        servaddrivp4.sin_addr.s_addr = inet_addr(address.c_str()); 
        servaddrivp4.sin_port = htons(port);
    }else{
        sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
        servaddrivp6.sin6_family = AF_INET6;
        inet_pton(AF_INET6, address.c_str(), &servaddrivp6.sin6_addr);
        servaddrivp6.sin6_port  = htons(port);
    }

    if (sockfd == -1){
        printTime();
        std::cerr << "ERROR: Socket creation failed\n";
        return 1;
    }

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    if (
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0
        ||
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0
    ){
        printTime();
        std::cerr << "ERROR: Socket options setting failed\n";
        close(sockfd);
        return 1;
    }

    Request req;

    printTime();
    std::cout << "Requesting WRITE of " << path << " TO " << address << ":" << port << "\n";

    //sending request itself
    if (ipv4){
        sendto(sockfd, (const char *) req.message, req.size, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
    }else{
        sendto(sockfd, (const char *) req.message, req.size, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
    }

    //OACK
    char* oackBuff = (char*) malloc((4 + 512)*sizeof(char));
    memset(oackBuff, 0, 516);
    char* toclean[3];
    toclean[0] = oackBuff;
    toclean[1] = nullptr;
    toclean[2] = nullptr;

    unsigned int adrlen = 0;
    int rec = 0;

    if (ipv4){
        adrlen = sizeof(servaddrivp4);
        rec = recvfrom(sockfd, oackBuff, 4+512 , 0, (struct sockaddr *)&servaddrivp4, &adrlen);
    }else{
        adrlen = sizeof(servaddrivp6);
        rec = recvfrom(sockfd, oackBuff, 4+512 , 0, (struct sockaddr *)&servaddrivp6, &adrlen);
    }

    if (rec == -1){
        printTime();
        std::cerr << "ABORTING TRANSFER: no reply recieved from " << address << ":" << port << "\n";
        cleanup(sockfd, nullptr, toclean);
        return 1;
    }

    OACK oackVals;
    //parsing server reply
    if (oackBuff[1] == 6){

        oackVals.parse(oackBuff, rec);
        if (timeout_i != -1){
            if (oackVals.timeout == ""){
                std::cout << "NOTICE: server refused timeout value, using default value\n";
            }else if (oackVals.timeout == timeout_s){
                try{
                    timeout.tv_sec = std::stoi(oackVals.timeout);
                }catch (...){
                    std::cerr << "ABORTING TRANSFER: server responded with non numerical timeout value\n";
                    cleanup(sockfd, nullptr, toclean);
                    return 1;
                }
                if (
                    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0
                    ||
                    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0
                ){
                    printTime();
                    std::cerr << "ERROR: Socket options setting failed\n";
                    cleanup(sockfd, nullptr, toclean);
                    return 1;
                }
            }else{
                std::cerr << "ABORTING TRANSFER: server proposed different timeout\n";
                cleanup(sockfd, nullptr, toclean);
                return 1;
            }
        }
        if (blocksize_i != -1){
            if (oackVals.blksize == ""){
                std::cout << "NOTICE: server refused blksize value, using default TFTPv2 value (512)\n";
                blocksize_s = "512";
                blocksize_i = 512;
            }else{
                try{
                    blocksize_i = std::stoll(oackVals.blksize);
                    if(oackVals.blksize != blocksize_s){
                        std::cout << "NOTICE: using blksize value proposed by server (" << blocksize_i << ")\n";
                    }
                    blocksize_s = oackVals.blksize;
                }catch (...){
                    std::cerr << "ABORTING TRANSFER: responded with non numerical blksize value\n";
                    cleanup(sockfd, nullptr, toclean);
                    return 1;
                }
            }
        }else{
            blocksize_i = 512;
            blocksize_s = "512";
        }
        if (oackVals.tsize != ""){
            if (oackVals.tsize != std::to_string(std::filesystem::file_size(path))){
                std::cerr << "ABORTING TRANSFER: server did not echo tsize correctly\n";
                cleanup(sockfd, nullptr, toclean);
                return 1;
            }
        }
    }else if(oackBuff[1] == 4){
        std::cout << "NOTICE: server does not support option negotiation, using default TFTPv2 values\n";
        blocksize_i = 512;
        blocksize_s = "512";
    }else if (oackBuff[1] == 8){
        std::cerr << "ABORTING TRANSFER: (err code 8) - terminated by server due to option negotiation (" << &oackBuff[4] << ")\n";
        cleanup(sockfd, nullptr, toclean);
        return 1;
    }else if (oackBuff[1] == 5){
        std::cerr << "ABORTING TRANSFER: (err code 5) -" << &oackBuff[4] << "\n";
        cleanup(sockfd, nullptr, toclean);
        return 1;
    }

    int blockNum = 0;
    int num = 0;
    FILE* cfile = fopen(path.filename().c_str(), "r");

    char* dataBlock = (char*) malloc((4 + blocksize_i)*sizeof(char));
    memset(dataBlock, 0, blocksize_i + 4);
    toclean[1] = dataBlock;
    dataBlock[1] = 3;

    //ack
    char* ack = (char*) malloc(4 * sizeof (char));
    memset(ack, 0, 4);
    toclean[2] = ack;

    printTime();
    std::cout << "Starting transport...\n";

    //actual data transport
    do{
        dataBlock[2] = ++blockNum >> 8;
        dataBlock[3] = blockNum;
        num = readFile(&dataBlock[4], blocksize_i, cfile);
        if (ipv4){
            sendto(sockfd, (const char *) dataBlock, num + 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
            rec = recvfrom(sockfd, ack, 4, 0, (struct sockaddr *)&servaddrivp4, &adrlen);
        }else{
            sendto(sockfd, (const char *) dataBlock, num + 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
            rec = recvfrom(sockfd, ack, 4, 0, (struct sockaddr *)&servaddrivp6, &adrlen);
        }
        //restransmission in case of none/incorrect ack
        if (rec == -1 || ack[2] != dataBlock[2] || ack[3] != dataBlock[3]){
            std::cout << "NOTICE: no ack for block " << (int) dataBlock[3] << " recieved, retransmitting...\n";
            if (ipv4){
                sendto(sockfd, (const char *) dataBlock, num + 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp4, sizeof(servaddrivp4));
                rec = recvfrom(sockfd, ack, 4, 0, (struct sockaddr *)&servaddrivp4, &adrlen);
            }else{
                sendto(sockfd, (const char *) dataBlock, num + 4, MSG_CONFIRM, (const struct sockaddr *) &servaddrivp6, sizeof(servaddrivp6));
                rec = recvfrom(sockfd, ack, 4, 0, (struct sockaddr *)&servaddrivp6, &adrlen);
            }
            if (rec == -1 || ack[2] != blockNum >> 8 || ack[3] != blockNum){
                std::cerr << "ABORTING TRANSFER: connection interrupted\n";
                cleanup(sockfd, cfile, toclean);
                return 1;
            }
        }
        memset(&dataBlock[4], 0, blocksize_i);
        
    }while (num == blocksize_i);

    printTime();
    std::cout << "Transport finished sucessfully\n";

    cleanup(sockfd, cfile, toclean);

    return 0;
}


int main(){

    while(!feof(stdin)){
        paramSet();
        std::cout << ">";
        std::string input;
        getline(std::cin, input);
        if(paramCheck(input)){
            if (readON){
                readRequest();
            }else if(!readON){
                writeRequest();
            }
        }
    }
    
    return 0;
}