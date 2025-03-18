#pragma once

#include <util/generic/buffer.h>

namespace NHtml {
    void NormalizeUtfInput(TBuffer* buf, bool replaceNulls);
}
