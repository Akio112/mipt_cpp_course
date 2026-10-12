#include "agent.h"

#include <print>
#include <string>

#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {

Agent::Agent(std::size_t win, bool quiet) : window_(win), quiet_(quiet) {}

void Agent::HandleEvent(const Event& event) {
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

void Agent::AddLine(bool comment) {
    ++lines_;
    if (comment) ++comments_;
}

void Agent::PrintSummary() const {
    if (quiet_) return;
    std::println("строк {}, из них комментариев {}", lines_, comments_);
    std::println("событий всего : {}", events_);
    for (const auto& [type, count] : types_) {
        std::println("событий типа {} всего : {}", type, count);
    }
}

void Agent::Trampoline(const os_event* ev, void* ctx) noexcept {
    try {
        EventParts parts;
        parts.ts = std::to_string(ev->ts);
        parts.type = ev->type;
        if (ev->pid != 0) parts.pid = std::to_string(ev->pid);
        for (std::size_t i = 0; i < ev->field_count; ++i) {
            parts.fields.push_back({ev->fields[i].key, ev->fields[i].value});
        }
        static_cast<Agent*>(ctx)->HandleEvent(Event(parts));
    } catch (...) {
    }
}

void Agent::PrintContext(const Event* prev, const Event* prev2) const {
    if (prev2) {
        std::print("[CTX] -2: ts={} type={} pid={}\n", prev2->raw_ts(),
                   prev2->type(), prev2->pid());
    }
    if (prev) {
        std::print("[CTX] -1: ts={} type={} pid={}\n", prev->raw_ts(),
                   prev->type(), prev->pid());
    }
}

}  // namespace nano_edr
