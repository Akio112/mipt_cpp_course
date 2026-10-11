#include "fields.h"

#include <charconv>
#include <stdexcept>

namespace nano_edr {

namespace {
bool EqualsInsensitive(char a, char b) {
    return std::tolower(a) == std::tolower(b);
}

bool StartsAt(const std::string& text, std::size_t pos,
              const std::string& word) {
    if (pos + word.size() > text.size()) return false;
    for (std::size_t i = 0; i < word.size(); ++i) {
        if (!EqualsInsensitive(text[pos + i], word[i])) return false;
    }
    return true;
}
}  // namespace

//В случае, если поля нет выдает nullptr, вместо exception.
//Сделано т.к.для данной функции это стандартная логика.
//Поле, которое запрашивает пользовательно, необязательно должно существовать.
const std::string* FindField(const Event& event, const std::string& key) {
    for (const auto& [field_key, field_value] : event.fields) {
        if (field_key == key) {
            return &field_value;
        }
    }
    return nullptr;
}

// Бросает std::invalid_argument, если поля нет: раз оно объявлено обязательным,
// его отсутствие — нарушение контракта формата, и вернуть пустую строку значило
// бы соврать вызывающему. Сообщение исключения называет поле: диагностика,
// по которой нельзя найти причину, — половина диагностики.
const std::string& GetRequiredField(const Event& event, const std::string& key) {
    if (const std::string* field = FindField(event, key)) {
        return *field;
    }
    throw std::invalid_argument("У поля отсутствует значение " + key);
}

// Возвращает false, если поля нет или оно не разбирается как целое. Второе
// бывает не реже первого: значение приходит из внешнего источника,
// и «size=абв» — битые данные, а не ошибка программы.
bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    const std::string* value = FindField(event, key);
    if (value == nullptr || value->empty()) return false;

    uint64_t number;
    const char* first = value->data();
    const char* last = first + value->size();
    const auto [end, error] = std::from_chars(first, last, number);
    if (error != std::errc() || end != last) return false;

    *out = number;
    return true;
}

// Перегрузка со значением по умолчанию — для полей, отсутствие которых
// осмысленно: у события без ppid родителя просто нет.
uint64_t GetIntField(const Event& event, const std::string& key,
                     uint64_t fallback) {
    uint64_t parsed = 0;
    if (GetIntField(event, key, &parsed)) return parsed;
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
    if (path == nullptr || path->size() < suffix.size()) return false;

    const std::size_t offset = path->size() - suffix.size();
    for (std::size_t i = 0; i < suffix.size(); ++i) {
        if (!EqualsInsensitive((*path)[offset + i], suffix[i])) return false;
    }
    return true;
}

bool CommandLineContains(const Event& event, const std::string& needle) {
    const std::string* cmdline = FindField(event, "cmdline");
    if (cmdline == nullptr) return false;
    if (needle.empty()) return true;
    if (needle.size() > cmdline->size()) return false;

    for (std::size_t i = 0; i + needle.size() <= cmdline->size(); ++i) {
        if (StartsAt(*cmdline, i, needle)) return true;
    }
    return false;
}

std::string NormalizePath(const std::string& path) {
    const std::string temp = "\\appdata\\local\\temp";
    std::string result;

    for (std::size_t i = 0; i < path.size();) {
        if (StartsAt(path, i, "%TEMP%")) {
            result += temp;
            i += 6;
            continue;
        }
        if (StartsAt(path, i, "%TMP%")) {
            result += temp;
            i += 5;
            continue;
        }

        char ch = path[i++];
        if (ch == '/') {
            ch = '\\';
        }
        if (isupper(ch)) {
            ch = std::tolower(ch);
        }
        if (ch == '\\' && !result.empty() && result.back() == '\\') {
            continue;
        }
        result.push_back(ch);
    }
    return result;
}

}  // namespace nano_edr
