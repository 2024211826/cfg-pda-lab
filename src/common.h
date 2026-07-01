#ifndef FLA_COMMON_H
#define FLA_COMMON_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fla {

inline std::string trim(const std::string& s) {
    size_t left = 0;
    while (left < s.size() && std::isspace(static_cast<unsigned char>(s[left]))) {
        ++left;
    }
    if (left + 3 <= s.size() &&
        static_cast<unsigned char>(s[left]) == 0xEF &&
        static_cast<unsigned char>(s[left + 1]) == 0xBB &&
        static_cast<unsigned char>(s[left + 2]) == 0xBF) {
        left += 3;
    }

    size_t right = s.size();
    while (right > left && std::isspace(static_cast<unsigned char>(s[right - 1]))) {
        --right;
    }

    return s.substr(left, right - left);
}

inline bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

inline std::vector<std::string> splitWhitespace(const std::string& s) {
    std::istringstream in(s);
    std::vector<std::string> tokens;
    std::string token;
    while (in >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

inline std::vector<std::string> splitByChar(const std::string& s, char sep) {
    std::vector<std::string> parts;
    std::string current;
    std::istringstream in(s);
    while (std::getline(in, current, sep)) {
        parts.push_back(trim(current));
    }
    return parts;
}

inline std::string afterColon(const std::string& s) {
    size_t pos = s.find(':');
    if (pos == std::string::npos) {
        throw std::runtime_error("missing ':' in line: " + s);
    }
    return trim(s.substr(pos + 1));
}

inline bool isEpsilonToken(const std::string& token) {
    return token == "#" || token == "epsilon" || token == "eps";
}

inline bool hasWhitespace(const std::string& s) {
    return std::any_of(s.begin(), s.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
}

inline std::string join(const std::vector<std::string>& items, const std::string& sep) {
    std::string result;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) {
            result += sep;
        }
        result += items[i];
    }
    return result;
}

}

#endif
