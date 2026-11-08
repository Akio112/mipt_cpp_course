#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "event.h"
#include "event_source.h"
#include "ring_buffer.h"
#include "rule_engine.h"

namespace nano_edr {

class Agent : public IEventHandler {
 public:
    explicit Agent(bool quiet);

    void HandleEvent(const Event& event) override;
    void OnSkipped() override;
    void OnBroken() override;
    void OnTick() override;
    void PrintSummary() const;

 private:
    struct Entry {
        std::string ts;
        std::string type;
        std::string pid;
    };

    static constexpr std::size_t kWindow = 256;

    void PrintContext() const;

    RingBuffer<Entry, kWindow> window_;
    RuleEngine engine_;
    int64_t skipped_ = 0;
    int64_t events_ = 0;
    int64_t failed_ = 0;
    std::unordered_map<std::string, int64_t> types_;
    bool quiet_ = false;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
