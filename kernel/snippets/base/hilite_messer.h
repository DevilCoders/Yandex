#pragma once

/*
   The whole idea of this file is totally abuse! It was added for experiments only.
   Will be deleted as soon as. For more details visit SNIPPETS-2174
 */

#include <kernel/snippets/hits/ctx.h>
#include <util/generic/string.h>

namespace NSnippets {
    class TConfig;
    class TQueryy;
    class TPassageReply;
    void MessWithReply(const TConfig& cfg, const TQueryy& queryCtx, const TString& url,
        const THitsInfoPtr& hitsInfo, TPassageReply& reply);
}
