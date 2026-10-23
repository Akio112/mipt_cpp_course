#include "agent_rules.h"

#include "conditions.h"
#include "rules.h"

namespace nano_edr {
namespace {

MatchRule ScriptTemp() {
    MatchRule script("script_host_from_temp", Severity::kHigh);
    script.AddCondition(Condition::Equals("type", {"process_start"}));
    script.AddCondition(
        Condition::EndsWith("image", {"wscript.exe", "cscript.exe"}));
    script.AddCondition(Condition::Contains("cmdline", {"\\appdata\\local\\temp\\", "/appdata/local/temp/",
                                                        "\\windows\\temp\\", "/windows/temp/", "%temp%", "%tmp%"}));
    script.AddAction("kill_process");
    return script;
}

MatchRule Lolbin() {
    MatchRule lolbin("lolbin_download", Severity::kHigh);
    lolbin.AddCondition(Condition::Equals("type", {"process_start"}));
    lolbin.AddCondition(
        Condition::EndsWith("image", {"certutil.exe", "bitsadmin.exe"}));
    lolbin.AddCondition(Condition::Contains(
        "cmdline", {"urlcache", "transfer", "http:", "https:"}));
    lolbin.AddAction("kill_process");
    return lolbin;
}

MatchRule HiddenPs() {
    MatchRule hidden("hidden_powershell", Severity::kMedium);
    hidden.AddCondition(Condition::Equals("type", {"process_start"}));
    hidden.AddCondition(
        Condition::EndsWith("image", {"powershell.exe", "pwsh.exe"}));
    hidden.AddCondition(Condition::Contains(
        "cmdline", {"-w hidden", "-windowstyle hidden", "-enc",
                    "-encodedcommand"}));
    hidden.AddAction("kill_process");
    return hidden;
}

MatchRule AutostartPath() {
    MatchRule start_path("autostart_write", Severity::kHigh);
    start_path.AddCondition(
        Condition::Equals("type", {"file_create", "file_write"}));
    start_path.AddCondition(Condition::Contains(
        "path", {"\\start menu\\programs\\startup\\",
                 "/start menu/programs/startup/"}));
    start_path.AddAction("quarantine_file");
    return start_path;
}

MatchRule AutostartTo() {
    MatchRule start_to("autostart_write", Severity::kHigh);
    start_to.AddCondition(Condition::Equals("type", {"file_move"}));
    start_to.AddCondition(Condition::Contains(
        "to", {"\\start menu\\programs\\startup\\",
               "/start menu/programs/startup/"}));
    start_to.AddAction("quarantine_file");
    return start_to;
}

MatchRule RansomPath() {
    MatchRule lock_path("ransom_extension", Severity::kCritical);
    lock_path.AddCondition(
        Condition::Equals("type", {"file_create", "file_write"}));
    lock_path.AddCondition(Condition::EndsWith("path", {".locked"}));
    lock_path.AddAction("quarantine_file");
    return lock_path;
}

MatchRule RansomTo() {
    MatchRule lock_to("ransom_extension", Severity::kCritical);
    lock_to.AddCondition(Condition::Equals("type", {"file_move"}));
    lock_to.AddCondition(Condition::EndsWith("to", {".locked"}));
    lock_to.AddAction("quarantine_file");
    return lock_to;
}

}  // namespace

std::vector<MatchRule> BuildAgentRules() {
    std::vector<MatchRule> rules;
    rules.reserve(7);

    rules.push_back(ScriptTemp());
    rules.push_back(Lolbin());
    rules.push_back(HiddenPs());
    rules.push_back(AutostartPath());
    rules.push_back(AutostartTo());
    rules.push_back(RansomPath());
    rules.push_back(RansomTo());

    return rules;
}

}  // namespace nano_edr
