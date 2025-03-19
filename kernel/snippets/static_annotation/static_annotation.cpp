#include "static_annotation.h"

#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/config/enums.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/custom/trash_classifier/trash_classifier.h>
#include <kernel/snippets/iface/archive/segments.h>
#include <kernel/snippets/iface/archive/viewer.h>
#include <kernel/snippets/sent_info/beautify.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/glue.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/title_trigram/title_trigram.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <kernel/tarc/markup_zones/unpackers.h>

#include <util/generic/hash_set.h>

namespace NSnippets
{
    class TStatAnnotViewer::TImpl : public IArchiveViewer {
    private:
        static const int MAX_UNPACK_SENTS = 100;

        const TArchiveMarkup& Markup;
        TUnpacker* Unpacker;
        TSentsOrder All;
        TArchiveView Result;
        TStatAnnotMode StatAnnotMode;
        EStatAnnotType StatAnnotType;

    public:
        TImpl(const TConfig& cfg, const TArchiveMarkup& markup)
          : Markup(markup)
          , Unpacker(nullptr)
          , StatAnnotMode(static_cast<EStaticAnnotationMode>(cfg.GetStaticAnnotationMode()))
          , StatAnnotType(SAT_UNKNOWN)
        {
        }
        void OnUnpacker(TUnpacker* unpacker) override {
            Unpacker = unpacker;
        }
        typedef THashSet<int> TUsefulSentNumHash;
        TUsefulSentNumHash GetGoodSegmentsSentNumbers(const TArchiveMarkupZones& zones) {
            const EArchiveZone USEFUL_ARCHIVE_ZONES[] = {AZ_SEGCONTENT, AZ_SEGHEAD};
            const ui32 USEFUL_ARCHIVE_ZONES_COUNT = sizeof(USEFUL_ARCHIVE_ZONES) / sizeof(EArchiveZone);
            TUsefulSentNumHash sentences;
            for (size_t i = 0; i < USEFUL_ARCHIVE_ZONES_COUNT; ++i) {
                const TArchiveZone& z = zones.GetZone(USEFUL_ARCHIVE_ZONES[i]);
                for (TVector<TArchiveZoneSpan>::const_iterator span = z.Spans.begin(); span != z.Spans.end(); ++span) {
                    for (ui16 n = span->SentBeg; n <= span->SentEnd; ++n) {
                        sentences.insert(n);
                    }
                }
            }
            return sentences;
        }
        bool NoSegmentInfo() {
            const NSegments::TSegmentsInfo* segmInfo = Markup.GetSegments();
            return !segmInfo || !segmInfo->HasData();
        }
        void Forbid(THashSet<ui16>& forbidenZones, const TArchiveMarkupZones& zones,
            EArchiveZone zoneName) {
                const TArchiveZone& z = zones.GetZone(zoneName);
                for (TVector<TArchiveZoneSpan>::const_iterator span = z.Spans.begin();
                    span != z.Spans.end(); ++span) {
                    for (ui16 sentId = span->SentBeg; sentId <= span->SentEnd; ++sentId) {
                        forbidenZones.insert(sentId);
                    }
                }
        }
        void OnMarkup(const TArchiveMarkupZones& zones) override {
            if (StatAnnotMode == SAM_DISABLED) {
                return;
            }
            TUsefulSentNumHash goodSegmentsNumbers = GetGoodSegmentsSentNumbers(zones);
            THashSet<ui16> titleZones;
            Forbid(titleZones, zones, AZ_TITLE);
            const TArchiveZone& mainContentZone = zones.GetZone(AZ_MAIN_CONTENT);
            int nSents = 0;
            if (!mainContentZone.Spans.empty()) {
                for (TVector<TArchiveZoneSpan>::const_iterator span = mainContentZone.Spans.begin(); span != mainContentZone.Spans.end(); ++span) {
                    for (ui16 n = span->SentBeg; n <= span->SentEnd; ++n) {
                        if (nSents < MAX_UNPACK_SENTS && goodSegmentsNumbers.find(n) != goodSegmentsNumbers.end() && titleZones.find(n) == titleZones.end()) {
                            All.PushBack(n, n);
                            ++nSents;
                        }
                    }
                }
            }

            if (!All.Empty()) {
                Unpacker->AddRequest(All);
                StatAnnotType = SAT_MAIN_CONTENT;
                return;
            }

            const bool canUseContent = StatAnnotMode & SAM_CAN_USE_CONTENT;
            if (canUseContent && zones.GetZone(AZ_SEGCONTENT).Spans.size()) {
                int beg = zones.GetZone(AZ_SEGCONTENT).Spans[0].SentBeg;
                int end = zones.GetZone(AZ_SEGCONTENT).Spans[0].SentEnd;
                if (end - beg + 1 > MAX_UNPACK_SENTS) {
                    end = beg + MAX_UNPACK_SENTS - 1;
                }
                for (int n = beg; n <= end; ++n) {
                    if (titleZones.find(n) == titleZones.end()) {
                        All.PushBack(n, n);
                    }
                }
                Unpacker->AddRequest(All);
                StatAnnotType = SAT_FIRST_CONTENT;
                return;
            }

            const bool canUseReferat = StatAnnotMode & SAM_CAN_USE_REFERAT;
            if (canUseReferat && zones.GetZone(AZ_SEGREFERAT).Spans.size()) {
                int beg = zones.GetZone(AZ_SEGREFERAT).Spans[0].SentBeg;
                int end = zones.GetZone(AZ_SEGREFERAT).Spans[0].SentEnd;
                if (end - beg + 1 > MAX_UNPACK_SENTS) {
                    end = beg + MAX_UNPACK_SENTS - 1;
                }
                for (int n = beg; n <= end; ++n) {
                    if (titleZones.find(n) == titleZones.end()) {
                        All.PushBack(n, n);
                    }
                }
                Unpacker->AddRequest(All);
                StatAnnotType = SAT_FIRST_REFERAT;
                return;
            }

            // Fix for custom searches: if there's no segment info we unpack first 10 sentences...
              // and build statannot, otherwise we'll get empty body.
            const bool canUseDocStart = StatAnnotMode & SAM_DOC_START;
            if (canUseDocStart && NoSegmentInfo() && All.Empty()) {
                THashSet<ui16> forbidenZones;
                Forbid(forbidenZones, zones, AZ_TITLE);
                Forbid(forbidenZones, zones, AZ_ABSTRACT);
                // unpack 10 sents tops
                for (int i = 1, sentCnt = 0; i < 100 && sentCnt < 10; ++i) {
                    if (!forbidenZones.contains(i)) {
                        All.PushBack(i, i);
                        ++sentCnt;
                    }
                }
                Unpacker->AddRequest(All);
                StatAnnotType = SAT_DOC_START_IF_NO_SEGMENTS;
                return;
            }
        }
        void OnEnd() override {
            DumpResult(All, Result);
        }
        const TArchiveView& GetResult() const {
            return Result;
        }
        EStatAnnotType GetResultType() const {
            return StatAnnotType;
        }
    };

    TStatAnnotViewer::TStatAnnotViewer(const TConfig& cfg, const TArchiveMarkup& markup)
      : Impl(new TImpl(cfg, markup))
    {
    }
    TStatAnnotViewer::~TStatAnnotViewer()
    {
    }
    IArchiveViewer& TStatAnnotViewer::GetViewer() {
        return *Impl.Get();
    }
    const TArchiveView& TStatAnnotViewer::GetResult() const {
        return Impl->GetResult();
    }
    EStatAnnotType TStatAnnotViewer::GetResultType() const {
        return Impl->GetResultType();
    }

    class TStaticAnnotation::TImpl {
    private:
        static const int MAX_ANNOTATION_SIZE = 500;

        const TConfig& Cfg;
        const TArchiveMarkup& Markup;
        TUtf16String Specsnip;
        EStatAnnotType StatAnnotType;
    public:
        TImpl(const TConfig& cfg, const TArchiveMarkup& markup)
          : Cfg(cfg)
          , Markup(markup)
          , Specsnip()
          , StatAnnotType(SAT_UNKNOWN)
        {
        }

        void InitFromSentenceViewer(const TStatAnnotViewer& sentenceViewer, const TSnipTitle& title, const TQueryy& query);
        TUtf16String GetTextCopy() const {
            return Specsnip;
        }
        EStatAnnotType GetStatAnnotType() const {
            return StatAnnotType;
        }
    };

    TStaticAnnotation::TStaticAnnotation(const TConfig& cfg, const TArchiveMarkup& markup)
      : Impl(new TImpl(cfg, markup))
    {
    }
    TStaticAnnotation::~TStaticAnnotation()
    {
    }
    void TStaticAnnotation::InitFromSentenceViewer(const TStatAnnotViewer& sentenceViewer, const TSnipTitle& title, const TQueryy& query) {
        Impl->InitFromSentenceViewer(sentenceViewer, title, query);
    }
    TUtf16String TStaticAnnotation::GetSpecsnippet() const {
        return Impl->GetTextCopy();
    }
    EStatAnnotType TStaticAnnotation::GetStatAnnotType() const {
        return Impl->GetStatAnnotType();
    }

    static TArchiveView GetNonTrash(const TArchiveView& view) {
        TArchiveView res;
        for (size_t i = 0; i < view.Size(); ++i) {
            if (view.Get(i)->Sent.size() <= 40 || NTrashClassifier::IsGoodEnough(view.Get(i)->Sent)) {
                res.PushBack(view.Get(i));
            }
        }
        return res;
    }

    void TStaticAnnotation::TImpl::InitFromSentenceViewer(const TStatAnnotViewer& sentenceViewer, const TSnipTitle& title, const TQueryy& query)
    {
        StatAnnotType = sentenceViewer.GetResultType();
        if (static_cast<EStaticAnnotationMode>(Cfg.GetStaticAnnotationMode()) == SAM_DISABLED) {
            return;
        }

        const TArchiveView& view = sentenceViewer.GetResult();
        int maxLen = MAX_ANNOTATION_SIZE;
        TArchiveView fview = GetFirstSentences(Cfg.RTYSnippets() ? GetNonTrash(view) : view, maxLen);

        if (fview.Empty()) {
            return;
        }

        TRetainedSentsMatchInfo customSents;
        customSents.SetView(&Markup, fview, TRetainedSentsMatchInfo::TParams(Cfg, query).SetPutDot().SetParaTables());
        const TSentsInfo& sentsInfo = *customSents.GetSentsInfo();
        const TSentsMatchInfo& sentsMatchInfo = *customSents.GetSentsMatchInfo();
        TTitleMatchInfo titleMatchInfo;
        titleMatchInfo.Fill(sentsMatchInfo, &title);

        int i = 0;
        for (; i < sentsInfo.SentencesCount(); ++i) {
            int w0 = sentsInfo.FirstWordIdInSent(i);
            int w1 = sentsInfo.LastWordIdInSent(i);
            if (titleMatchInfo.GetTitleLikeness(w0, w1) < 0.2) {
                break;
            }
        }
        if (i < sentsInfo.SentencesCount()) {
            Specsnip = sentsInfo.GetTextWithEllipsis(
                sentsInfo.FirstWordIdInSent(i), sentsInfo.WordCount() - 1);
        }
    }
}
