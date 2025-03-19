#include "subindex.h"

#define SUPERLONG_SIGNED_MAX ~(LL(1) << 63)

YxPerst YxPerst::NullSubIndex = GetPerst(SUPERLONG_SIGNED_MAX, 0);

TSubIndexInfo TSubIndexInfo::NullSubIndex;
