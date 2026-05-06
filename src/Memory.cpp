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

void Memory::print(int start, int count) const{
    for(int i = start; i < start + count && i < mem.size(); ++i){
        std::cout << "Mem[" << i << "]: " << mem[i] << "\n";
    }
}