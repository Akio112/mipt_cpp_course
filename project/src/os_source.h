#ifndef NANO_EDR_OS_SOURCE_H
#define NANO_EDR_OS_SOURCE_H

#include <string>

#include "os_handle.h"
#include "source.h"

namespace nano_edr {

class OsSource : public Source {
 public:
    explicit OsSource(const std::string& path);

    void Run(Agent* agent) override;

 private:
    OsHandle handle_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_OS_SOURCE_H
