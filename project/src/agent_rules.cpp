#include "agent_rules.h"

#include "conditions.h"
#include "rules.h"

namespace nano_edr {

std::vector<MatchRule> BuildAgentRules() {
    std::vector<MatchRule> rules;

    MatchRule script("script_host_from_temp", Severity::kHigh);
    script.AddCondition(Condition::Equals("type", {"process_start"}));
    script.AddCondition(
        Condition::EndsWith("image", {"wscript.exe", "cscript.exe"}));
    script.AddCondition(Condition::Contains("cmdline", {
        "\\appdata\\local\\temp\\", "/appdata/local/temp/",
        "\\windows\\temp\\", "/windows/temp/", "%temp%", "%tmp%"}));
    script.AddAction("kill_process");
    rules.push_back(script);

    MatchRule lolbin("lolbin_download", Severity::kHigh);
    lolbin.AddCondition(Condition::Equals("type", {"process_start"}));
    lolbin.AddCondition(
        Condition::EndsWith("image", {"certutil.exe", "bitsadmin.exe"}));
    lolbin.AddCondition(Condition::Contains(
        "cmdline", {"urlcache", "transfer", "http:", "https:"}));
    lolbin.AddAction("kill_process");
    rules.push_back(lolbin);

    MatchRule hidden("hidden_powershell", Severity::kMedium);
    hidden.AddCondition(Condition::Equals("type", {"process_start"}));
    hidden.AddCondition(
        Condition::EndsWith("image", {"powershell.exe", "pwsh.exe"}));
    hidden.AddCondition(Condition::Contains(
        "cmdline", {"-w hidden", "-windowstyle hidden", "-enc",
                    "-encodedcommand"}));
    hidden.AddAction("kill_process");
    rules.push_back(hidden);

    MatchRule start_path("autostart_write", Severity::kHigh);
    start_path.AddCondition(
        Condition::Equals("type", {"file_create", "file_write"}));
    start_path.AddCondition(Condition::Contains(
        "path", {"\\start menu\\programs\\startup\\",
                 "/start menu/programs/startup/"}));
    start_path.AddAction("quarantine_file");
    rules.push_back(start_path);

    MatchRule start_to("autostart_write", Severity::kHigh);
    start_to.AddCondition(Condition::Equals("type", {"file_move"}));
    start_to.AddCondition(Condition::Contains(
        "to", {"\\start menu\\programs\\startup\\",
               "/start menu/programs/startup/"}));
    start_to.AddAction("quarantine_file");
    rules.push_back(start_to);

    MatchRule lock_path("ransom_extension", Severity::kCritical);
    lock_path.AddCondition(
        Condition::Equals("type", {"file_create", "file_write"}));
    lock_path.AddCondition(Condition::EndsWith("path", {".locked"}));
    lock_path.AddAction("quarantine_file");
    rules.push_back(lock_path);

    MatchRule lock_to("ransom_extension", Severity::kCritical);
    lock_to.AddCondition(Condition::Equals("type", {"file_move"}));
    lock_to.AddCondition(Condition::EndsWith("to", {".locked"}));
    lock_to.AddAction("quarantine_file");
    rules.push_back(lock_to);

    return rules;
}

}  // namespace nano_edr
