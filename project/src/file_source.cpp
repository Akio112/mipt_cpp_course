#include "file_source.h"

#include <stdexcept>
#include <string>

#include "agent.h"
#include "event.h"
#include "parse.h"

namespace nano_edr {

FileSource::FileSource(const std::string& path) : log_(path) {
    if (!log_) throw std::runtime_error("не удалось открыть журнал: " + path);
}

void FileSource::Run(Agent* agent) {
    std::string line;
    while (std::getline(log_, line)) {
        const bool skip = IsBlankOrComment(line);
        agent->AddLine(skip && !line.empty());
        if (skip) continue;

        EventParts parts;
        if (!ParseEventParts(line, &parts)) {
            agent->FailEvent();
            continue;
        }
        try {
            agent->HandleEvent(Event(parts));
        } catch (...) {
            agent->FailEvent();
        }
    }
}

}  // namespace nano_edr
