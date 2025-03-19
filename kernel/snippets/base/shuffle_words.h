#pragma once

#include <kernel/snippets/hits/ctx.h>

#include <util/generic/string.h>

namespace NSnippets {
    class TConfig;
    class TPassageReply;
    void ShuffleWords(const TConfig& cfg, const TString& url, const THitsInfoPtr& hitsInfo, TPassageReply& reply);
}
