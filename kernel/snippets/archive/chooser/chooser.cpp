#include "chooser.h"

#include <kernel/snippets/iface/archive/segments.h>
#include <kernel/snippets/config/config.h>

#include <util/string/split.h>

namespace NSnippets {
    struct TChooser {
        const TConfig& Cfg;
        const TArchiveView& Hit;
        const TArchiveView& Unp;
        const TArchiveView& Title;
        const NSegments::TSegmentsInfo* const Seg = nullptr;
        float ContentThreshold = -2.0;
        const TSentFilter& SentExcludeFilter;

        TChooser(const TConfig& cfg, const NSegments::TSegmentsInfo* seg, const TArchiveView& hit, const TArchiveView& unp,
                 const TArchiveView& title, const TSentFilter& excludeFilter)
          : Cfg(cfg)
          , Hit(hit)
          , Unp(unp)
          , Title(title)
          , Seg(seg)
          , SentExcludeFilter(excludeFilter)
        {
            if (Seg == nullptr)
                return;
            using namespace NSegm;

            float curMinValue = std::numeric_limits<float>::max();
            for (NSegments::TSegmentCIt it = Seg->SegmentsBegin(); it != Seg->SegmentsEnd(); ++it) {
                if (ESegmentType(it->Type) == STP_CONTENT)
                    curMinValue = Min(curMinValue, it->Weight);
            }
            if (curMinValue != std::numeric_limits<float>::max())
                ContentThreshold = Max(ContentThreshold, curMinValue);
        }
        size_t GetLimitGrow() const {
            return 300;
        }
        int GetHandleBeforeHit() const {
            return 2;
        }
        int GetHandleAfterHit() const {
            return 2;
        }
        void ChooseBySents(TArchiveView& res);
        void Choose(TArchiveView& res) {
            ChooseBySents(res);
        }
        int SegBadness(const NSegments::TSegmentCIt& s) const {
            using namespace NSegm;
            if (!Seg->IsValid(s))
                return 0;

            if (s->Weight >= ContentThreshold) {
                return 0;
            }
            return 1;
        }
        int SegBadness(const TArchiveSent* sent) const {
            if (!Seg) {
                return -1;
            }
            return SegBadness(Seg->GetArchiveSegment(*sent));
        }
    };

    void ChooseArchiveSents(const TConfig& cfg, const TChooserContext& ctx, const NSegments::TSegmentsInfo* seg,
                            TArchiveView& res, const TSentFilter& excludeFilter)
    {
        TChooser c(cfg, seg, ctx.Hit, ctx.NearHit, ctx.Title, excludeFilter);
        c.Choose(res);
    }

    void TChooser::ChooseBySents(TArchiveView& res) {
        int iu = 0;
        TArchiveView hit;
        int bHit = -1;
        for (int t = 0; t < 2; ++t)
        for (size_t i = 0; i < Hit.Size(); ++i) {
            int b = SegBadness(Hit.Get(i));
            if (!t) {
                if (bHit < 0 || b < bHit) {
                    bHit = b;
                }
            } else {
                if (bHit == 2 || b <= bHit || b <= 1) {
                    hit.PushBack(Hit.Get(i));
                }
            }
        }
        TVector<bool> r(Unp.Size(), false);
        iu = 0;
        for (int ih = 0; iu < (int)Unp.Size() && ih < (int)hit.Size(); ++iu) {
            while (ih < (int)hit.Size() && hit.Get(ih)->SentId < Unp.Get(iu)->SentId) {
                ++ih;
            }
            if (ih >= (int)hit.Size() || hit.Get(ih)->SentId != Unp.Get(iu)->SentId) {
                continue;
            }
            int b = SegBadness(hit.Get(ih));
            r[iu] = true;
            int dl = 10;
            int dr = 10;
            for (int t = 0; t < 2; ++t) {
                int l = iu;
                size_t szL = Unp.Get(iu)->Sent.size();
                while (l - 1 >= 0 && Unp.Get(l - 1)->SentId >= Unp.Get(iu)->SentId - GetHandleBeforeHit()) {
                    if (t == 1 && szL > GetLimitGrow()) {
                        break;
                    }
                    --l;
                    if (t == 0 && SegBadness(Unp.Get(l)) < 1 && dl == 10) {
                        dl = iu - l;
                    }
                    if (t == 1 && (b < 1 || dl < dr) && SegBadness(Unp.Get(l)) <= b) {
                        r[l] = true;
                    }
                    szL += Unp.Get(l)->Sent.size();
                }
                int rr = iu;
                size_t szR = Unp.Get(iu)->Sent.size();
                while (rr + 1 < (int)Unp.Size() && Unp.Get(rr + 1)->SentId <= Unp.Get(iu)->SentId + GetHandleAfterHit()) {
                    if (t == 1 && szR > GetLimitGrow()) {
                        break;
                    }
                    ++rr;
                    if (t == 0 && SegBadness(Unp.Get(rr)) < 1 && dr == 10) {
                        dr = rr - iu;
                    }
                    if (t == 1 && (b < 1 || dl >= dr) && SegBadness(Unp.Get(rr)) <= b) {
                        r[rr] = true;
                    }
                    szR += Unp.Get(rr)->Sent.size();
                }
            }
        }
        if (Cfg.RequireSentAttrs()) {
            for (size_t i = 0; i < Unp.Size(); ++i) {
                if (!Unp.Get(i)->Attr) {
                    r[i] = false;
                }
            }
        }
        for (int i = 0; i < (int)Unp.Size(); ++i) {
            const TArchiveView::TSentPtr sent = Unp.Get(i);
            if (r[i] && !SentExcludeFilter.IsMarked(sent->SentId)) {
                res.PushBack(sent);
            }
        }
    }

}
