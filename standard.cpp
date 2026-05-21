#include <iostream>
#include <string>
extern "C"{
    std::string input(){
        std::string out;
        std::cin >> out;
        return out;
    }
}