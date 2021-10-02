#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>


bool read;
std::string path;
int timeout; //set to zero
int size; //zistit na co nastavit
bool multicast; //set to false
std::string mode; //set to "binary"
std::string adress;//set to 127.0.0.1
int port;//set to 69



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
            read = false;
            value = value.append(" ");
            arguments = value.append(arguments);
        }else if (arg == "-R"){
            read = true;
            value = value.append(" ");
            arguments = value.append(arguments);
        }else if (arg == "-m"){
            multicast = true;
            value = value.append(" ");
            arguments = value.append(arguments);
            std::cout << "-m value is:" << multicast << ":\n";
        }else if (arg == "-d"){
            path = value;
            std::cout << "-d value is:" << path << ":\n";
        }else if (arg == "-t"){
            timeout = std::stoi(value);
            std::cout << "-t value is:" << timeout << ":\n";
        }else if (arg == "-s"){
            size = std::stoi(value);
            std::cout << "-s value is:" << size << ":\n";
        }else if (arg == "-c"){
            mode = value;
            std::cout << "-c value is:" << mode << ":\n";
        }else if (arg == "-a"){
            adress = value.substr(0, value.find(','));
            port = std::stoi(value.substr(value.find(',')+1));
            std::cout << "-a value is:" << adress << "," << port << ":\n";
        }
        std::cout << "The rest is:" << arguments << ":\n\n";
    }

    return 1;
}



int main(){

    std::cout << ">";
    std::string input;
    getline(std::cin, input);
    //std::cout << input;

    paramCheck(input);

    return 0;
}