#include "RegisterFile.h"

RegisterFile::RegisterFile(){
    registers.resize(32,0);
}   

int RegisterFile::read(int index) const{
    if(index == 0){
        return 0;
    }
    return registers[index];
}

void RegisterFile::write(int index, int value){
    if(index != 0){
        registers[index] = value;
    }
}

void RegisterFile::print() const{
    for(int i = 0; i < registers.size(); ++i){
        std::cout << "R" << i << ": " << read(i) << std::endl;
    }
}