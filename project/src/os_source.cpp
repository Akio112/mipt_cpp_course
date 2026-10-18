#include "os_source.h"

#include <stdexcept>
#include <string>

#include "agent.h"
#include "os.h"

namespace nano_edr {

OsSource::OsSource(const std::string& path) : handle_(path) {}

void OsSource::Run(Agent* agent) {
    handle_.Subscribe(Agent::Trampoline, agent);
    handle_.Start();
    os_status code = OS_TIMEOUT;
    while ((code = handle_.Wait(100)) == OS_TIMEOUT) {
    }
    if (code != OS_OK) {
        throw std::runtime_error("ошибка источника: " +
                                 std::string(os_status_str(code)));
    }
}

}  // namespace nano_edr
