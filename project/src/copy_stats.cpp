#include "copy_stats.h"

namespace nano_edr {

CopyStats& copy_stats() {

    static CopyStats stats;
    return stats;
}

void ResetCopyStats() {
    copy_stats() = CopyStats();
}

}
