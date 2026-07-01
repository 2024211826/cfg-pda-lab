#include "cfg.h"

#include "common.h"

#include <deque>
#include <iostream>
#include <stdexcept>

namespace fla {

namespace {

std::vector<Symbol> orderedSymbols(const std::set<Symbol>& symbols) {
    return std::vector<Symbol>(symbols.begin(), symbols.end());
}

std::string setText(const std::set<Symbol>& symbols) {
    return "{" + join(orderedSymbols(symbols), ", ") + "}";
}

std::set<Symbol> allGrammarSymbols(const CFG& cfg) {
    std::set<Symbol> symbols = cfg.nonterminals;
    symbols.insert(cfg.terminals.begin(), cfg.terminals.end());
    return symbols;
}

std::vector<Symbol> compactSymbols(const std::string& text, const std::set<Symbol>& known) {
    std::vector<Symbol> symbols;
    for (char ch : text) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            symbols.push_back(std::string(1, ch));
        }
    }

    if (known.empty()) {
        return symbols;
    }

    std::vector<Symbol> sorted(known.begin(), known.end());
    std::sort(sorted.begin(), sorted.end(), [](const Symbol& a, const Symbol& b) {
        if (a.size() != b.size()) {
            return a.size() > b.size();
        }
        return a < b;
    });

    std::vector<Symbol> result;
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

RHS parseRHS(const std::string& text, const CFG& cfg) {
    std::string value = trim(text);
    if (value.empty() || isEpsilonToken(value)) {
        return {};
    }

    if (hasWhitespace(value)) {
        std::vector<Symbol> tokens = splitWhitespace(value);
        if (tokens.size() == 1 && isEpsilonToken(tokens[0])) {
            return {};
        }
        for (const auto& token : tokens) {
            if (isEpsilonToken(token)) {
                throw std::runtime_error("epsilon cannot appear with other symbols: " + text);
            }
        }
        return tokens;
    }

    return compactSymbols(value, allGrammarSymbols(cfg));
}

void addProductionLine(CFG& cfg, const std::string& line) {
    std::string value = trim(line);
    if (value.empty() || value == "END" || value == "@") {
        return;
    }

    std::string left;
    std::string right;
    size_t arrow = value.find("->");
    if (arrow != std::string::npos) {
        left = trim(value.substr(0, arrow));
        right = trim(value.substr(arrow + 2));
    } else {
        auto tokens = splitWhitespace(value);
        if (tokens.size() < 2) {
            throw std::runtime_error("bad production line: " + line);
        }
        left = tokens[0];
        right = trim(value.substr(value.find(tokens[1])));
    }

    if (left.empty() || right.empty()) {
        throw std::runtime_error("bad production line: " + line);
    }

    for (const auto& part : splitByChar(right, '|')) {
        cfg.productions[left].insert(parseRHS(part, cfg));
    }
}

std::set<Symbol> parseSymbolSet(const std::string& text) {
    std::set<Symbol> result;
    std::string value = trim(text);
    auto tokens = splitWhitespace(value);
    if (hasWhitespace(value) && !tokens.empty()) {
        result.insert(tokens.begin(), tokens.end());
        return result;
    }

    for (char ch : value) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            result.insert(std::string(1, ch));
        }
    }
    return result;
}

Symbol firstHeaderSymbol(const std::string& text) {
    std::string value = trim(text);
    if (hasWhitespace(value)) {
        auto tokens = splitWhitespace(value);
        if (!tokens.empty()) {
            return tokens[0];
        }
    }

    for (char ch : value) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            return std::string(1, ch);
        }
    }

    return "";
}

CFG parseNamedCFG(const std::vector<std::string>& lines) {
    CFG cfg;
    bool inProductions = false;

    for (const auto& raw : lines) {
        std::string line = trim(raw);
        if (line.empty()) {
            continue;
        }
        if (line == "END" || line == "@") {
            break;
        }

        if (inProductions) {
            addProductionLine(cfg, line);
            continue;
        }

        if (startsWith(line, "N:")) {
            cfg.nonterminals = parseSymbolSet(afterColon(line));
        } else if (startsWith(line, "T:")) {
            cfg.terminals = parseSymbolSet(afterColon(line));
        } else if (startsWith(line, "S:")) {
            auto tokens = splitWhitespace(afterColon(line));
            if (tokens.empty()) {
                throw std::runtime_error("missing start symbol");
            }
            cfg.start = tokens[0];
        } else if (startsWith(line, "P:")) {
            inProductions = true;
        } else {
            throw std::runtime_error("unexpected line before P: " + line);
        }
    }

    return cfg;
}

CFG parseCompactCFG(const std::vector<std::string>& lines) {
    std::vector<std::string> useful;
    for (const auto& raw : lines) {
        std::string line = trim(raw);
        if (!line.empty()) {
            useful.push_back(line);
        }
    }

    if (useful.size() < 3) {
        throw std::runtime_error("not enough lines for compact CFG input");
    }

    CFG cfg;
    cfg.nonterminals = parseSymbolSet(useful[0]);
    cfg.terminals = parseSymbolSet(useful[1]);
    cfg.start = firstHeaderSymbol(useful[0]);

    for (size_t i = 2; i < useful.size(); ++i) {
        if (useful[i] == "END" || useful[i] == "@") {
            break;
        }
        addProductionLine(cfg, useful[i]);
    }

    return cfg;
}

bool isUnitProduction(const CFG& cfg, const RHS& rhs) {
    return rhs.size() == 1 && cfg.nonterminals.count(rhs[0]);
}

Symbol uniqueSymbol(const CFG& cfg, const Symbol& base) {
    for (int i = 0;; ++i) {
        Symbol candidate = base + std::to_string(i);
        if (!cfg.nonterminals.count(candidate) && !cfg.terminals.count(candidate)) {
            return candidate;
        }
    }
}

void addNullableVariants(const RHS& rhs,
                         const std::set<Symbol>& nullable,
                         size_t index,
                         RHS& current,
                         std::set<RHS>& out) {
    if (index == rhs.size()) {
        if (!current.empty()) {
            out.insert(current);
        }
        return;
    }

    const Symbol& symbol = rhs[index];
    if (nullable.count(symbol)) {
        addNullableVariants(rhs, nullable, index + 1, current, out);
    }

    current.push_back(symbol);
    addNullableVariants(rhs, nullable, index + 1, current, out);
    current.pop_back();
}

bool rhsGenerating(const CFG& cfg, const RHS& rhs, const std::set<Symbol>& generating) {
    for (const auto& symbol : rhs) {
        if (cfg.terminals.count(symbol)) {
            continue;
        }
        if (!generating.count(symbol)) {
            return false;
        }
    }
    return true;
}

}

CFG parseCFG(const std::vector<std::string>& lines) {
    bool named = false;
    for (const auto& raw : lines) {
        std::string line = trim(raw);
        if (startsWith(line, "N:") || startsWith(line, "T:") || startsWith(line, "P:")) {
            named = true;
            break;
        }
    }

    CFG cfg = named ? parseNamedCFG(lines) : parseCompactCFG(lines);
    validateCFG(cfg);
    return cfg;
}

void validateCFG(const CFG& cfg) {
    if (cfg.start.empty()) {
        throw std::runtime_error("missing CFG start symbol");
    }
    for (const auto& symbol : cfg.nonterminals) {
        if (isEpsilonToken(symbol)) {
            throw std::runtime_error("epsilon token cannot be declared as a nonterminal: " + symbol);
        }
    }
    for (const auto& symbol : cfg.terminals) {
        if (isEpsilonToken(symbol)) {
            throw std::runtime_error("epsilon token cannot be declared as a terminal: " + symbol);
        }
    }
    if (!cfg.nonterminals.count(cfg.start)) {
        throw std::runtime_error("CFG start symbol is not in N: " + cfg.start);
    }

    for (const auto& [left, rights] : cfg.productions) {
        if (!cfg.nonterminals.count(left)) {
            throw std::runtime_error("production left side is not in N: " + left);
        }
        for (const auto& rhs : rights) {
            for (const auto& symbol : rhs) {
                if (!cfg.nonterminals.count(symbol) && !cfg.terminals.count(symbol)) {
                    throw std::runtime_error("undeclared grammar symbol: " + symbol);
                }
            }
        }
    }
}

CFG removeEpsilonProductions(const CFG& cfg) {
    std::set<Symbol> nullable;
    bool changed = true;

    while (changed) {
        changed = false;
        for (const auto& [left, rights] : cfg.productions) {
            if (nullable.count(left)) {
                continue;
            }
            for (const auto& rhs : rights) {
                bool allNullable = rhs.empty();
                if (!rhs.empty()) {
                    allNullable = true;
                    for (const auto& symbol : rhs) {
                        if (!nullable.count(symbol)) {
                            allNullable = false;
                            break;
                        }
                    }
                }
                if (allNullable) {
                    nullable.insert(left);
                    changed = true;
                    break;
                }
            }
        }
    }

    CFG result;
    result.nonterminals = cfg.nonterminals;
    result.terminals = cfg.terminals;
    result.start = cfg.start;

    bool startNullable = nullable.count(cfg.start) > 0;
    Symbol oldStart = cfg.start;
    if (startNullable) {
        result.start = uniqueSymbol(cfg, cfg.start);
        result.nonterminals.insert(result.start);
    }

    for (const auto& [left, rights] : cfg.productions) {
        for (const auto& rhs : rights) {
            if (rhs.empty()) {
                continue;
            }

            RHS current;
            std::set<RHS> variants;
            addNullableVariants(rhs, nullable, 0, current, variants);
            for (const auto& item : variants) {
                result.productions[left].insert(item);
            }
        }
    }

    if (startNullable) {
        result.productions[result.start].insert(RHS{oldStart});
        result.productions[result.start].insert(RHS{});
    }

    return result;
}

CFG removeUnitProductions(const CFG& cfg) {
    CFG result;
    result.nonterminals = cfg.nonterminals;
    result.terminals = cfg.terminals;
    result.start = cfg.start;

    for (const auto& start : cfg.nonterminals) {
        std::set<Symbol> closure;
        std::deque<Symbol> q;
        closure.insert(start);
        q.push_back(start);

        while (!q.empty()) {
            Symbol current = q.front();
            q.pop_front();

            auto it = cfg.productions.find(current);
            if (it == cfg.productions.end()) {
                continue;
            }

            for (const auto& rhs : it->second) {
                if (isUnitProduction(cfg, rhs) && !closure.count(rhs[0])) {
                    closure.insert(rhs[0]);
                    q.push_back(rhs[0]);
                }
            }
        }

        for (const auto& symbol : closure) {
            auto it = cfg.productions.find(symbol);
            if (it == cfg.productions.end()) {
                continue;
            }

            for (const auto& rhs : it->second) {
                if (!isUnitProduction(cfg, rhs)) {
                    result.productions[start].insert(rhs);
                }
            }
        }
    }

    return result;
}

CFG removeUselessSymbols(const CFG& cfg) {
    std::set<Symbol> generating;
    bool changed = true;

    while (changed) {
        changed = false;
        for (const auto& [left, rights] : cfg.productions) {
            if (generating.count(left)) {
                continue;
            }
            for (const auto& rhs : rights) {
                if (rhsGenerating(cfg, rhs, generating)) {
                    generating.insert(left);
                    changed = true;
                    break;
                }
            }
        }
    }

    CFG mid;
    mid.start = cfg.start;
    mid.terminals = cfg.terminals;

    if (!generating.count(cfg.start)) {
        mid.nonterminals.insert(cfg.start);
        return mid;
    }

    mid.nonterminals = generating;
    for (const auto& [left, rights] : cfg.productions) {
        if (!generating.count(left)) {
            continue;
        }
        for (const auto& rhs : rights) {
            if (rhsGenerating(cfg, rhs, generating)) {
                mid.productions[left].insert(rhs);
            }
        }
    }

    std::set<Symbol> reachableNonterminals;
    std::set<Symbol> reachableTerminals;
    std::deque<Symbol> q;
    reachableNonterminals.insert(mid.start);
    q.push_back(mid.start);

    while (!q.empty()) {
        Symbol current = q.front();
        q.pop_front();

        auto it = mid.productions.find(current);
        if (it == mid.productions.end()) {
            continue;
        }

        for (const auto& rhs : it->second) {
            for (const auto& symbol : rhs) {
                if (mid.terminals.count(symbol)) {
                    reachableTerminals.insert(symbol);
                } else if (mid.nonterminals.count(symbol) && !reachableNonterminals.count(symbol)) {
                    reachableNonterminals.insert(symbol);
                    q.push_back(symbol);
                }
            }
        }
    }

    CFG result;
    result.start = mid.start;
    result.nonterminals = reachableNonterminals;
    result.terminals = reachableTerminals;

    for (const auto& [left, rights] : mid.productions) {
        if (!reachableNonterminals.count(left)) {
            continue;
        }

        for (const auto& rhs : rights) {
            bool ok = true;
            for (const auto& symbol : rhs) {
                if (!reachableTerminals.count(symbol) && !reachableNonterminals.count(symbol)) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                result.productions[left].insert(rhs);
            }
        }
    }

    return result;
}

CFG simplifyCFG(const CFG& cfg) {
    return removeUselessSymbols(removeUnitProductions(removeEpsilonProductions(cfg)));
}

std::string formatRHS(const RHS& rhs) {
    if (rhs.empty()) {
        return "#";
    }
    return join(rhs, " ");
}

void printCFG(std::ostream& out, const CFG& cfg) {
    out << "N = " << setText(cfg.nonterminals) << '\n';
    out << "T = " << setText(cfg.terminals) << '\n';
    out << "S = " << cfg.start << '\n';
    out << "P:\n";

    bool printed = false;
    std::vector<Symbol> order;
    if (!cfg.start.empty() && cfg.nonterminals.count(cfg.start)) {
        order.push_back(cfg.start);
    }
    for (const auto& symbol : cfg.nonterminals) {
        if (symbol != cfg.start) {
            order.push_back(symbol);
        }
    }

    for (const auto& left : order) {
        auto it = cfg.productions.find(left);
        if (it == cfg.productions.end() || it->second.empty()) {
            continue;
        }

        out << "  " << left << " -> ";
        bool first = true;
        for (const auto& rhs : it->second) {
            if (!first) {
                out << " | ";
            }
            out << formatRHS(rhs);
            first = false;
        }
        out << '\n';
        printed = true;
    }

    if (!printed) {
        out << "  (none)\n";
    }
}

}
