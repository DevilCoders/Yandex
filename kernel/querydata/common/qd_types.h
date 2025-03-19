#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NQueryData {

    using TStringBufs = TVector<TStringBuf>;
    using TStringBufPair = std::pair<TStringBuf, TStringBuf>;
    using TStringBufPairs = TVector<TStringBufPair>;

    using TKeyTypes = TVector<int>;

}
