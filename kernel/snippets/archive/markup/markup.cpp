#include "markup.h"

#include <kernel/snippets/iface/archive/segments.h>

#include <kernel/snippets/hits/filter.h>

#include <kernel/tarc/markup_zones/text_markup.h>

#include <util/generic/singleton.h>

namespace NSnippets {

    TArchiveMarkup::TArchiveMarkup() {
    }
    TArchiveMarkup::~TArchiveMarkup() {
    }

    const NSegments::TSegmentsInfo* TArchiveMarkup::GetSegments() const {
        return Segments.Get();
    }
    void TArchiveMarkup::ResetSegments(NSegments::TSegmentsInfo* segments) {
        Segments.Reset(segments);
    }

    const TArchiveMarkupZones& TArchiveMarkup::GetArcMarkup(EARC arc) const {
        if (Arc[+arc].Markup.Get()) {
            return *Arc[+arc].Markup.Get();
        }
        return Default<TArchiveMarkupZones>();
    }
    void TArchiveMarkup::SetArcMarkup(EARC arc, const void* markupInfo, size_t markupInfoLen) {
        if (Arc[+arc].Markup.Get()) {
            Arc[+arc].Markup.Reset(nullptr);
        }
        Arc[+arc].Markup.Reset(new TArchiveMarkupZones());
        TArchiveMarkupZones& mZones = *Arc[+arc].Markup.Get();
        UnpackMarkupZones(markupInfo, markupInfoLen, &mZones);
    }

    void TArchiveMarkup::SetHitsInfo(const THitsInfoPtr& hits) {
        Hits = hits;
    }
    const TVector<ui16>& TArchiveMarkup::GetFilteredHits(EARC arc) const {
        return Arc[+arc].FilteredHits;
    }

    class TBadSegChecker : public ISentChecker
    {
    private:
        TBitSet GForms;
        NSegments::TSegmentsInfo& SegInfo;
    public:
        TBadSegChecker(NSegments::TSegmentsInfo& segInfo,
                const TVector<ui16>& sents, const TVector<ui16>& masks)
            : GForms()
            , SegInfo(segInfo)
        {
            for (size_t i = 0; i < sents.size(); ++i) {
                if (GetSegBadness(sents[i]) < 2) {
                    TBitSet forms(masks[i]);
                    for (const size_t& form = forms.Begin(); !forms.End(); forms.Next()) {
                        GForms.Insert(form);
                    }
                }
            }
        }

        int GetSegBadness(ui16 sent) {
            using namespace NSegm;
            switch (SegInfo.GetType(SegInfo.GetArchiveSegment(sent))) {
                case STP_MENU:
                case STP_FOOTER:
                    return 2;
                default:
                    return 0;
            }
        }

        bool IsBad(ui16 sent, const TBitSet& uforms) override
        {
            return ((uforms.GetBits() & GForms.GetBits()) == uforms.GetBits())
                && (GetSegBadness(sent) >= 2);
        }
    };

    void TArchiveMarkup::FilterHits(EARC arc, ui32 hitsTopLen, bool allHits) {
        if (!Hits.Get())
            return;

        if (ARC_LINK == arc) {
            Arc[+arc].FilteredHits = Hits->LinkHits;
            return;
        }

        if (ARC_TEXT == arc) {
            const TVector<ui16>& sents = Hits->THSents;
            const TVector<ui16>& masks = Hits->THMasks;
            if (masks.empty() && sents.empty() && !Hits->TextSentsPlain.empty()) {
                Arc[+arc].FilteredHits = Hits->TextSentsPlain;
                if (!Hits->MoreTextSentsPlain.empty()) {
                    TVector<ui16>& v = Arc[+arc].FilteredHits;
                    v.insert(v.end(), Hits->MoreTextSentsPlain.begin(), Hits->MoreTextSentsPlain.end());
                    Sort(v.begin(), v.end());
                    v.erase(Unique(v.begin(), v.end()), v.end());
                }
                return;
            }
            if (sents.size() != masks.size()) {
                allHits = true;
            }
            TMultiTopFilter flt(Arc[+arc].FilteredHits, Hits->FormWeights, hitsTopLen, allHits);
            if (!allHits && Segments.Get())
                flt.SetChecker(new TBadSegChecker(*Segments.Get(), sents, masks));
            for (size_t i = 0; i < sents.size(); ++i) {
                TBitSet uforms;
                if (!allHits) {
                    uforms = masks[i];
                }
                flt.ProcessSent(sents[i], uforms);
            }
            flt.Finilize();
        }
    }
}
