#include <cstddef>
#include <format>
#include <string>
#include <utility>

#include "response.h"

namespace nano_edr {

const char* ActionKindName(ActionKind kind) {
    switch (kind) {
        case ActionKind::kKillProcess:
            return "kill";
        case ActionKind::kQuarantineFile:
            return "quarantine";
        case ActionKind::kDeleteFile:
            return "delete";
    }
    return "unknown";
}

std::string ToString(const Action& action) {
    std::string text = ActionKindName(action.kind);
    if (action.kind == ActionKind::kKillProcess) {
        text += std::format(" pid={} start={}", action.pid,
                            action.expected_start.ms);
    } else {
        text += " " + action.path;
        if (!action.to.empty()) {
            text += " -> " + action.to;
        }
    }
    if (!action.reason.empty()) {
        text += "  (" + action.reason + ")";
    }
    return text;
}

bool ResponsePlan::Has(const std::string& target) const {
    for (std::size_t i = 0; i < actions_.size(); ++i) {
        if (actions_[i].target() == target) {
            return true;
        }
    }
    return false;
}

bool ResponsePlan::Add(Action action) {
    if (action.target().empty()) {
        return false;
    }
    if (Has(action.target())) {

        return false;
    }

    actions_.emplace_back(std::move(action));
    return true;
}

}
