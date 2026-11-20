#include "event.h"

#include <charconv>
#include <stdexcept>
#include <utility>

#include "copy_stats.h"

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

Timestamp Event::Validate(const std::string& raw_ts, const std::string& type) {
    uint64_t ms = 0;
    const char* first = raw_ts.data();
    const char* last = first + raw_ts.size();
    const auto [end, err] = std::from_chars(first, last, ms);
    if (raw_ts.empty() || err != std::errc() || end != last) {
        throw std::invalid_argument("неверное время события: " + raw_ts);
    }
    if (type.empty()) {
        throw std::invalid_argument("пустой тип события: " + type);
    }
    return Timestamp{ms};
}

Event::Event(const EventParts& parts) {
    ts_ = Validate(parts.ts, parts.type);
    event_type_ = EventTypeFromString(parts.type);
    raw_ts_ = parts.ts;
    type_ = parts.type;
    pid_ = parts.pid;
    fields_ = parts.fields;
    copy_stats().event_strings_copied += 3 + 2 * parts.fields.size();
}

Event::Event(EventParts&& parts) {
    ts_ = Validate(parts.ts, parts.type);
    event_type_ = EventTypeFromString(parts.type);
    copy_stats().event_strings_moved += 3 + 2 * parts.fields.size();
    raw_ts_ = std::move(parts.ts);
    type_ = std::move(parts.type);
    pid_ = std::move(parts.pid);
    fields_ = std::move(parts.fields);
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
