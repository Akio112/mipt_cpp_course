#ifndef NANO_EDR_FILE_SOURCE_H
#define NANO_EDR_FILE_SOURCE_H

#include <fstream>
#include <string>

#include "source.h"

namespace nano_edr {

class FileSource : public Source {
 public:
    explicit FileSource(const std::string& path);

    void Run(Agent* agent) override;

 private:
    std::ifstream log_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_FILE_SOURCE_H
