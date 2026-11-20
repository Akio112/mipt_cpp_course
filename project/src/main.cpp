#include <exception>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <utility>

#include "agent.h"
#include "alert_sink.h"
#include "edr_handle.h"
#include "event_source.h"
#include "field_traits.h"
#include "file_source.h"
#include "os_source.h"

namespace nano_edr {
namespace {

struct Args {
    std::string path;
    std::string out;
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

        } else if (arg == "--out") {
            if (i + 1 >= argc) {
                return false;
            }
            args->out = argv[++i];
        } else if (arg.starts_with("--") || !args->path.empty()) {
            return false;
        } else {
            args->path = arg;
        }
    }
    return !args->path.empty();
}

int Run(const Args& args) {
    EdrHandle edr(args.path);
    const std::string text = edr.Config("dedup_period_ms", "10000");
    uint64_t period = 0;
    if (!FieldTraits<uint64_t>::Parse(text, &period)) {
        std::print(stderr, "[WARN] dedup_period_ms не разбирается: {}\n", text);
        period = 10000;
    }
    Agent agent(args.quiet, period);

    FanOutSink targets;
    targets.Add(ConsoleSink());
    if (!args.out.empty()) {
        targets.Add(FileSink(args.out));
    }
    targets.Add(EdrSink(&edr));
    agent.Subscribe(std::move(targets));

    std::unique_ptr<IEventSource> src;
    if (args.file) {
        src = std::make_unique<FileSource>(args.path);
    } else {
        src = std::make_unique<OsSource>(args.path);
    }
    src->Run(&agent);
    if (edr.Flush() != EDR_OK) {
        std::print(stderr, "[WARN] детекты не отправлены\n");
    }
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
                               "[--out <файл>] [--file] <журнал.log>\n");
            return 2;
        }
        return nano_edr::Run(args);
    } catch (const std::exception& e) {
        std::print(stderr, "ошибка: {}\n", e.what());
        return 1;
    }
}
