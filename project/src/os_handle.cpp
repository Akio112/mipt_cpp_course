#include "os_handle.h"

#include <stdexcept>

namespace nano_edr {

OsHandle::OsHandle(const std::string& path) {
    const os_status code = os_init(path.c_str(), &handle_);
    if (code != OS_OK) {
        throw std::runtime_error("не удалось открыть " + path + ": " +
                                 os_status_str(code));
    }
}

OsHandle::~OsHandle() {
    os_free(handle_);
}

void OsHandle::Subscribe(os_event_cb cb, void* ctx) {
    const os_status code = os_event_subscribe(handle_, cb, ctx);
    if (code != OS_OK) {
        throw std::runtime_error("ошибка подписки: " +
                                 std::string(os_status_str(code)));
    }
}

void OsHandle::Start() {
    const os_status code = os_start(handle_);
    if (code != OS_OK) {
        throw std::runtime_error("ошибка запуска: " +
                                 std::string(os_status_str(code)));
    }
}

os_status OsHandle::Wait(uint32_t ms) {
    return os_wait(handle_, ms);
}

os_status OsHandle::Kill(uint32_t pid, uint64_t expected_start_time) {
    return os_process_kill(handle_, pid, expected_start_time);
}

os_status OsHandle::MoveFile(const std::string& from, const std::string& to) {
    return os_file_move(handle_, from.c_str(), to.c_str());
}

os_status OsHandle::DeleteFile(const std::string& path) {
    return os_file_delete(handle_, path.c_str());
}

os_status OsHandle::ProcessInfo(uint32_t pid, os_process_info* out) {
    return os_process_get_info(handle_, pid, out);
}

void OsHandle::Stop() {
    os_stop(handle_);
}

}  // namespace nano_edr
