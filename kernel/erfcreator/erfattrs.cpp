#include "erfattrs.h"
#include <kernel/indexer/faceproc/docattrs.h>
#include <util/stream/mem.h>

void UnpackErfAttrs(const void* data, size_t sz, TErfAttrs& erfAttrs) {
    TMemoryInput in(data, sz);
    UnpackFrom(&in, &erfAttrs);
}

