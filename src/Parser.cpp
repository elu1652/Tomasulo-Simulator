#include "Parser.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <unordered_map>

// Trim whitespace from the beginning and end of a string
static std::string trim(const std::string& input) {
    auto first = input.find_first_not_of(" \t\r\n");

    if (first == std::string::npos) {
        return "";
    }

    auto last = input.find_last_not_of(" \t\r\n");

    return input.substr(first, last - first + 1);
}

// Parse register to number
// Example: R3 -> 3
static int parseRegister(const std::string& token) {
    return std::stoi(token.substr(1));
}

static bool parseSetupDirective(
    const std::string& line,
    ProgramSetup& setup,
    const std::string& filename,
    const std::string& displayLine
) {
    // .REG and .MEM are assembler/test setup directives, not executable
    // instructions. They are stored separately so static PCs and dynamic
    // instruction IDs are assigned only to real program instructions.
    std::string normalized = line;
    normalized.erase(
        std::remove(normalized.begin(), normalized.end(), ','),
        normalized.end()
    );

    std::stringstream ss(normalized);
    std::string directive;
    ss >> directive;

    if (directive == ".REG") {
        std::string regToken;
        std::string valueToken;
        ss >> regToken >> valueToken;

        if (regToken.empty() || valueToken.empty()) {
            std::cerr << "Warning: invalid .REG directive ignored in "
                      << filename << ": " << line << "\n";
            return true;
        }

        setup.registerInitializers.push_back({
            parseRegister(regToken),
            std::stoi(valueToken)
        });
        setup.directiveLines.push_back(displayLine);
        return true;
    }

    if (directive == ".MEM") {
        std::string addressToken;
        std::string valueToken;
        ss >> addressToken >> valueToken;

        if (addressToken.empty() || valueToken.empty()) {
            std::cerr << "Warning: invalid .MEM directive ignored in "
                      << filename << ": " << line << "\n";
            return true;
        }

        setup.memoryInitializers.push_back({
            std::stoi(addressToken),
            std::stoi(valueToken)
        });
        setup.directiveLines.push_back(displayLine);
        return true;
    }

    return false;
}

static OpCode parseOpCode(const std::string& token) {
    if (token == "ADD") return OpCode::ADD;
    if (token == "ADDI") return OpCode::ADDI;
    if (token == "SUB") return OpCode::SUB;
    if (token == "MUL") return OpCode::MUL;
    if (token == "LD")  return OpCode::LD;
    if (token == "SD")  return OpCode::SD;
    if (token == "BEQ") return OpCode::BEQ;
    if (token == "BNE") return OpCode::BNE;
    if (token == "FADD") return OpCode::FADD;
    if (token == "FSUB") return OpCode::FSUB;
    if (token == "FMUL") return OpCode::FMUL;
    if (token == "FDIV") return OpCode::FDIV;

    return OpCode::INVALID;
}

ParsedProgram Parser::parseProgram(const std::string& filename) {
    ParsedProgram program;
    std::unordered_map<std::string, int> labelToInstructionIndex;

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: could not open file: " << filename << std::endl;
        return program;
    }

    std::string line;

    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;
        std::string originalLine = line;

        // Remove comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line = trim(line);

        // Skip empty/comment-only lines
        if (line.empty()) {
            continue;
        }

        // Handle one or more labels before an instruction:
        // loop:
        // or
        // loop: ADD R1, R1, R5
        while (true) {
            size_t colonPos = line.find(':');

            if (colonPos == std::string::npos) {
                break;
            }

            std::string labelName = trim(line.substr(0, colonPos)); // Get label name

            if (labelName.empty()) {
                std::cerr << "Error: empty label in file: " << filename << "\n";
                break;
            }

            labelToInstructionIndex[labelName] =
                static_cast<int>(program.instructions.size()); // Map label to instruction index

            line = trim(line.substr(colonPos + 1));

            // Line was only a label, no instruction after it
            if (line.empty()) {
                break;
            }
        }

        if (line.empty()) {
            continue;
        }

        if (parseSetupDirective(
                line,
                program.setup,
                filename,
                trim(originalLine)
            )) {
            continue;
        }

        Instruction instr;
        instr.rawText = line;
        instr.sourceLine = lineNumber;

        // Remove commas
        line.erase(std::remove(line.begin(), line.end(), ','), line.end());

        std::stringstream ss(line);

        std::string opcodeToken;
        ss >> opcodeToken;

        instr.opcode = parseOpCode(opcodeToken);

        if (instr.opcode == OpCode::ADD ||
            instr.opcode == OpCode::SUB ||
            instr.opcode == OpCode::MUL ||
            instr.opcode == OpCode::FADD ||
            instr.opcode == OpCode::FSUB ||
            instr.opcode == OpCode::FMUL ||
            instr.opcode == OpCode::FDIV) {

            std::string rdToken, rs1Token, rs2Token;
            ss >> rdToken >> rs1Token >> rs2Token;

            instr.rd = parseRegister(rdToken);
            instr.rs1 = parseRegister(rs1Token);
            instr.rs2 = parseRegister(rs2Token);
        }
        else if (instr.opcode == OpCode::ADDI) {
            std::string rdToken, rs1Token, immToken;
            ss >> rdToken >> rs1Token >> immToken;

            instr.rd = parseRegister(rdToken);
            instr.rs1 = parseRegister(rs1Token);
            instr.immediate = std::stoi(immToken);
        }
        else if (instr.opcode == OpCode::LD) {
            std::string rdToken, addressToken;

            ss >> rdToken >> addressToken;

            instr.rd = parseRegister(rdToken);

            size_t openParen = addressToken.find('(');
            size_t closeParen = addressToken.find(')');

            std::string immString = addressToken.substr(0, openParen);
            std::string regString = addressToken.substr(
                openParen + 1,
                closeParen - openParen - 1
            );

            instr.immediate = std::stoi(immString);
            instr.rs1 = parseRegister(regString);
        }
        else if (instr.opcode == OpCode::SD) {
            std::string rs2Token, addressToken;

            ss >> rs2Token >> addressToken;

            instr.rs2 = parseRegister(rs2Token);

            size_t openParen = addressToken.find('(');
            size_t closeParen = addressToken.find(')');

            std::string immString = addressToken.substr(0, openParen);
            std::string regString = addressToken.substr(
                openParen + 1,
                closeParen - openParen - 1
            );

            instr.immediate = std::stoi(immString);
            instr.rs1 = parseRegister(regString);
        }
        else if (instr.opcode == OpCode::BEQ ||
                 instr.opcode == OpCode::BNE) {

            std::string rs1Token, rs2Token, labelToken;

            ss >> rs1Token >> rs2Token >> labelToken;

            instr.rs1 = parseRegister(rs1Token);
            instr.rs2 = parseRegister(rs2Token);
            instr.label = labelToken;
        }
        else {
            std::cerr << "Warning: invalid instruction ignored: "
                      << instr.rawText << "\n";
            continue;
        }

        program.instructions.push_back(instr);
    }

    // Resolve branch labels after all instructions are parsed
    // Handles branching to future labels
    for (Instruction& instr : program.instructions) {
        if (instr.opcode == OpCode::BEQ ||
            instr.opcode == OpCode::BNE) {

            if (labelToInstructionIndex.find(instr.label) == labelToInstructionIndex.end()) {
                std::cerr << "Error: undefined label: "
                          << instr.label
                          << " in instruction: "
                          << instr.rawText
                          << "\n";
                instr.branchTarget = -1;
            } else {
                instr.branchTarget = labelToInstructionIndex[instr.label];
            }
        }
    }

    return program;
}

std::vector<Instruction> Parser::parseFile(const std::string& filename) {
    return parseProgram(filename).instructions;
}
