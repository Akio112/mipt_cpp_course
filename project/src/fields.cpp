#include "fields.h"

#include <charconv>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace nano_edr {

namespace {
bool EqualsInsensitive(char a, char b) {
    return std::tolower(a) == std::tolower(b);
}

}  // namespace

const std::string* FindField(const Event& event, const std::string& key) {
    for (const auto& [field_key, field_value] : event.fields) {
        if (field_key == key) {
            return &field_value;
        }
    }
    return nullptr;
}

const std::string& GetRequiredField(const Event& event, const std::string& key) {
    if (key == "ts" && event.ts.size()) {
        return event.ts;
    } else if (key == "type" && event.type.size()) {
        return event.type;
    } else if (key == "pid" && event.pid.size()) {
        return event.pid;
    } else if (const std::string* field = FindField(event, key)) {
        return *field;
    }
    throw std::invalid_argument("У поля отсутствует значение " + key);
}

bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    const std::string* finded_value = nullptr;
    for (const auto& [field_key, field_value] : event.fields) {
        if (field_key == key) {
            finded_value = &field_value;
        }
    }

    if (!finded_value || finded_value->empty()) {
        return false;
    }
    uint64_t number;
    const char* finded_value_c = finded_value->c_str();
    const char* finded_value_end = finded_value_c + finded_value->size();
    auto [incorrect_pointer, error_code] = std::from_chars(finded_value_c, finded_value_end, number);
    if (error_code == std::errc() && incorrect_pointer == finded_value_end) {
        *out = number;
        return true;
    }
    return false;
}
uint64_t GetIntField(const Event& event, const std::string& key, uint64_t fallback) {
    uint64_t parsed = 0;
    if (GetIntField(event, key, &parsed)) {
        return parsed;
    }
    return fallback;
}
bool IsProcessStart(const Event& event) {
    return event.type == "process_start";
}
bool IsFileWrite(const Event& event) {
    return event.type == "file_write";
}
bool IsNetConnect(const Event& event) {
    return event.type == "net_connect";
}

bool PathEndsWith(const Event& event, const std::string& suffix) {
    const std::string* path = FindField(event, "path");
    if (path == nullptr || path->size() < suffix.size()) {
        return false;
    }
    const std::size_t offset = path->size() - suffix.size();
    for (std::size_t i = 0; i < suffix.size(); ++i) {
        if (std::tolower(path->operator[](offset + i)) != std::tolower(suffix[i])) {
            return false;
        }
    }
    return true;
}

bool CommandLineContains(const Event& event, const std::string& needle) {
    const std::string* cmdline = FindField(event, "cmdline");
    if (cmdline == nullptr) return false;
    if (needle.empty()) return true;
    if (needle.size() > cmdline->size()) return false;
    for (std::size_t i = 0; i + needle.size() <= cmdline->size(); ++i) {
        bool match = true;
        for (std::size_t j = 0; j < needle.size(); ++j) {
            if (!EqualsInsensitive(cmdline->operator[](i + j), needle[j])) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

std::string NormalizePath(const std::string& path) {
    std::string expanded;
    std::cout << path << '\n';

    const std::size_t n = path.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (path[i] == '%') {
            const std::size_t end = path.find('%', i + 1);
            if (end != std::string::npos && end > i + 1) {
                std::string name = path.substr(i + 1, end - i - 1);
                std::string value;
                if (name == "TEMP" || name == "TMP") {
                    value = "C:\\Users\\user\\AppData\\Local\\Temp";
                } else if (name == "USERPROFILE") {
                    value = "C:\\Users\\user";
                } else if (name == "SystemRoot") {
                    value = "C:\\Windows";
                }
                if (!value.empty()) {
                    expanded.append(value);
                    i = end;
                    continue;
                }
            }
        }
        if (!(i > 0 && path[i] == '\\' && path[i - 1] == '\\')) {
            expanded.push_back(path[i]);
        }
    }

    for (char& c : expanded) {
        if (c == '/') {
            c = '\\';
        } else if (isupper(c)) {
            c = std::tolower(c);
        }
    }
    std::cout << expanded << '\n';
    return expanded;
}

}  // namespace nano_edr