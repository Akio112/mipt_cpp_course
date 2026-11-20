#ifndef NANO_EDR_AGENT_RULES_H
#define NANO_EDR_AGENT_RULES_H

#include "entity_model.h"
#include "rule_engine.h"

namespace nano_edr {

RuleEngine BuildAgentRules(const EntityModel* model);

}  // namespace nano_edr

#endif
