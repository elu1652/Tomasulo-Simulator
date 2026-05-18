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

std::vector<int> RegisterFile::snapshot(int count) const{
    std::vector<int> values;

    if(count <= 0){
        return values;
    }

    values.reserve(count);

    for(int i = 0; i < count; ++i){
        if(i < static_cast<int>(registers.size())){
            values.push_back(read(i));
        } else {
            values.push_back(0);
        }
    }

    return values;
}

void RegisterFile::print() const{
    for(int i = 0; i < registers.size(); ++i){
        std::cout << "R" << i << ": " << read(i) << std::endl;
    }
}
