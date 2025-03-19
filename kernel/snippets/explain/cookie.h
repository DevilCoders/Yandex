#pragma once

#include <kernel/snippets/sent_match/callback.h>
#include <kernel/info_request/inforequestformatter.h>
#include <util/stream/output.h>

namespace NSnippets
{
    struct TSnipInfoReqParams;

    class TCookieCallback : public ISnippetsCallback
    {
    private:
        class TImpl;
        THolder<TImpl> Impl;

    public:
        ISnippetCandidateDebugHandler* GetCandidateHandler() override;
        ISnippetDebugOutputHandler* GetDebugOutput() override;
        void OnPassageReply(const TPassageReply& reply, const TEnhanceSnippetConfig&) override;
        void OnDocInfos(const TDocInfos& docInfos) override;
        void OnMarkers(const TMarkersMask& markers) override;

    public:
        TCookieCallback(const TSnipInfoReqParams& params);
        double GetShareOfTrashCandidates(bool isByLink) const override;
        void GetExplanation(IOutputStream& output) const override;
        void OnBestFinal(const TSnip& snip, bool isByLink) override;
    };
}
