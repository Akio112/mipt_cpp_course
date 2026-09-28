#include "fields.h"
#include "agent_rules.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace nano_edr {
namespace {

const std::string kTempSegment = "\\appdata\\local\\temp\\";


std::string ToLowerCopy(const std::string& text) {
    std::string result = text;
    for (char& ch : result) {
        if (isupper(ch)) {
                ch = std::tolower(ch);
            }
    }
    return result;
    
}

bool EndsWith(const std::string& text, const std::string& suffix) {
    if (suffix.size() > text.size()) return false;
    return text.compare(text.size() - suffix.size(),
                        suffix.size(), suffix) == 0;
}

bool Contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

bool IsUnderTemp(const std::string& normalized_path) {
    return Contains(normalized_path, kTempSegment);
}

bool IsScriptHostImage(const std::string& image_path) {
    const std::string normalized = NormalizePath(image_path);
    return EndsWith(normalized, "\\wscript.exe") ||
           EndsWith(normalized, "\\cscript.exe");
}

bool IsOfficeImage(const std::string& image_path) {
    const std::string normalized = NormalizePath(image_path);
    return EndsWith(normalized, "\\winword.exe")  ||
           EndsWith(normalized, "\\excel.exe")    ||
           EndsWith(normalized, "\\powerpnt.exe") ||
           EndsWith(normalized, "\\outlook.exe");
}

bool IsShellOrScriptHost(const std::string& image_path) {
    const std::string normalized = NormalizePath(image_path);
    return EndsWith(normalized, "\\wscript.exe")    ||
           EndsWith(normalized, "\\cscript.exe")    ||
           EndsWith(normalized, "\\powershell.exe") ||
           EndsWith(normalized, "\\pwsh.exe")       ||
           EndsWith(normalized, "\\cmd.exe")        ||
           EndsWith(normalized, "\\mshta.exe")      ||
           EndsWith(normalized, "\\rundll32.exe")   ||
           EndsWith(normalized, "\\regsvr32.exe");
}


bool IsScriptHostFromTemp(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }

    const std::string* image = FindField(event, "image");
    if (image == nullptr || !IsScriptHostImage(*image)) { 
        return false;
    }

    return IsUnderTemp(NormalizePath(*image));
}

bool IsScriptHostUrlCmdline(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }

    const std::string* image = FindField(event, "image");
    if (image == nullptr) {
        return false;
    }

    if (!IsScriptHostImage(*image)) {
        return false;
    }

    const std::string* cmdline = FindField(event, "cmdline");
    if (cmdline == nullptr) {
        return false;
    }

    const std::string lower = ToLowerCopy(*cmdline);
    return Contains(lower, "http://")  ||
           Contains(lower, "https://") ||
           Contains(lower, "ftp://");
}


bool IsOfficeSpawnsScriptHost(const Event& event) {
    if (!IsProcessStart(event)){
        return false;
    }

    const std::string* image = FindField(event, "image");
    if (image == nullptr || !IsShellOrScriptHost(*image)){
        return false;
    }

    const std::string* parent = FindField(event, "parent_image");
    if (parent == nullptr) {
        parent = FindField(event, "ppid_image");
        if (parent == nullptr) {
            return false;
        }
    }
    

    return IsOfficeImage(*parent);
}

bool IsPowerShellEncoded(const Event& event) {
    if (!IsProcessStart(event)) return false;

    const std::string* image = FindField(event, "image");
    if (image == nullptr) {
        return false;
    }

    const std::string normalized = NormalizePath(*image);
    if (!EndsWith(normalized, "\\powershell.exe") &&
        !EndsWith(normalized, "\\pwsh.exe")) {
        return false;
    }

    const std::string* cmdline = FindField(event, "cmdline");
    if (cmdline == nullptr) return false;

    const std::string lower = ToLowerCopy(*cmdline);
    return Contains(lower, "-encodedcommand") ||
           Contains(lower, "frombase64string");
}


bool IsStartupPersistenceWrite(const Event& event) {
    if (!IsFileWrite(event)) return false;

    const std::string* path = FindField(event, "path");
    if (path == nullptr) {
        return false;
    }

    const std::string normalized = NormalizePath(*path);
    if (!Contains(normalized, "\\startup\\")) {
        return false;
    }

    return EndsWith(normalized, ".js")  ||
           EndsWith(normalized, ".vbs") ||
           EndsWith(normalized, ".ps1") ||
           EndsWith(normalized, ".bat") ||
           EndsWith(normalized, ".exe") ||
           EndsWith(normalized, ".lnk");
}

}  

const Rule kAgentRules[] = {
    {"script_host_from_temp",      IsScriptHostFromTemp,      Severity::kHigh},
    {"script_host_url_cmdline",    IsScriptHostUrlCmdline,    Severity::kCritical},
    {"office_spawns_script_host",  IsOfficeSpawnsScriptHost,  Severity::kHigh},
    {"powershell_encoded_command", IsPowerShellEncoded,       Severity::kHigh},
    {"startup_persistence_write",  IsStartupPersistenceWrite, Severity::kMedium},
};

const std::size_t kAgentRuleCount =
    sizeof(kAgentRules) / sizeof(kAgentRules[0]);

}  