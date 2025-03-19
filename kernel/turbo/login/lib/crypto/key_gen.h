#pragma once

#include <util/generic/string.h>

namespace NTurboLogin {
    TString GenerateSecret(size_t keyNum, size_t start_offset = 0, bool pretty = false);
}
