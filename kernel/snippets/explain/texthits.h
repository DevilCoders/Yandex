#pragma once

#include <kernel/snippets/sent_match/callback.h>
#include <util/generic/ptr.h>
#include <util/stream/output.h>

namespace NSnippets
{
    struct TSnipInfoReqParams;

    class TSnippetHitsCallback : public ISnippetsCallback {
    private:
        class TImpl;
        THolder<TImpl> Impl;

    public:
        ISnippetTextDebugHandler* GetTextHandler(bool isBylink) override;

        void GetExplanation(IOutputStream& output) const override;
        TSnippetHitsCallback(const TSnipInfoReqParams& params);
    };
}
