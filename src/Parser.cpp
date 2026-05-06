#include "Parser.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

// Parse register to number
// Example: R3 -> 3
static int parseRegister(const std::string& token) {
    return std::stoi(token.substr(1));
}

static OpCode parseOpCode(const std::string& token) {
    if (token == "ADD") return OpCode::ADD;
    if (token == "SUB") return OpCode::SUB;
    if (token == "MUL") return OpCode::MUL;
    if (token == "LD")  return OpCode::LD;
    if (token == "SD")  return OpCode::SD;
    if (token == "BNE") return OpCode::BNE;

    return OpCode::INVALID;
}

std::vector<Instruction> Parser::parseFile(const std::string& filename) {
    std::vector<Instruction> instructions;

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: could not open file: " << filename << std::endl;
        return instructions;
    }

    std::string line;

    while(std::getline(file,line)){
        if(line.empty()) continue; 

        Instruction instr;
        instr.rawText = line;

        //Remove commas
        line.erase(std::remove(line.begin(), line.end(), ','), line.end());

        std::stringstream ss(line);

        std::string opcodeToken;
        ss >> opcodeToken;

        instr.opcode = parseOpCode(opcodeToken);

        if (instr.opcode == OpCode::ADD ||
        instr.opcode == OpCode::SUB ||
        instr.opcode == OpCode::MUL) {
            std::string rdToken, rs1Token, rs2Token;
            ss >> rdToken >> rs1Token >> rs2Token;

            instr.rd = parseRegister(rdToken);
            instr.rs1 = parseRegister(rs1Token);
            instr.rs2 = parseRegister(rs2Token);
        }
        else if(instr.opcode == OpCode::LD) {
            std::string rdToken, addressToken;

            ss >> rdToken >> addressToken;

            instr.rd = parseRegister(rdToken);

            size_t openParen = addressToken.find('(');
            size_t closeParen = addressToken.find(')');

            std::string immString = addressToken.substr(0, openParen);
            std::string regString = addressToken.substr(openParen + 1, closeParen - openParen - 1);

            instr.immediate = std::stoi(immString);
            instr.rs1 = parseRegister(regString);
        }
        else if(instr.opcode == OpCode::SD) {
            std::string rs2Token, addressToken;

            ss >> rs2Token >> addressToken;

            instr.rs2 = parseRegister(rs2Token);

            size_t openParen = addressToken.find('(');
            size_t closeParen = addressToken.find(')');

            std::string immString = addressToken.substr(0, openParen);
            std::string regString = addressToken.substr(openParen + 1, closeParen - openParen - 1);

            instr.immediate = std::stoi(immString);
            instr.rs1 = parseRegister(regString);
        }
        instructions.push_back(instr);
    }

    return instructions;
}