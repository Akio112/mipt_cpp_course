#include "agent.h"

#include <print>
#include <string>

#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {
Agent::Agent(bool quiet) : engine_(BuildAgentRules()), quiet_(quiet) {}

void Agent::HandleEvent(const Event& event) {
    ++events_;
    ++types_[event.type()];
    bool ctx_printed = false;
    std::vector<const IRule*> fired;
    engine_.ProcessEvent(event, &fired);
    for (const auto rule : fired) {
        if (!ctx_printed && !quiet_) {
            PrintContext();
            ctx_printed = true;
        }
        std::println("[DETECT] {}  {}  ts={} pid={}",
                     SeverityName(rule->severity()), rule->id(),
                     event.raw_ts(), event.pid());
    }
    window_.PushBack({event.raw_ts(), event.type(), event.pid()});
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
    std::println("[STAT] в окне {} из {} мест", window_.size(), window_.capacity());
    std::println("правила, детектов:");
    for (std::size_t i = 0; i < engine_.size(); ++i) {
        const IRule& rule = engine_.rule(i);
        std::println("  {}  {}", rule.id(), rule.hits());
    }
}

void Agent::PrintContext() const {
    const std::size_t size = window_.size();
    for (std::size_t i = size > 2 ? size - 2 : 0; i < size; ++i) {
        const Entry& entry = window_[i];
        std::print("[CTX] -{}: {} {} pid={}\n", size - i, entry.ts, entry.type,
                   entry.pid);
    }
}

}  // namespace nano_edr
