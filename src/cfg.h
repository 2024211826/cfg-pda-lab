#ifndef FLA_CFG_H
#define FLA_CFG_H

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fla {

using Symbol = std::string;
using RHS = std::vector<Symbol>;

struct CFG {
    std::set<Symbol> nonterminals;
    std::set<Symbol> terminals;
    Symbol start;
    std::map<Symbol, std::set<RHS>> productions;
};

CFG parseCFG(const std::vector<std::string>& lines);
void validateCFG(const CFG& cfg);

CFG removeEpsilonProductions(const CFG& cfg);
CFG removeUnitProductions(const CFG& cfg);
CFG removeUselessSymbols(const CFG& cfg);
CFG simplifyCFG(const CFG& cfg);

void printCFG(std::ostream& out, const CFG& cfg);
std::string formatRHS(const RHS& rhs);

}

#endif
