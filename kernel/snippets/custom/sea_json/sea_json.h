#pragma once

#include <util/generic/string.h>

namespace NSnippets {
    namespace NProto {
        class TRawPreview;
    }

    TString MakeSeaJson(const NProto::TRawPreview& rawPreview);
}
