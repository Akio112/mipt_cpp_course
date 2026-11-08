#include "field_traits.h"

#include <charconv>

#include "text.h"

namespace nano_edr {

bool FieldTraits<std::string>::Parse(const std::string& text, std::string* out) {
    *out = text;
    return true;
}

bool FieldTraits<uint64_t>::Parse(const std::string& text, uint64_t* out) {
    uint64_t number;
    const char* first = text.data();
    const char* last = first + text.size();
    const auto [end, error] = std::from_chars(first, last, number);
    if (error != std::errc() || end != last) return false;

    *out = number;
    return true;
}

bool FieldTraits<bool>::Parse(const std::string& text, bool* out) {
    for (const auto word : {"1", "true", "yes"}) {
        if (TextEquals(text, word)) {
            *out = true;
            return true;
        }
    }
    for (const auto word : {"0", "false", "no"}) {
        if (TextEquals(text, word)) {
            *out = false;
            return true;
        }
    }
    return false;
}

}  // namespace nano_edr
