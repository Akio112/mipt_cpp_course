#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "event.h"
#include "event_list.h"
#include "event_source.h"
#include "rule_engine.h"

namespace nano_edr {

class Agent : public IEventHandler {
 public:
    Agent(std::size_t win, bool quiet);

    void HandleEvent(const Event& event) override;
    void OnSkipped() override;
    void OnBroken() override;
    void OnTick() override;
    void PrintSummary() const;

 private:
    void PrintContext(const Event* prev, const Event* prev2) const;

    EventList window_;
    RuleEngine engine_;
    const EventNode* prev_ = nullptr;
    const EventNode* prev2_ = nullptr;
    int64_t skipped_ = 0;
    int64_t events_ = 0;
    int64_t failed_ = 0;
    std::unordered_map<std::string, int64_t> types_;
    bool quiet_ = false;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
