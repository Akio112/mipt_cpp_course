#include "agent.h"

#include <print>
#include <string>

#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {
Agent::Agent(std::size_t win, bool quiet)
    : window_(win), engine_(BuildAgentRules()), quiet_(quiet) {}

void Agent::HandleEvent(const Event& event) {
    ++events_;
    ++types_[event.type()];
    bool ctx_printed = false;
    std::vector<const IRule*> fired;
    engine_.ProcessEvent(event, &fired);
    for (const auto rule : fired) {
        if (!ctx_printed && !quiet_) {
            PrintContext(prev_ ? &prev_->event : nullptr,
                         prev2_ ? &prev2_->event : nullptr);
            ctx_printed = true;
        }
        std::println("[DETECT] {}  {}  ts={} pid={}",
                     SeverityName(rule->severity()), rule->id(),
                     event.raw_ts(), event.pid());
    }
    window_.PushBack(event);
    if (window_.capacity() == 1) {
        prev2_ = nullptr;
    } else {
        prev2_ = prev_;
    }
    prev_ = window_.back();
}

void Agent::OnSkipped() {
    ++skipped_;
}

void Agent::OnBroken() {
    ++failed_;
}

void Agent::OnTick() {}

void Agent::PrintSummary() const {
    if (quiet_) return;
    std::println("пропущено строк : {}", skipped_);
    std::println("событий всего : {}", events_);
    std::println("не обработано событий : {}", failed_);
    for (const auto& [type, count] : types_) {
        std::println("событий типа {} всего : {}", type, count);
    }
    std::println("правила, детектов:");
    for (std::size_t i = 0; i < engine_.size(); ++i) {
        const IRule& rule = engine_.rule(i);
        std::println("  {}  {}", rule.id(), rule.hits());
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
