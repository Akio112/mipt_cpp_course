#include <charconv>
#include <cstddef>
#include <exception>
#include <fstream>
#include <memory>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "agent_rules.h"
#include "event.h"
#include "event_list.h"
#include "os_handle.h"
#include "parse.h"
#include "rules.h"

namespace nano_edr {
namespace {

struct Args {
    std::string path;
    std::size_t win = 64;
    bool quiet = false;
    bool file = false;
};

bool ParseArgs(int argc, char** argv, Args* args) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--quiet") {
            args->quiet = true;
        } else if (arg == "--file") {
            if (args->file || !args->path.empty() || i + 1 >= argc) {
                return false;
            }
            args->file = true;
            args->path = argv[++i];

        } else if (arg == "--window-size") {
            if (i + 1 >= argc) {
                return false;
            }
            const std::string_view val = argv[++i];
            const char* first = val.data();
            const char* last = first + val.size();
            const auto [end, err] = std::from_chars(first, last, args->win);

            if (err != std::errc() || end != last) {
                return false;
            }
        } else if (arg.starts_with("--") || !args->path.empty()) {
            return false;
        } else {
            args->path = arg;
        }
    }
    return !args->path.empty();
}

class Agent {
 public:
    Agent(std::size_t win, bool quiet) : window_(win), quiet_(quiet) {}

    void HandleEvent(const Event& event) {
        ++events_;
        ++types_[event.type()];
        const int fired = CheckRules(event, AgentRules(), AgentRuleCount());
        if (fired > 0 && !quiet_) {
            PrintContext(prev_ ? &prev_->event : nullptr,
                         prev2_ ? &prev2_->event : nullptr);
        }
        window_.PushBack(event);
        if (window_.capacity() == 1) {
            prev2_ = nullptr;
        } else {
            prev2_ = prev_;
        }
        prev_ = window_.back();
    }

    void AddLine(bool comment) {
        ++lines_;
        if (comment) ++comments_;
    }

    void PrintSummary() const {
        if (quiet_) return;
        std::println("строк {}, из них комментариев {}", lines_, comments_);
        std::println("событий всего : {}", events_);
        for (const auto& [type, count] : types_) {
            std::println("событий типа {} всего : {}", type, count);
        }
    }

    static void Trampoline(const os_event* ev, void* ctx) noexcept {
        try {
            EventParts parts;
            parts.ts = std::to_string(ev->ts);
            parts.type = ev->type;
            if (ev->pid != 0) parts.pid = std::to_string(ev->pid);
            for (std::size_t i = 0; i < ev->field_count; ++i) {
                parts.fields.push_back({ev->fields[i].key,
                                        ev->fields[i].value});
            }
            static_cast<Agent*>(ctx)->HandleEvent(Event(parts));
        } catch (...) {
        }
    }

 private:
    void PrintContext(const Event* prev, const Event* prev2) const {
        if (prev2) {
            std::print("[CTX] -2: ts={} type={} pid={}\n", prev2->raw_ts(),
                       prev2->type(), prev2->pid());
        }
        if (prev) {
            std::print("[CTX] -1: ts={} type={} pid={}\n", prev->raw_ts(),
                       prev->type(), prev->pid());
        }
    }

    EventList window_;
    const EventNode* prev_ = nullptr;
    const EventNode* prev2_ = nullptr;
    bool quiet_ = false;
    int64_t lines_ = 0;
    int64_t comments_ = 0;
    int64_t events_ = 0;
    std::unordered_map<std::string, int64_t> types_;
};

class Source {
 public:
    virtual ~Source() = default;
    virtual void Run(Agent* agent) = 0;
};

class FileSource : public Source {
 public:
    explicit FileSource(const std::string& path) : log_(path) {
        if (!log_) throw std::runtime_error("не удалось открыть журнал: " + path);
    }

    void Run(Agent* agent) override {
        std::string line;
        while (std::getline(log_, line)) {
            const bool skip = IsBlankOrComment(line);
            agent->AddLine(skip && !line.empty());
            if (skip) continue;

            EventParts parts;
            if (!ParseEventParts(line, &parts)) continue;
            try {
                agent->HandleEvent(Event(parts));
            } catch (...) {
            }
        }
    }

 private:
    std::ifstream log_;
};

class OsSource : public Source {
 public:
    explicit OsSource(const std::string& path) : handle_(path) {}

    void Run(Agent* agent) override {
        handle_.Subscribe(Agent::Trampoline, agent);
        handle_.Start();
        os_status code = OS_TIMEOUT;
        while ((code = handle_.Wait(100)) == OS_TIMEOUT) {
        }
        if (code != OS_OK) {
            throw std::runtime_error("ошибка источника: " +
                                     std::string(os_status_str(code)));
        }
    }

 private:
    OsHandle handle_;
};

int Run(const Args& args) {
    Agent agent(args.win, args.quiet);
    std::unique_ptr<Source> src;
    if (args.file) {
        src = std::make_unique<FileSource>(args.path);
    } else {
        src = std::make_unique<OsSource>(args.path);
    }
    src->Run(&agent);
    agent.PrintSummary();
    return 0;
}

}  // namespace
}  // namespace nano_edr

int main(int argc, char** argv) {
    try {
        nano_edr::Args args;
        if (!nano_edr::ParseArgs(argc, argv, &args)) {
            std::print(stderr, "использование: nano-edr [--quiet] "
                               "[--window-size N] [--file] <журнал.log>\n");
            return 2;
        }
        return nano_edr::Run(args);
    } catch (const std::exception& e) {
        std::print(stderr, "ошибка: {}\n", e.what());
        return 1;
    }
}
