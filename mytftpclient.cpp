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

    if (arguments[0] != '-' || arguments[2] != ' ' || arguments[3] != '-' || arguments[4] != 'd' || arguments[5] != ' '){
        fprintf(stderr, "Missing or invalid arguments\n");
        exit(1);
    }
    if (arguments[1] == 'W'){
        read = false;
    }else if(arguments[1] == 'R'){
        read = true;
    }else{
        fprintf(stderr, "Missing or invalid arguments\n");
        exit(1);
    }
    arguments = arguments.substr(6);
    path = arguments.substr(0, arguments.find(' '));
    arguments = arguments.substr(arguments.find(' ')+1);
    arguments = arguments.append(" ");

    while (arguments.length() != 0){
        std::string arg = arguments.substr(0, arguments.find(' '));
        arguments = arguments.substr(arguments.find(' ')+1);
        std::string value = arguments.substr(0, arguments.find(' '));
        arguments = arguments.substr(arguments.find(' ')+1);
        
        if (arg == "-t"){
            timeout = std::stoi(value);
            std::cout << "-t value is:" << timeout << ":\n";
        }else if (arg == "-s"){
            size = std::stoi(value);
            std::cout << "-s value is:" << size << ":\n";
        }else if (arg == "-m"){
            multicast = true;
            value = value.append(" ");
            arguments = value.append(arguments);
            std::cout << "-m value is:" << multicast << ":\n";
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