#include <print>
#include <string>
#include <utility>

#include "fields.h"
#include "response.h"
#include "text.h"

namespace nano_edr {
namespace {

bool SystemFile(const std::string& path) {
    return StartsAt(path, 0, "C:\\Windows\\") ||
           StartsAt(path, 0, "C:\\Program Files");
}

}

ResponsePlanner::ResponsePlanner(const EntityModel* model,
                                 std::string quarantine_dir)
    : model_(model), quarantine_dir_(std::move(quarantine_dir)) {}

std::string ResponsePlanner::QuarantinePath(const std::string& path,
                                            const std::string& pid) const {
    if (quarantine_dir_.empty()) return "";
    const std::size_t pos = path.find_last_of('\\');
    const std::string name = pos == std::string::npos ? path : path.substr(pos + 1);
    return quarantine_dir_ + "\\" + pid + "-" + name;
}

ResponsePlan ResponsePlanner::Plan(const Detection& detection) const {
    ResponsePlan plan;
    const std::vector<std::string> tree = model_->GetProcessTree(detection.pid);
    for (const std::string& name : detection.actions) {
        if (name == "kill_process") {
            for (std::size_t i = tree.size(); i > 0; --i) {
                const std::string& pid = tree[i - 1];
                Action action;
                action.kind = ActionKind::kKillProcess;
                action.pid = pid;
                action.expected_start = model_->FindProcess(pid)->start;
                action.reason = detection.rule;
                if (pid != detection.pid) {
                    action.reason += ", потомок " + detection.pid;
                }
                plan.Add(std::move(action));
            }
        } else if (name == "quarantine_source" || name == "quarantine_image") {
            const std::string* path = nullptr;
            if (name == "quarantine_source") {
                const FileRecord* source = model_->FindSpawnSource(detection.pid);
                if (source != nullptr) path = &source->path;
            } else {
                const ProcessRecord* process = model_->FindProcess(detection.pid);
                if (process != nullptr) path = &process->image;
            }
            if (path == nullptr || path->empty() || SystemFile(*path)) continue;
            Action action;
            action.kind = ActionKind::kQuarantineFile;
            action.path = *path;
            action.to = QuarantinePath(*path, detection.pid);
            if (action.to.empty()) continue;
            action.reason = detection.rule + (name == "quarantine_source"
                ? ", источник запуска" : ", образ процесса");
            plan.Add(std::move(action));
        } else if (name == "delete_dropped") {
            for (const std::string& pid : tree) {
                for (const std::string& path : model_->GetFilesCreatedBy(pid, true)) {
                    if (SystemFile(path)) continue;
                    Action action;
                    action.kind = ActionKind::kDeleteFile;
                    action.path = path;
                    action.reason = detection.rule + ", принесён " + pid;
                    plan.Add(std::move(action));
                }
            }
        } else if (name == "delete_evidence") {
            for (const auto& event : detection.evidence) {
                const std::string* path = FindField(*event,
                    event->event_type() == EventType::kFileMove ? "to" : "path");
                if (path == nullptr || SystemFile(*path)) continue;
                Action action;
                action.kind = ActionKind::kDeleteFile;
                action.path = *path;
                action.reason = detection.rule + ", файл из детекта";
                plan.Add(std::move(action));
            }
        }
    }
    return plan;
}

Responder::Responder(OsHandle* os, bool dry_run) : os_(os), dry_run_(dry_run) {}

void Responder::Execute(const ResponsePlan& plan) {
    for (const Action& action : plan.actions()) {
        ++stats_.planned;
        bool repeat = false;
        for (const std::string& target : done_) {
            if (target == action.target()) {
                repeat = true;
                break;
            }
        }
        if (repeat) {
            ++stats_.skipped_repeat;
            continue;
        }
        done_.push_back(action.target());
        if (dry_run_ || os_ == nullptr) {
            std::print("[PLAN] {}\n", ToString(action));
            continue;
        }
        switch (action.kind) {
            case ActionKind::kKillProcess:
                Kill(action);
                break;
            case ActionKind::kQuarantineFile:
                MoveToQuarantine(action);
                break;
            case ActionKind::kDeleteFile:
                Delete(action);
                break;
        }
    }
}

}
