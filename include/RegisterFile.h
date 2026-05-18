#pragma once

#include <vector>
#include <iostream>

class RegisterFile{
    private:
        std::vector<int> registers;

    public:
        RegisterFile();
        
        int read(int index) const;
        void write(int index, int value);
        std::vector<int> snapshot(int count = 32) const;
        void print() const;
};
