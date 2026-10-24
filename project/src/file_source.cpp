#include "file_source.h"

#include <stdexcept>
#include <string>

#include "event.h"
#include "parse.h"

namespace nano_edr {

FileSource::FileSource(const std::string& path) : log_(path) {
    if (!log_) {
        throw std::runtime_error("не удалось открыть журнал: " + path);
    }
}

void FileSource::Run(IEventHandler* handler) {
    std::string line;
    while (std::getline(log_, line)) {
        const bool skip = IsBlankOrComment(line);
        if (skip) {
            handler->OnSkipped();
            continue;
        }

        EventParts parts;
        if (!ParseEventParts(line, &parts)) {
            handler->OnBroken();
            continue;
        }
        try {
            handler->HandleEvent(Event(parts));
        } catch (...) {
            handler->OnBroken();
        }
    }
}

}  // namespace nano_edr
