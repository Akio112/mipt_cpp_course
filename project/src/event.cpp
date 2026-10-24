#include "event.h"

#include <charconv>
#include <stdexcept>

namespace nano_edr {

EventType EventTypeFromString(const std::string& type) {
    if (type == "process_start") return EventType::kProcessStart;
    if (type == "process_end") return EventType::kProcessEnd;
    if (type == "file_create") return EventType::kFileCreate;
    if (type == "file_write") return EventType::kFileWrite;
    if (type == "file_delete") return EventType::kFileDelete;
    if (type == "file_move") return EventType::kFileMove;
    if (type == "net_connect") return EventType::kNetConnect;
    return EventType::kOther;
}

Event::Event(const EventParts& parts)
    : raw_ts_(parts.ts), type_(parts.type), pid_(parts.pid), fields_(parts.fields) {
    uint64_t ms = 0;
    const char* first = raw_ts_.data();
    const char* last = first + raw_ts_.size();
    const auto [end, err] = std::from_chars(first, last, ms);

    if (raw_ts_.empty() || err != std::errc() || end != last) {
        throw std::invalid_argument("неверное время события: " + raw_ts_);
    }
    if (type_.empty()) {
        throw std::invalid_argument("пустой тип события: " + type_);
    }
    ts_.ms = ms;
    event_type_ = EventTypeFromString(type_);
}

std::string ToString(const Event& event) {
    std::string text = "ts=" + event.raw_ts() + " type=" + event.type() +
                       " pid=" + event.pid();
    for (const auto& field : event.fields()) {
        text += " " + field.key + "=" + field.value;
    }
    return text;
}

std::ostream& operator<<(std::ostream& out, const Event& event) {
    return out << ToString(event);
}

}  // namespace nano_edr
