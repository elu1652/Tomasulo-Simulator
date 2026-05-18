#include "Memory.h"

Memory::Memory(int size){
    mem.resize(size,0);
}

int Memory::load(int address) const{
    return mem[address];
}

void Memory::store(int address, int value){
    mem[address] = value;
}

std::vector<int> Memory::snapshot(int count) const{
    std::vector<int> values;

    if(count <= 0){
        return values;
    }

    values.reserve(count);

    for(int i = 0; i < count; ++i){
        if(i < static_cast<int>(mem.size())){
            values.push_back(mem[i]);
        } else {
            values.push_back(0);
        }
    }

    return values;
}

void Memory::print(int start, int count) const{
    for(int i = start; i < start + count && i < mem.size(); ++i){
        std::cout << "Mem[" << i << "]: " << mem[i] << "\n";
    }
}
