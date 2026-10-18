#include "parse.h"

namespace nano_edr {

bool IsBlankOrComment(const std::string& line) {
    std::size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;

    if (i >= line.size()) return true;

    return line[i] == '#' || line[i] == ';';
}

bool isSpace(char symb) {
    return symb == ' ' || symb == '\t';
}

bool ParseEventParts(const std::string& line, EventParts* out) {
    if (IsBlankOrComment(line)) return false;

    const std::string& s = line;
    std::size_t i = 0;
    bool has_ts = false;
    bool has_type = false;
    bool has_pid = false;

    while (i < s.size()) {
        while (i < s.size() && isSpace(s[i])) ++i;
        if (i >= s.size()) break;

        std::size_t key_start = i;
        while (i < s.size() && s[i] != '=' && !isSpace(s[i])) ++i;

        if (i >= s.size() || s[i] != '=') return false;

        std::string key = s.substr(key_start, i - key_start);
        if (key.empty()) return false;
        ++i;

        std::string value;
        if (i < s.size() && s[i] == '"') {
            ++i;
            std::size_t val_start = i;

            while (i < s.size() && s[i] != '"') ++i;

            if (i >= s.size()) return false;

            value = s.substr(val_start, i - val_start);
            ++i;

            if (i < s.size() && !isSpace(s[i])) return false;
        } else {
            std::size_t val_start = i;

            while (i < s.size() && !isSpace(s[i])) ++i;
            value = s.substr(val_start, i - val_start);
        }

        if (key == "ts" && !has_ts) {
            out->ts = value;
            has_ts = true;
        } else if (key == "type" && !has_type) {
            out->type = value;
            has_type = true;
        } else if (key == "pid" && !has_pid) {
            out->pid = value;
            has_pid = true;
        } else {
            Field f;
            f.key = key;
            f.value = value;
            out->fields.push_back(f);
        }
    }

    return true;
}
}  // namespace nano_edr
