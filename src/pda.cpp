#include "pda.h"

#include "common.h"

#include <functional>
#include <stdexcept>
#include <tuple>

namespace fla {

bool TransitionKey::operator<(const TransitionKey& other) const {
    return std::tie(state, input, stackTop) < std::tie(other.state, other.input, other.stackTop);
}

bool TransitionValue::operator<(const TransitionValue& other) const {
    return std::tie(nextState, pushSymbols) < std::tie(other.nextState, other.pushSymbols);
}

namespace {

std::set<Symbol> parseSymbolSet(const std::string& text) {
    std::set<Symbol> result;
    auto tokens = splitWhitespace(text);
    result.insert(tokens.begin(), tokens.end());
    return result;
}

std::vector<StackSymbol> parseCompactStackSymbols(const std::string& text,
                                                  const std::set<StackSymbol>& known) {
    std::vector<StackSymbol> sorted(known.begin(), known.end());
    std::sort(sorted.begin(), sorted.end(), [](const StackSymbol& a, const StackSymbol& b) {
        if (a.size() != b.size()) {
            return a.size() > b.size();
        }
        return a < b;
    });

    std::vector<StackSymbol> result;
    size_t i = 0;
    while (i < text.size()) {
        if (std::isspace(static_cast<unsigned char>(text[i]))) {
            ++i;
            continue;
        }

        bool matched = false;
        for (const auto& symbol : sorted) {
            if (!symbol.empty() && text.compare(i, symbol.size(), symbol) == 0) {
                result.push_back(symbol);
                i += symbol.size();
                matched = true;
                break;
            }
        }

        if (!matched) {
            result.push_back(std::string(1, text[i]));
            ++i;
        }
    }

    return result;
}

std::vector<StackSymbol> parsePushSymbols(const std::string& text, const PDA& pda) {
    std::string value = trim(text);
    if (value.empty() || isEpsilonToken(value)) {
        return {};
    }

    if (hasWhitespace(value)) {
        auto tokens = splitWhitespace(value);
        if (tokens.size() == 1 && isEpsilonToken(tokens[0])) {
            return {};
        }
        for (const auto& token : tokens) {
            if (isEpsilonToken(token)) {
                throw std::runtime_error("epsilon cannot appear with other stack symbols: " + text);
            }
        }
        return tokens;
    }

    return parseCompactStackSymbols(value, pda.stackSymbols);
}

void addTransition(PDA& pda, const TransitionKey& key, const TransitionValue& value) {
    pda.delta[key].insert(value);
}

bool parseDeltaCall(PDA& pda, const std::string& line) {
    if (!startsWith(line, "delta")) {
        return false;
    }

    size_t leftOpen = line.find('(');
    size_t leftClose = line.find(')', leftOpen);
    size_t rightOpen = line.find('(', leftClose);
    size_t rightClose = line.find(')', rightOpen);
    if (leftOpen == std::string::npos || leftClose == std::string::npos ||
        rightOpen == std::string::npos || rightClose == std::string::npos) {
        throw std::runtime_error("bad delta line: " + line);
    }

    auto leftParts = splitByChar(line.substr(leftOpen + 1, leftClose - leftOpen - 1), ',');
    auto rightParts = splitByChar(line.substr(rightOpen + 1, rightClose - rightOpen - 1), ',');
    if (leftParts.size() != 3 || rightParts.size() != 2) {
        throw std::runtime_error("bad delta line: " + line);
    }

    TransitionKey key;
    key.state = leftParts[0];
    key.input = isEpsilonToken(leftParts[1]) ? "" : leftParts[1];
    key.stackTop = leftParts[2];

    TransitionValue value;
    value.nextState = rightParts[0];
    value.pushSymbols = parsePushSymbols(rightParts[1], pda);
    addTransition(pda, key, value);
    return true;
}

void parseArrowTransition(PDA& pda, const std::string& line) {
    size_t arrow = line.find("->");
    if (arrow == std::string::npos) {
        if (parseDeltaCall(pda, line)) {
            return;
        }
        throw std::runtime_error("missing -> in transition: " + line);
    }

    std::string left = trim(line.substr(0, arrow));
    std::string right = trim(line.substr(arrow + 2));

    auto leftTokens = splitWhitespace(left);
    auto rightTokens = splitWhitespace(right);
    if (leftTokens.size() != 3 || rightTokens.size() < 2) {
        throw std::runtime_error("bad transition line: " + line);
    }

    TransitionKey key;
    key.state = leftTokens[0];
    key.input = isEpsilonToken(leftTokens[1]) ? "" : leftTokens[1];
    key.stackTop = leftTokens[2];

    TransitionValue value;
    value.nextState = rightTokens[0];

    std::string pushText;
    if (rightTokens.size() > 1) {
        std::vector<std::string> pushTokens(rightTokens.begin() + 1, rightTokens.end());
        pushText = join(pushTokens, " ");
    }
    value.pushSymbols = parsePushSymbols(pushText, pda);

    addTransition(pda, key, value);
}

std::string variableName(const State& p, const StackSymbol& stack, const State& q) {
    return "[" + p + "," + stack + "," + q + "]";
}

Symbol uniqueStartForCFG(const CFG& cfg) {
    if (!cfg.nonterminals.count("S") && !cfg.terminals.count("S")) {
        return "S";
    }
    for (int i = 0;; ++i) {
        Symbol candidate = "S" + std::to_string(i);
        if (!cfg.nonterminals.count(candidate) && !cfg.terminals.count(candidate)) {
            return candidate;
        }
    }
}

void enumerateStates(const std::vector<State>& states,
                     size_t length,
                     std::vector<State>& current,
                     const std::function<void(const std::vector<State>&)>& visit) {
    if (current.size() == length) {
        visit(current);
        return;
    }

    for (const auto& state : states) {
        current.push_back(state);
        enumerateStates(states, length, current, visit);
        current.pop_back();
    }
}

}

PDA parsePDA(const std::vector<std::string>& lines) {
    PDA pda;
    bool inTransitions = false;

    for (const auto& raw : lines) {
        std::string line = trim(raw);
        if (line.empty()) {
            continue;
        }
        if (line == "END" || line == "@") {
            break;
        }

        if (inTransitions) {
            parseArrowTransition(pda, line);
            continue;
        }

        if (startsWith(line, "Q:")) {
            pda.states = parseSymbolSet(afterColon(line));
        } else if (startsWith(line, "T:")) {
            pda.inputSymbols = parseSymbolSet(afterColon(line));
        } else if (startsWith(line, "G:")) {
            pda.stackSymbols = parseSymbolSet(afterColon(line));
        } else if (startsWith(line, "q0:")) {
            auto tokens = splitWhitespace(afterColon(line));
            if (tokens.empty()) {
                throw std::runtime_error("missing q0");
            }
            pda.startState = tokens[0];
        } else if (startsWith(line, "z0:")) {
            auto tokens = splitWhitespace(afterColon(line));
            if (tokens.empty()) {
                throw std::runtime_error("missing z0");
            }
            pda.startStack = tokens[0];
        } else if (startsWith(line, "F:")) {
            pda.finalStates = parseSymbolSet(afterColon(line));
        } else if (startsWith(line, "D:")) {
            inTransitions = true;
        } else if (parseDeltaCall(pda, line)) {
            continue;
        } else {
            throw std::runtime_error("unexpected PDA line: " + line);
        }
    }

    validatePDA(pda);
    return pda;
}

void validatePDA(const PDA& pda) {
    if (pda.states.empty()) {
        throw std::runtime_error("missing PDA states");
    }
    for (const auto& state : pda.states) {
        if (isEpsilonToken(state)) {
            throw std::runtime_error("epsilon token cannot be declared as a state: " + state);
        }
    }
    for (const auto& symbol : pda.inputSymbols) {
        if (isEpsilonToken(symbol)) {
            throw std::runtime_error("epsilon token cannot be declared as an input symbol: " + symbol);
        }
    }
    for (const auto& symbol : pda.stackSymbols) {
        if (isEpsilonToken(symbol)) {
            throw std::runtime_error("epsilon token cannot be declared as a stack symbol: " + symbol);
        }
    }
    if (!pda.states.count(pda.startState)) {
        throw std::runtime_error("PDA start state is not in Q: " + pda.startState);
    }
    if (!pda.stackSymbols.count(pda.startStack)) {
        throw std::runtime_error("PDA start stack symbol is not in G: " + pda.startStack);
    }

    for (const auto& state : pda.finalStates) {
        if (!pda.states.count(state)) {
            throw std::runtime_error("PDA final state is not in Q: " + state);
        }
    }

    for (const auto& [key, values] : pda.delta) {
        if (!pda.states.count(key.state)) {
            throw std::runtime_error("transition state is not in Q: " + key.state);
        }
        if (!key.input.empty() && !pda.inputSymbols.count(key.input)) {
            throw std::runtime_error("transition input symbol is not in T: " + key.input);
        }
        if (!pda.stackSymbols.count(key.stackTop)) {
            throw std::runtime_error("transition stack symbol is not in G: " + key.stackTop);
        }

        for (const auto& value : values) {
            if (!pda.states.count(value.nextState)) {
                throw std::runtime_error("next state is not in Q: " + value.nextState);
            }
            for (const auto& symbol : value.pushSymbols) {
                if (!pda.stackSymbols.count(symbol)) {
                    throw std::runtime_error("push stack symbol is not in G: " + symbol);
                }
            }
        }
    }
}

CFG convertPDAToCFG(const PDA& pda) {
    CFG cfg;
    cfg.terminals = pda.inputSymbols;

    for (const auto& p : pda.states) {
        for (const auto& stack : pda.stackSymbols) {
            for (const auto& q : pda.states) {
                cfg.nonterminals.insert(variableName(p, stack, q));
            }
        }
    }

    cfg.start = uniqueStartForCFG(cfg);
    cfg.nonterminals.insert(cfg.start);

    for (const auto& q : pda.states) {
        cfg.productions[cfg.start].insert(RHS{variableName(pda.startState, pda.startStack, q)});
    }

    std::vector<State> states(pda.states.begin(), pda.states.end());

    for (const auto& [key, values] : pda.delta) {
        for (const auto& value : values) {
            if (value.pushSymbols.empty()) {
                RHS rhs;
                if (!key.input.empty()) {
                    rhs.push_back(key.input);
                }
                cfg.productions[variableName(key.state, key.stackTop, value.nextState)].insert(rhs);
                continue;
            }

            std::vector<State> current;
            enumerateStates(states, value.pushSymbols.size(), current, [&](const std::vector<State>& seq) {
                RHS rhs;
                if (!key.input.empty()) {
                    rhs.push_back(key.input);
                }

                State from = value.nextState;
                for (size_t i = 0; i < value.pushSymbols.size(); ++i) {
                    rhs.push_back(variableName(from, value.pushSymbols[i], seq[i]));
                    from = seq[i];
                }

                cfg.productions[variableName(key.state, key.stackTop, seq.back())].insert(rhs);
            });
        }
    }

    validateCFG(cfg);
    return cfg;
}

}
