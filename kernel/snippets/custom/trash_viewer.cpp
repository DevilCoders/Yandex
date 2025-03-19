#include "trash_viewer.h"

#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/iface/archive/segments.h>
#include <kernel/snippets/iface/archive/viewer.h>

#include <kernel/tarc/iface/tarcface.h>

namespace NSnippets {
    static const int MAX_UNPACK_SENTS = 100;

    class TTrashViewer::TImpl : public IArchiveViewer {
    private:
        const TArchiveMarkup& Markup;
        TUnpacker* Unpacker;
        TSentsOrder All;
        TArchiveView Result;

    public:
        TImpl(const TArchiveMarkup& markup)
            : Markup(markup)
            , Unpacker(nullptr)
        {
        }
        void OnUnpacker(TUnpacker* unpacker) override {
            Unpacker = unpacker;
        }
        bool OnTrashMarkup(const TArchiveMarkupZones& zones,
            const EArchiveZone* archiveZones, size_t zoneCnt) {
            const NSegments::TSegmentsInfo* segmInfo = Markup.GetSegments();
            if (!segmInfo || !segmInfo->HasData()) {
                return false;
            }

            const TArchiveZoneSpan* bestSpan = nullptr;
            float bestWeight = 0.f;
            for (size_t i = 0; i < zoneCnt; ++i) {
                const EArchiveZone zoneName = archiveZones[i];
                if (zones.GetZone(zoneName).Spans.empty()) {
                    continue;
                }
                for (size_t j = 0; j < zones.GetZone(zoneName).Spans.size(); ++j) {
                    int beg = zones.GetZone(zoneName).Spans[j].SentBeg;
                    const NSegments::TSegmentCIt segm = segmInfo->GetArchiveSegment(beg);
                    if (segmInfo->IsValid(segm)) {
                        if (bestSpan == nullptr || bestWeight < segm->Weight) {
                            bestWeight = segm->Weight;
                            bestSpan = &zones.GetZone(zoneName).Spans[j];
                        }
                    }
                }
            }

            if (bestSpan != nullptr) {
                int beg = bestSpan->SentBeg;
                int end = bestSpan->SentEnd;
                int cnt = 0;
                THashSet<int> azTitle;
                const TArchiveZone& z = zones.GetZone(AZ_TITLE);
                for (TVector<TArchiveZoneSpan>::const_iterator span = z.Spans.begin(); span != z.Spans.end(); ++span) {
                    for (ui16 sentId = span->SentBeg; sentId <= span->SentEnd; ++sentId) {
                        azTitle.insert(sentId);
                    }
                }
                for (int i = beg; i <= end && cnt < MAX_UNPACK_SENTS; ++i) {
                    if (azTitle.find(i) != azTitle.end()) {
                        continue;
                    }
                    All.PushBack(i, i);
                    cnt++;
                }
                if (cnt > 0) {
                    Unpacker->AddRequest(All);
                    return true;
                }
            }
            return false;
        }
        void OnMarkup(const TArchiveMarkupZones& zones) override {
            {
                const EArchiveZone SEMI_TRASH_ARCHIVE_ZONES[] = {
                    AZ_SEGHEAD,
                    AZ_SEGMENU,
                };
                if (OnTrashMarkup(zones, SEMI_TRASH_ARCHIVE_ZONES,
                        Y_ARRAY_SIZE(SEMI_TRASH_ARCHIVE_ZONES))) {
                    return;
                }
            }
            {
                const EArchiveZone TOTAL_TRASH_ARCHIVE_ZONES[] = {
                    AZ_SEGAUX,
                    AZ_SEGCOPYRIGHT,
                };
                OnTrashMarkup(zones, TOTAL_TRASH_ARCHIVE_ZONES,
                    Y_ARRAY_SIZE(TOTAL_TRASH_ARCHIVE_ZONES));
            }
        }
        void OnEnd() override {
            DumpResult(All, Result);
        }
        const TArchiveView& GetResult() const {
            return Result;
        }
    };

    TTrashViewer::TTrashViewer(const TArchiveMarkup& markup)
        : Impl(new TImpl(markup))
    {
    }
    TTrashViewer::~TTrashViewer()
    {
    }
    const TArchiveView& TTrashViewer::GetResult() const {
        return Impl->GetResult();
    }
    IArchiveViewer& TTrashViewer::GetViewer() {
        return *Impl.Get();
    }
}
