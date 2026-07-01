#ifndef FLA_PDA_H
#define FLA_PDA_H

#include "cfg.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace fla {

using State = std::string;
using StackSymbol = std::string;

struct TransitionKey {
    State state;
    Symbol input;
    StackSymbol stackTop;

    bool operator<(const TransitionKey& other) const;
};

struct TransitionValue {
    State nextState;
    std::vector<StackSymbol> pushSymbols;

    bool operator<(const TransitionValue& other) const;
};

struct PDA {
    std::set<State> states;
    std::set<Symbol> inputSymbols;
    std::set<StackSymbol> stackSymbols;
    std::map<TransitionKey, std::set<TransitionValue>> delta;
    State startState;
    StackSymbol startStack;
    std::set<State> finalStates;
};

PDA parsePDA(const std::vector<std::string>& lines);
void validatePDA(const PDA& pda);
CFG convertPDAToCFG(const PDA& pda);

}

#endif
