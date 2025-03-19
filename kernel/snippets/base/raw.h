#pragma once
#include <kernel/snippets/hits/ctx.h>

class TInlineHighlighter;

namespace NSnippets {

class TPassageReply;
class TConfig;
class TArcManip;

void ProcessRawPassages(TPassageReply& reply,
    const TConfig& cfg,
    const THitsInfoPtr& hitsInfo,
    const TInlineHighlighter& IH,
    TArcManip& arcCtx);

}
