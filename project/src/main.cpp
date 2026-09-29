#include <charconv>
#include <cstddef>
#include <exception>
#include <fstream>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>

#include "agent_rules.h"
#include "event.h"
#include "event_list.h"
#include "parse.h"
#include "rules.h"

namespace nano_edr {
namespace {

void ParseArgs(int argc, char** argv,
               std::string& log_path,
               std::size_t& window_size,
               bool& is_quiet) {
    if (argc < 2) return;
    log_path = argv[1];

    for (int i = 2; i < argc; ++i) {
        const std::string_view arg_text = argv[i];
        if (arg_text == "--quiet") {
            is_quiet = true;
        } else if (arg_text == "--window-size" && i + 1 < argc) {
            const std::string_view size_text = argv[i + 1];
            const char* first = size_text.data();
            const char* last = first + size_text.size();

            std::size_t size_value = 0;
            const auto [end_ptr, errc] =
                std::from_chars(first, last, size_value);

            if (errc == std::errc() && end_ptr == last) {
                window_size = size_value;
                ++i;
            }
        }
    }
}

void PrintSummary(long long line_count, long long comment_count,
                  long long events_count,
                  const std::unordered_map<std::string, long long>& by_type) {
    std::println("строк {}, из них комментариев {}",
                 line_count, comment_count);
    std::println("событий всего : {}", events_count);

    for (const auto& [type, count] : by_type) {
        std::println("событий типа {} всего : {}", type, count);
    }
}

void PrintContext(const Event& prev_event, const Event& prev2_event,
                  bool has_prev, bool has_prev2) {
    if (has_prev2) {
        std::print("[CTX] -2: ts={} type={} pid={}\n",
                   prev2_event.ts, prev2_event.type, prev2_event.pid);
    }
    if (has_prev) {
        std::print("[CTX] -1: ts={} type={} pid={}\n",
                   prev_event.ts, prev_event.type, prev_event.pid);
    }
}

int RunLog(std::istream& log, std::size_t window_size, bool is_quiet) {
    EventList window{};
    window.capacity = window_size;

    long long line_count = 0;
    long long comment_count = 0;
    long long events_count = 0;
    std::unordered_map<std::string, long long> type_counts;

    Event prev_event;
    Event prev2_event;
    bool has_prev = false;
    bool has_prev2 = false;

    std::string line;
    while (std::getline(log, line)) {
        ++line_count;
        if (IsBlankOrComment(&line)) {
            if (!line.empty()) ++comment_count;
            continue;
        }

        Event event;
        if (!ParseEventLine(&line, &event)) continue;

        ++events_count;
        ++type_counts[event.type];

        const int fired = CheckRules(event, kAgentRules, kAgentRuleCount);
        if (fired > 0 && !is_quiet) {
            PrintContext(prev_event, prev2_event, has_prev, has_prev2);
        }

        prev2_event = prev_event;
        has_prev2 = has_prev;
        prev_event = event;
        has_prev = true;

        ListPushBack(&window, &event);
    }

    if (!is_quiet) {
        PrintSummary(line_count, comment_count, events_count, type_counts);
    }
    return 0;
}

}  // namespace
}  // namespace nano_edr

int main(int argc, char** argv) {
    try {
        std::string log_path;
        std::size_t window_size = 64;
        bool is_quiet = false;
        nano_edr::ParseArgs(argc, argv, log_path, window_size, is_quiet);

        if (log_path.empty()) {
            std::print(stderr, "использование: nano-edr <журнал.log>\n");
            return 2;
        }

        std::ifstream log(log_path);
        if (!log) {
            std::print(stderr, "не удалось открыть журнал: {}\n", log_path);
            return 2;
        }

        return nano_edr::RunLog(log, window_size, is_quiet);
    } catch (const std::invalid_argument& e) {
        std::print(stderr, "нарушен контракт формата: {}\n", e.what());
        return 1;
    } catch (const std::exception& e) {
        std::print(stderr, "ошибка: {}\n", e.what());
        return 1;
    }
}