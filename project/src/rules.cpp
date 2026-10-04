#include "rules.h"

#include <cstddef>
#include <cstdio>

namespace nano_edr {

const char* SeverityName(Severity severity) {
    switch (severity) {
        case Severity::kLow: return "low";
        case Severity::kMedium: return "medium";
        case Severity::kHigh: return "high";
        case Severity::kCritical: return "critical";
    }
    return "?";
}

std::size_t CheckRules(const Event& event, const Rule* rules, std::size_t rule_count) {
    if (rules == nullptr) {
        return 0;
    }

    std::size_t detected = 0;
    for (std::size_t i = 0; i < rule_count; ++i) {
        if (rules[i].check == nullptr) {
            continue;
        }
        if (rules[i].check(event)) {
            std::printf("[DETECT] rule=%s severity=%s\n",
                        rules[i].id, SeverityName(rules[i].severity));
            ++detected;
        }
    }
    return detected;
}

}  // namespace nano_edr