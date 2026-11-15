#include "agent.h"

#include <print>
#include <string>
#include <utility>

#include "agent_rules.h"
#include "dedup_sink.h"
#include "rules.h"

namespace nano_edr {
Agent::Agent(bool quiet, uint64_t period_ms)
    : engine_(BuildAgentRules(&model_)),
      quiet_(quiet),
      dedup_period_ms_(period_ms) {
    engine_.SetModel(&model_);
}

void Agent::Subscribe(AlertSink sink) {
    FanOutSink all;
    all.Add([this](const Detection&) { PrintContextOnce(); });
    all.Add(std::move(sink));
    engine_.Subscribe(DedupSink(std::move(all), dedup_period_ms_, &suppressed_));
}

void Agent::HandleEvent(const Event& event) {
    model_.Observe(event);
    ++events_;
    ++types_[event.type()];
    ctx_done_ = false;
    engine_.ProcessEvent(event, nullptr);
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
    std::println("[STAT] в модели процессов {}, файлов {}",
                 model_.process_count(), model_.file_count());
    if (suppressed_ > 0) {
        std::println("[STAT] подавлено дубликатов {} (срок {} мс)",
                     suppressed_, dedup_period_ms_);
    }
    std::println("правила, детектов:");
    for (std::size_t i = 0; i < engine_.size(); ++i) {
        const IRule& rule = engine_.rule(i);
        std::println("  {}  {}", rule.id(), rule.hits());
    }
}

void Agent::PrintContextOnce() {
    if (quiet_ || ctx_done_) return;
    ctx_done_ = true;
    const std::size_t size = window_.size();
    for (std::size_t i = size > 2 ? size - 2 : 0; i < size; ++i) {
        const Entry& entry = window_[i];
        std::print("[CTX] -{}: {} {} pid={}\n", size - i, entry.ts, entry.type,
                   entry.pid);
    }
}

}  // namespace nano_edr
