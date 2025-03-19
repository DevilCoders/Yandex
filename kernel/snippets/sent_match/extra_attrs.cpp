#include "extra_attrs.h"

#include <library/cpp/json/writer/json.h>

#include <util/stream/str.h>

namespace NSnippets {
    TString TExtraSnipAttrs::GetPackedClickLikeSnip() const {
        if (ClickLikeSnip.empty()) {
            return TString();
        }
        NJsonWriter::TBuf w(NJsonWriter::HEM_UNSAFE);
        w.BeginObject();
        for (const auto& it : ClickLikeSnip) {
            w.WriteKey(it.first);
            w.UnsafeWriteValue(it.second);
        }
        w.EndObject();
        TStringStream out;
        w.FlushTo(&out);
        return out.Str();
    }
}
