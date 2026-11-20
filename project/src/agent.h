#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "alert_sink.h"
#include "entity_model.h"
#include "event.h"
#include "event_source.h"
#include "response.h"
#include "ring_buffer.h"
#include "window_entry.h"
#include "rule_engine.h"

namespace nano_edr {

class Agent : public IEventHandler {
 public:
    Agent(bool quiet, uint64_t period_ms);

    void Subscribe(AlertSink sink);
    void EnableResponse(OsHandle* os, bool dry_run,
                        const std::string& quarantine_dir);
    void HandleEvent(const Event& event) override;
    void OnSkipped() override;
    void OnBroken() override;
    void OnTick() override;
    void PrintSummary() const;

 private:
    static constexpr std::size_t kWindow = 256;

    void PrintContextOnce();

    RingBuffer<WindowEntry, kWindow> window_;
    EntityModel model_;
    RuleEngine engine_;
    std::unique_ptr<ResponsePlanner> planner_{};
    std::unique_ptr<Responder> responder_{};
    int64_t skipped_ = 0;
    int64_t events_ = 0;
    int64_t failed_ = 0;
    std::unordered_map<std::string, int64_t> types_;
    bool quiet_ = false;
    bool ctx_done_ = false;
    uint64_t dedup_period_ms_{};
    std::size_t suppressed_{};
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
