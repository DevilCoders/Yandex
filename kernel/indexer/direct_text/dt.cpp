#include "dt.h"

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/stream/output.h>

template <>
void Out<NIndexerCore::TDirectTextEntry2>(IOutputStream& out, TTypeTraits<NIndexerCore::TDirectTextEntry2>::TFuncParam e) {
    bool front = true;
    if (e.Token.IsInited()) {
        out << WideToUTF8(TWtringBuf(e.Token));
        front = false;
    }
    for (size_t s = 0; s < e.SpaceCount; ++s) {
        if (!front || !IsSpace(e.Spaces[s].Space, e.Spaces[s].Length)) {
            out << WideToUTF8(e.Spaces[s].Space, e.Spaces[s].Length);
            front = false;
        }
    }
}
