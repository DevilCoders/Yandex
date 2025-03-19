#pragma once

#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/iface/archive/viewer.h>
#include <kernel/snippets/sent_match/callback.h>

#include <kernel/tarc/iface/tarcface.h>

namespace NSnippets
{
    class TQueryy;
    class TConfig;
    class TSnipTitle;
    class TUnpacker;
    class TMakeTitleOptions;

    class THeaderViewer : public IArchiveViewer {
    public:
        const int MaxSentCount;
        const int MaxTotalSentLen;
        TUnpacker* Unpacker;
        TSentsOrder All;
        TArchiveViews Result;
        THeaderViewer();
        void Reset() {
            Unpacker = nullptr;
            All.Clear();
        }
        void OnUnpacker(TUnpacker* unpacker) override;
        void FilterSpans(const TVector<TArchiveZoneSpan>& headerSpans,
                         const TVector<TArchiveZoneSpan>& forbiddenSpans,
                         TVector<TArchiveZoneSpan>& filteredSpans);
        void OnMarkup(const TArchiveMarkupZones& zones) override;
        void OnEnd() override;
    };

    bool GenerateHeaderBasedTitle(TSnipTitle& resTitle, const THeaderViewer& viewer, const TQueryy& query, const TSnipTitle& naturalTitle,
                                  const TMakeTitleOptions& options, const TConfig& config, const TString& url, ISnippetCandidateDebugHandler* candidateHandler);
}
