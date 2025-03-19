#include "zonedstring.h"
#include "pack.h"

#include <util/generic/algorithm.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/utility.h>

TZonedString TZonedString::Substr(size_t ofs, size_t len, const TWtringBuf& addPrefix, const TWtringBuf& addSuffix) const {
    TWtringBuf bufString(String);
    TZonedString res(TUtf16String::Join(addPrefix, bufString.substr(ofs, len), addSuffix));
    res.Zones = Zones;
    for (TZones::iterator it = res.Zones.begin(); it != res.Zones.end(); ++it) {
        TSpans& v = it->second.Spans;
        for (size_t i = 0; i != v.size(); ++i) {
            v[i].Span = TWtringBuf(res.String.data() + (~v[i] - String.data()) - ofs + addPrefix.size(), +v[i]);
        }
    }
    res.Normalize(addPrefix.size(), len);
    return res;
}

void TZonedString::Normalize() {
    Normalize(0, String.size());
}

void TZonedString::Normalize(size_t ofs, size_t len) {
    ofs = ::Min(ofs, String.size());
    len = ::Min(len, String.size() - ofs);
    for (TZones::iterator it = Zones.begin(); it != Zones.end(); ++it) {
        TSpans& v = it->second.Spans;
        size_t n = 0;
        for (size_t i = 0; i != v.size(); ++i) {
            if (~v[i] + +v[i] <= String.data() + ofs) {
                continue;
            }
            if (String.data() + ofs + len <= ~v[i]) {
                continue;
            }
            if (~v[i] + +v[i] > String.data() + ofs + len) {
                v[i].Span = TWtringBuf(~v[i], String.data() + ofs + len);
            }
            if (~v[i] < String.data() + ofs) {
                v[i].Span = TWtringBuf(String.data() + ofs, ~v[i] + +v[i]);
            }
            DoSwap(v[n], v[i]);
            ++n;
        }
        v.resize(n);
        Sort(v.begin(), v.end(), LsBuf);
    }
}
