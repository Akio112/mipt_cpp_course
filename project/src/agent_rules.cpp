#include "agent_rules.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "combinators.h"
#include "conditions.h"
#include "fields.h"
#include "make_rule.h"
#include "rule.h"
#include "text.h"

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
    rule->AddAction("quarantine_source");
    rule->AddAction("delete_dropped");
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
    rule->AddAction("delete_evidence");
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
    rule->AddAction("kill_process");
    rule->AddAction("quarantine_image");
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

struct IsDoc {
    std::string key;

    bool operator()(const Event& event) const {
        const std::string* path = FindField(event, key);
        if (path == nullptr) return false;
        static const std::string formats[] = {".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
                                              ".pdf", ".txt", ".csv", ".jpg", ".png", ".mp4",
                                              ".zip", ".7z", ".vsdx", ".locked"};
        for (const auto& ext : formats) {
            if (TextEndsWith(*path, ext)) return true;
        }
        return false;
    }
};

std::unique_ptr<ThresholdRule<128>> MassWrite() {
    auto rule = MakeRule<ThresholdRule<128>>("ransomware_mass_write",
                                             Severity::kCritical,
                                             20,
                                             10000,
                                             "pid");
    rule->SetCondition(MakeCondition(AllOfStatic(
        EventTypeIs({EventType::kFileCreate, EventType::kFileWrite,
                     EventType::kFileMove}),
        AnyOfStatic(IsDoc{"path"}, IsDoc{"from"}, IsDoc{"to"}))));
    rule->AddAction("kill_process");
    rule->AddAction("quarantine_image");
    return rule;
}

std::unique_ptr<SequenceRule> ShellScript(const EntityModel* model) {
    auto rule = MakeRule<SequenceRule>("shell_spawns_script_host",
                                       Severity::kHigh, 30000);
    auto shell = std::make_unique<AllOf>();
    shell->Add(Types({EventType::kProcessStart}));
    shell->Add(EndsWith("image", {"cmd.exe", "powershell.exe", "pwsh.exe"}));
    auto host = std::make_unique<AllOf>();
    host->Add(Types({EventType::kProcessStart}));
    host->Add(EndsWith("image", {"wscript.exe", "cscript.exe"}));
    rule->SetFirst(std::move(shell));
    rule->SetSecond(std::move(host));
    rule->set_child_of_prev(true);
    rule->SetModel(model);
    rule->AddAction("kill_process");
    return rule;
}

std::unique_ptr<LambdaRule> Dropped(const EntityModel* model) {
    auto rule = MakeRule<LambdaRule>(
        "spawned_from_dropped_file", Severity::kHigh,
        [model](const Event& event) {
            if (event.event_type() != EventType::kProcessStart) {
                return false;
            }
            const FileRecord* source = model->FindSpawnSource(event.pid());
            return source != nullptr && !source->writer_pid.empty() &&
                   source->writer_pid != event.pid();
        });
    rule->AddAction("kill_process");
    rule->AddAction("quarantine_source");
    rule->AddAction("delete_dropped");
    return rule;
}

}  // namespace

RuleEngine BuildAgentRules(const EntityModel* model) {
    RuleEngine engine;
    engine.AddRule(ScriptTemp());
    engine.AddRule(Lolbin());
    engine.AddRule(HiddenPs());
    engine.AddRule(Autostart());
    engine.AddRule(RansomExt());
    engine.AddRule(ScriptBeacon());
    engine.AddRule(MassWrite());
    engine.AddRule(ShellScript(model));
    engine.AddRule(Dropped(model));
    return engine;
}

}  // namespace nano_edr
