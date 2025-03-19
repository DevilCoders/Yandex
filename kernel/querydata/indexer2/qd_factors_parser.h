#pragma once

#include <util/generic/strbuf.h>

namespace NQueryData {

    class TSourceFactors;

    void FillSourceFactors(TSourceFactors&, TStringBuf value);
}
