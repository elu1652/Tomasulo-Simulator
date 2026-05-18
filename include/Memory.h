#pragma once

#include <vector>
#include <iostream>

class Memory{
    private:
        std::vector<int> mem;

    public:
        Memory(int size = 1024);

        int load(int address) const;
        void store(int address, int value);
        std::vector<int> snapshot(int count = 32) const;
        void print(int start = 0, int count = 16) const;
};
