#include "cfg.h"
#include "common.h"
#include "pda.h"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

using namespace fla;

namespace {

std::vector<std::string> readBlock() {
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(std::cin, line)) {
        lines.push_back(line);
        std::string value = trim(line);
        if (value == "END" || value == "@") {
            break;
        }
    }
    return lines;
}

bool isCFGMode(const std::string& mode) {
    return mode == "1" || mode == "cfg" || mode == "CFG";
}

bool isPDAMode(const std::string& mode) {
    return mode == "2" || mode == "pda" || mode == "PDA";
}

}

int main(int argc, char* argv[]) {
    try {
        std::ios::sync_with_stdio(false);
        std::cin.tie(nullptr);

        std::string mode;
        if (argc >= 2) {
            mode = argv[1];
        } else {
            std::cout << "1. Simplify CFG\n";
            std::cout << "2. Convert PDA to CFG and simplify\n";
            std::cout << "Mode: ";
            std::getline(std::cin, mode);
            mode = trim(mode);
        }

        std::vector<std::string> lines = readBlock();
        if (lines.empty()) {
            throw std::runtime_error("empty input");
        }

        if (isCFGMode(mode)) {
            CFG cfg = parseCFG(lines);
            CFG simplified = simplifyCFG(cfg);
            std::cout << "Simplified CFG\n";
            printCFG(std::cout, simplified);
        } else if (isPDAMode(mode)) {
            PDA pda = parsePDA(lines);
            CFG generated = convertPDAToCFG(pda);
            CFG simplified = simplifyCFG(generated);

            std::cout << "Equivalent CFG generated from PDA\n";
            printCFG(std::cout, generated);
            std::cout << "\nSimplified CFG\n";
            printCFG(std::cout, simplified);
        } else {
            throw std::runtime_error("unknown mode: " + mode);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
