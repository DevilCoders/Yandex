#pragma once

#include <kernel/snippets/sent_match/lang_check.h>
#include <util/generic/fwd.h>

namespace NSnippets {
    class TReplaceContext;
    struct TMultiCutResult;

    bool CheckAndAddExtension(TUtf16String& extension, size_t maxExtLen, const TReplaceContext& ctx,
        TMultiCutResult& extDesc, ELanguage lang);

}
