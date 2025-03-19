#pragma once

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_match/len_chooser.h>

namespace NSnippets {

    TMultiCutResult CutDescription(const TUtf16String& srcDesc, const TReplaceContext& ctx, float maxLen);
    TMultiCutResult CutDescription(const TUtf16String& srcDesc, const TReplaceContext& ctx, float maxLen, TSmartCutOptions& options);
    float GetExtSnipLen(const TConfig& cfg, const TLengthChooser& lenCfg);

}
