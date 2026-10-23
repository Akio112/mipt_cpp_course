#include "text.h"

#include <cctype>

namespace nano_edr {
namespace {

bool EqualChar(char a, char b) {
    return std::tolower(static_cast<unsigned char>(a)) ==
           std::tolower(static_cast<unsigned char>(b));
}

}  // namespace

bool StartsAt(const std::string& text, std::size_t pos,
              const std::string& word) {
    if (pos + word.size() > text.size()) return false;
    for (std::size_t i = 0; i < word.size(); ++i) {
        if (!EqualChar(text[pos + i], word[i])) return false;
    }
    return true;
}

bool TextEquals(const std::string& text, const std::string& value) {
    return text.size() == value.size() && StartsAt(text, 0, value);
}

bool TextContains(const std::string& text, const std::string& part) {
    if (part.empty()) return true;
    if (part.size() > text.size()) return false;
    for (std::size_t i = 0; i + part.size() <= text.size(); ++i) {
        if (StartsAt(text, i, part)) return true;
    }
    return false;
}

bool TextEndsWith(const std::string& text, const std::string& suffix) {
    if (suffix.size() > text.size()) return false;
    return StartsAt(text, text.size() - suffix.size(), suffix);
}

}  // namespace nano_edr
