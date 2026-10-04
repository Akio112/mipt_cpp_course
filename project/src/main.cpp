#include <charconv>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <print>
#include <string>
#include <unordered_map>
#include <vector>

#include "../kit/include/l1.2/parse.h"
#include "../kit/include/l1.2/event_list.h"

int main(int argc, char** argv) {
    const std::vector<std::string> attributes = {
        "wscript.exe",
        ".locked",
        "certutil.exe",
        "\\Startup\\",
    };

    bool is_quiet  = false;
    long long window_max_size = 64;

    if (argc < 2) {
        std::print(stderr, "использование: nano-edr <журнал.log>\n");
        return 2;
    }

    std::ifstream log(argv[1]);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", argv[1]);
        return 2;
    }

    for (int i = 2; i < argc; ++i) {
        std::string now_arg_string = argv[i];
        if (now_arg_string == "--quiet") {
            is_quiet = true;
        } else if (now_arg_string == "--window-size" && i + 1 < argc) {
            const char* s = argv[i + 1];
            long long n = 0;
            auto [incorrect_pointer, error_code] = std::from_chars(s, s + std::strlen(s), n);
            if (error_code == std::errc() && n >= 0) { window_max_size = n; ++i; }
        }
    }

    nano_edr::EventList window{};
    window.head     = nullptr;
    window.tail     = nullptr;
    window.size     = 0;
    window.capacity = (std::size_t)window_max_size;

    long long lines = 0, comments = 0, events = 0;
    std::unordered_map<std::string, long long> event_types_count;
    std::string line;

    while (std::getline(log, line)) {
        ++lines;
        if (nano_edr::IsBlankOrComment(&line)) {
            if (!line.empty()) ++comments;
            continue;
        }

        bool detected = false;
        for (auto& attribute : attributes) {
            if (line.find(attribute) != std::string::npos) {
                std::print("[DETECT] строка {}, признак {}: {}\n",
                           lines, attribute, line);
                detected = true;
            }
        }

        nano_edr::Event now_event;
        if (!nano_edr::ParseEventLine(&line, &now_event)) continue;

        ++events;
        ++event_types_count[now_event.type];


        if (detected && !is_quiet) {
            const nano_edr::Event* second_last = nullptr; 
            const nano_edr::Event* last = nullptr;

            for (nano_edr::EventNode* node = window.head; node; node = node->next) {
                second_last = last;
                last = &node->event;
            }

            if (second_last) std::print("[CTX] -2: ts={} type={} pid={}\n", second_last->ts, second_last->type, second_last->pid);
            if (last) std::print("[CTX] -1: ts={} type={} pid={}\n", last->ts, last->type, last->pid);
    }

        ListPushBack(&window, &now_event);
    }

    if (!is_quiet) {
        std::println("строк {}, из них комментариев {}\n", lines, comments);
        std::println("событий всего : {}", events);
        for (auto& [type, count] : event_types_count)
            std::println("событий типа {} всего : {}", type, count);
    }
    return 0;
}