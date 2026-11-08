#include <exception>
#include <memory>
#include <print>
#include <string>
#include <string_view>

#include "agent.h"
#include "event_source.h"
#include "file_source.h"
#include "os_source.h"

namespace nano_edr {
namespace {

struct Args {
    std::string path;
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

        } else if (arg.starts_with("--") || !args->path.empty()) {
            return false;
        } else {
            args->path = arg;
        }
    }
    return !args->path.empty();
}

int Run(const Args& args) {
    Agent agent(args.quiet);
    std::unique_ptr<IEventSource> src;
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
                               "[--file] <журнал.log>\n");
            return 2;
        }
        return nano_edr::Run(args);
    } catch (const std::exception& e) {
        std::print(stderr, "ошибка: {}\n", e.what());
        return 1;
    }
}
