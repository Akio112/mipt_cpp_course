#include "agent_rules.h"

#include <cstddef>
#include <string>

#include "fields.h"

namespace nano_edr {
namespace {

bool EndsWith(const std::string& text, const std::string& suffix) {
    if (suffix.size() > text.size()) return false;
    return text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool Contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

bool HasImage(const std::string& image,
              std::initializer_list<std::string> names) {
    const std::string path = NormalizePath(image);

    for (const auto& name : names) {
        if (EndsWith(path, name))
            return true;
    }

    return false;
}

bool FileEvent(const Event& event) {
    return event.type == "file_create" || event.type == "file_write" ||
           event.type == "file_move";
}

const std::string* EventPath(const Event& event) {
    if (event.type == "file_move") return FindField(event, "to");
    return FindField(event, "path");
}

bool ScriptTemp(const Event& event) {
    if (!IsProcessStart(event)) return false;

    const std::string& image = GetRequiredField(event, "image");
    if (!HasImage(image, {"\\wscript.exe", "\\cscript.exe"})) return false;

    const std::string* cmd = FindField(event, "cmdline");
    if (cmd == nullptr) return false;
    const std::string text = NormalizePath(*cmd);
    return Contains(text, "\\appdata\\local\\temp\\") ||
           Contains(text, "\\windows\\temp\\");
}

bool Lolbin(const Event& event) {
    if (!IsProcessStart(event)) return false;

    const std::string& image = GetRequiredField(event, "image");
    if (!HasImage(image, {"\\certutil.exe", "\\bitsadmin.exe"})) return false;

    const std::string* cmd = FindField(event, "cmdline");
    if (cmd == nullptr) return false;
    const std::string text = NormalizePath(*cmd);
    return Contains(text, "urlcache") || Contains(text, "transfer") ||
           Contains(text, "http:") || Contains(text, "https:");
}

bool HiddenPs(const Event& event) {
    if (!IsProcessStart(event)) return false;

    const std::string& image = GetRequiredField(event, "image");
    if (!HasImage(image, {"\\powershell.exe", "\\pwsh.exe"})) return false;

    const std::string* cmd = FindField(event, "cmdline");
    if (cmd == nullptr) return false;
    const std::string command = NormalizePath(*cmd);
    return Contains(command, "-w hidden") ||
           Contains(command, "-windowstyle hidden") || Contains(command, "-enc") ||
           Contains(command, "-encodedcommand");
}

bool Autostart(const Event& event) {
    if (!FileEvent(event)) return false;
    const std::string* path = EventPath(event);
    if (path == nullptr) return false;
    return Contains(NormalizePath(*path),
                    "\\start menu\\programs\\startup\\");
}

bool RansomExt(const Event& event) {
    if (!FileEvent(event)) return false;
    const std::string* path = EventPath(event);
    if (path == nullptr) return false;
    return EndsWith(NormalizePath(*path), ".locked");
}

constexpr Rule kRules[] = {
    {"script_host_from_temp", ScriptTemp, Severity::kHigh},
    {"lolbin_download", Lolbin, Severity::kHigh},
    {"hidden_powershell", HiddenPs, Severity::kMedium},
    {"autostart_write", Autostart, Severity::kHigh},
    {"ransom_extension", RansomExt, Severity::kCritical},
};

}  // namespace
const Rule* AgentRules() {
    return kRules;
}

size_t AgentRuleCount() {
    return sizeof(kRules) / sizeof(kRules[0]);
}

}  // namespace nano_edr