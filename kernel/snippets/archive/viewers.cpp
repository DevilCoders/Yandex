#include "viewers.h"

#include <kernel/snippets/archive/chooser/chooser.h>
#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/forums_handler/forums_handler.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/telephone/telephones.h>

#include <kernel/tarc/iface/tarcface.h>

#include <util/generic/algorithm.h>


namespace {

    constexpr wchar16 ATTR_DELIM = '\t';
    constexpr TWtringBuf KEY_LINKLANG = TWtringBuf(u"linklang");
    const TUtf16String RUS_LANG = ToWtring(static_cast<int>(LANG_RUS));

    static bool IsRussianSentLang(const NSnippets::TArchiveSent* sent) {
        TWtringBuf attr = sent->Attr;
        while (!attr.empty()) {
            const TWtringBuf key = attr.NextTok(ATTR_DELIM);
            const TWtringBuf value = attr.NextTok(ATTR_DELIM);
            if (key == KEY_LINKLANG && value == RUS_LANG) {
                return true;
            }

        }
        return false;
    }

    static void DropLang(const NSnippets::TConfig& config, NSnippets::TArchiveView& view) {
        if (config.GetFaceType() != ftYandexComTr || config.IsVideoExp())
            return;

        if (config.GetQueryLangMask().HasAny(TLangMask(LANG_RUS, LANG_UKR, LANG_BEL)))
            return;

        NSnippets::TArchiveView result;
        for (size_t i = 0; i < view.Size(); ++i) {
            const NSnippets::TArchiveSent* sent = view.Get(i);
            if (!IsRussianSentLang(sent)) {
                result.PushBack(sent);
            }
        }
        view.Swap(result);
    }
} // namespace

namespace NSnippets {
    TFirstAndHitSentsViewer::TFirstAndHitSentsViewer(bool isLinkArc, const NSnippets::TConfig& cfg, const NSnippets::TQueryy* query, TArchiveMarkup& markup)
      : MetadataViewer(cfg.ImgSearch())
      , IsLinkArc(isLinkArc)
      , Cfg(cfg)
      , Query(query)
      , Markup(markup)
    {
    }
    int TFirstAndHitSentsViewer::GetUnpackBeforeHit(bool link) {
        return link ? 0 : 2;
    }
    int TFirstAndHitSentsViewer::GetUnpackAfterHit(bool link) {
        return link ? 0 : 2;
    }
    int TFirstAndHitSentsViewer::GetExtsnipAdditionalSents(bool link) {
        return (!Cfg.ExpFlagOff("superextended") && !link) ? 4 : 0;
    }
    void TFirstAndHitSentsViewer::OnTelephoneMarkup(const TArchiveMarkupZones& markupZones) {
        if (!Query) {
            return;
        }
        const TArchiveZone& telephoneZone = markupZones.GetZone(AZ_TELEPHONE);
        const TArchiveZoneAttrs& telephoneAttrs = markupZones.GetZoneAttrs(AZ_TELEPHONE);

        for (TVector<TArchiveZoneSpan>::const_iterator span = telephoneZone.Spans.begin();
            span != telephoneZone.Spans.end();
            ++span)
        {
            int l = span->SentBeg;
            int r = span->SentEnd;
            if (l != r) {
                continue;
            }
            bool ok = false;
            TVector<ui16>::const_iterator li = LowerBound(HitSents.begin(), HitSents.end(), l);
            if (li != HitSents.begin() && l - *(li - 1) <= 2) {
                ok = true;
            }
            if (li != HitSents.end() && *li - l <= 2) {
                ok = true;
            }
            if (!ok) {
                continue;
            }
            const THashMap<TString, TUtf16String>* attributes = telephoneAttrs.GetSpanAttrs(*span).AttrsHash;
            if (!attributes) {
                continue;
            }
            const TPhone userPhone = TUserTelephones::AttrsToPhone(*attributes);
            if (!Query->LowerForms.contains(ASCIIToWide(userPhone.GetLocalPhone()))) {
                continue;
            }
            HitGen.PushBack(l, r);
            NearHitGen.PushBack(l - GetUnpackBeforeHit(IsLinkArc), r + GetUnpackAfterHit(IsLinkArc));
        }
    }

    void TFirstAndHitSentsViewer::OnHitsAndSegments(const TVector<ui16>& hitSents, const NSegments::TSegmentsInfo*) {
        HitSents = hitSents;
    }

    void TFirstAndHitSentsViewer::OnBeforeSents() {
        TVector<int> h(HitSents.begin(), HitSents.end());
        if (!MetadataViewer.Title.Sents.Empty() && !Cfg.ImgSearch()) {
            const int t1 = MetadataViewer.Title.Sents.Begin()->FirstId;
            const int t2 = MetadataViewer.Title.Sents.Begin()->LastId;
            int n = 0;
            for (size_t i = 0; i < h.size(); ++i) {
                if (t1 <= h[i] && h[i] <= t2) {
                    continue;
                }
                h[n++] = h[i];
            }
            h.resize(n);
        }
        for (size_t i = 0; i < h.size(); ++i) {
            HitGen.PushBack(h[i], h[i]);
            size_t from = h[i] - GetUnpackBeforeHit(IsLinkArc);
            size_t to = h[i] + GetUnpackAfterHit(IsLinkArc);
            size_t extra = GetExtsnipAdditionalSents(IsLinkArc);
            NearHitGen.PushBack(from, to);
            if (extra) {
                ExtNearHitGen.PushBack(from, to + extra);
            }
        }
        HitGen.SortAndMerge();
        HitGen.Complete(&HitOrder);
        if (!Cfg.ImgSearch()) {
            HitGen.Cutoff(MetadataViewer.Title, HitOrder);
            HitGen.Cutoff(MetadataViewer.Meta, HitOrder);
        }
        Unpacker->AddRequest(HitOrder);

        if (Cfg.UnpAll()) {
            NearHitGen.PushBack(1, 65535);
        }

        NearHitGen.SortAndMerge();
        NearHitGen.Complete(&NearHitOrder);
        if (!Cfg.ImgSearch()) {
            NearHitGen.Cutoff(MetadataViewer.Title, NearHitOrder);
            NearHitGen.Cutoff(MetadataViewer.Meta, NearHitOrder);
        }
        Unpacker->AddRequest(NearHitOrder);

        if (Cfg.ExpFlagOn("extsnip_unpack_all")) {
            ExtNearHitGen.PushBack(1, 65535);
        }

        ExtNearHitGen.SortAndMerge();
        ExtNearHitGen.Complete(&ExtNearHitOrder);
        Unpacker->AddRequest(ExtNearHitOrder);
    }

    void TFirstAndHitSentsViewer::OnEnd() {
        if (Cfg.UnpAll()) {
            DumpResult(NearHitOrder, Result);
        } else {
            if (IsLinkArc) {
                DumpResult(HitOrder, Result);
            } else {
                TSentFilter excludeFilter(100);
                if (ForumMarkupViewer && ForumMarkupViewer->FilterByMessages()) {
                    TArchiveView nearHitSents;
                    DumpResult(NearHitOrder, nearHitSents);
                    ForumMarkupViewer->FillSentExcludeFilter(excludeFilter, nearHitSents);
                }
                TChooserContext chooserCtx;
                DumpResult(GetMetadataViewer().Title, chooserCtx.Title);
                NSnippets::DumpResult(GetHitOrder(), chooserCtx.Hit);
                NSnippets::DumpResult(GetNearHitOrder(), chooserCtx.NearHit);
                ChooseArchiveSents(Cfg, chooserCtx, Markup.GetSegments(), Result, excludeFilter);
            }
        }
        if (IsLinkArc && Cfg.VideoLangHackTr()) {
            DropLang(Cfg, Result);
        }
        if (!IsLinkArc) {
            DumpResult(ExtNearHitOrder, ExtResult);
        }
    }
}
