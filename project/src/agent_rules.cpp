#include "agent_rules.h"

#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "conditions.h"
#include "rule.h"

namespace nano_edr {
namespace {

std::unique_ptr<EventTypeIs> Types(std::vector<EventType> types) {
    return std::make_unique<EventTypeIs>(types);
}

std::unique_ptr<AnyOf> Contains(const std::string& key,
                                std::initializer_list<const char*> values) {
    auto parts = std::make_unique<AnyOf>();
    for (const auto value : values) {
        parts->Add(std::make_unique<FieldContains>(key, value));
    }
    return parts;
}

std::unique_ptr<AnyOf> EndsWith(const std::string& key,
                                std::initializer_list<const char*> values) {
    auto parts = std::make_unique<AnyOf>();
    for (const auto value : values) {
        parts->Add(std::make_unique<FieldEndsWith>(key, value));
    }
    return parts;
}

std::unique_ptr<AnyOf> TempPath() {
    return Contains("cmdline", {"\\appdata\\local\\temp\\", "/appdata/local/temp/",
                                "\\windows\\temp\\", "/windows/temp/", "%temp%", "%tmp%"});
}

std::unique_ptr<MatchRule> ScriptTemp() {
    auto rule = std::make_unique<MatchRule>("script_host_from_temp",
                                            Severity::kHigh);
    rule->AddCondition(Types({EventType::kProcessStart}));
    rule->AddCondition(EndsWith("image", {"wscript.exe", "cscript.exe"}));
    rule->AddCondition(TempPath());
    rule->AddAction("kill_process");
    return rule;
}

std::unique_ptr<MatchRule> Lolbin() {
    auto rule = std::make_unique<MatchRule>("lolbin_download", Severity::kHigh);
    rule->AddCondition(Types({EventType::kProcessStart}));
    rule->AddCondition(EndsWith("image", {"certutil.exe", "bitsadmin.exe"}));
    rule->AddCondition(Contains(
        "cmdline", {"urlcache", "transfer", "http:", "https:"}));
    rule->AddAction("kill_process");
    return rule;
}

std::unique_ptr<MatchRule> HiddenPs() {
    auto rule = std::make_unique<MatchRule>("hidden_powershell",
                                            Severity::kMedium);
    rule->AddCondition(Types({EventType::kProcessStart}));
    rule->AddCondition(EndsWith("image", {"powershell.exe", "pwsh.exe"}));
    rule->AddCondition(Contains(
        "cmdline", {"-w hidden", "-windowstyle hidden", "-enc",
                    "-encodedcommand"}));
    rule->AddAction("kill_process");
    return rule;
}

std::unique_ptr<MatchRule> Autostart() {
    auto rule = std::make_unique<MatchRule>("autostart_write", Severity::kHigh);
    rule->AddCondition(Types({EventType::kFileCreate, EventType::kFileWrite,
                              EventType::kFileMove}));
    auto paths = std::make_unique<AnyOf>();
    for (const auto key : {"path", "to"}) {
        paths->Add(Contains(key, {"\\start menu\\programs\\startup\\",
                                  "/start menu/programs/startup/"}));
    }
    rule->AddCondition(std::move(paths));
    rule->AddAction("quarantine_file");
    return rule;
}

std::unique_ptr<MatchRule> RansomExt() {
    auto rule = std::make_unique<MatchRule>("ransom_extension",
                                            Severity::kCritical);
    rule->AddCondition(Types({EventType::kFileCreate, EventType::kFileWrite,
                              EventType::kFileMove}));
    auto paths = std::make_unique<AnyOf>();
    paths->Add(std::make_unique<FieldEndsWith>("path", ".locked"));
    paths->Add(std::make_unique<FieldEndsWith>("to", ".locked"));
    rule->AddCondition(std::move(paths));
    rule->AddAction("quarantine_file");
    return rule;
}

std::unique_ptr<SequenceRule> ScriptBeacon() {
    auto rule = std::make_unique<SequenceRule>("script_host_beacon",
                                               Severity::kCritical, 10000);
    auto start = std::make_unique<AllOf>();
    start->Add(Types({EventType::kProcessStart}));
    start->Add(EndsWith("image", {"wscript.exe", "cscript.exe"}));
    start->Add(TempPath());
    rule->SetFirst(std::move(start));
    rule->SetSecond(Types({EventType::kNetConnect}));
    rule->AddAction("kill_process");
    return rule;
}

}  // namespace

RuleEngine BuildAgentRules() {
    RuleEngine engine;
    engine.AddRule(ScriptTemp());
    engine.AddRule(Lolbin());
    engine.AddRule(HiddenPs());
    engine.AddRule(Autostart());
    engine.AddRule(RansomExt());
    engine.AddRule(ScriptBeacon());
    return engine;
}

}  // namespace nano_edr
