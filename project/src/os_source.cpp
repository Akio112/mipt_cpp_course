#include "os_source.h"

#include <stdexcept>
#include <string>

#include "event.h"
#include "os.h"

namespace nano_edr {
namespace {

Event ConvertEventFromOs(const os_event& ev) {
    EventParts parts;
    parts.ts = std::to_string(ev.ts);
    parts.type = ev.type ? ev.type : "";
    if (ev.pid != 0) {
        parts.pid = std::to_string(ev.pid);
    }
    for (std::size_t i = 0; i < ev.field_count; ++i) {
        parts.fields.push_back({ev.fields[i].key, ev.fields[i].value});
    }
    return Event(parts);
}

}  // namespace

OsSource::OsSource(const std::string& path) : handle_(path) {}

void OsSource::Run(IEventHandler* handler) {
    handle_.Subscribe(OsSource::Trampoline, handler);
    handle_.Start();
    os_status code = OS_TIMEOUT;
    while ((code = handle_.Wait(100)) == OS_TIMEOUT) {
        handler->OnTick();
    }
    if (code != OS_OK) {
        throw std::runtime_error("ошибка источника: " +
                                 std::string(os_status_str(code)));
    }
}

void OsSource::Trampoline(const os_event* ev, void* ctx) noexcept {
    IEventHandler* handler = static_cast<IEventHandler*>(ctx);
    try {
        handler->HandleEvent(ConvertEventFromOs(*ev));
    } catch (...) {
        handler->OnBroken();
    }
}

}  // namespace nano_edr
