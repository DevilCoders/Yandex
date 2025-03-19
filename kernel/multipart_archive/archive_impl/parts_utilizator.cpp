#include "parts_utilizator.h"

#include <library/cpp/balloc/optional/operators.h>

namespace NRTYArchive {
    void* TPartsUtilizator::CreateThreadSpecificResource() {
        ThreadDisableBalloc();
        return nullptr;
    }
}
