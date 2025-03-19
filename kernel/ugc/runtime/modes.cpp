#include "modes.h"

namespace NUgc {
    bool IsDeploymentMode(const TString& mode) {
        return (mode == PROD || mode == PRE || mode == TEST || mode == DEV);
    }
} // namespace NUgc
