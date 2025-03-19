#include "preview_viewer.h"


#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/view/storage.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/forums_handler/forums_handler.h>
#include <kernel/snippets/iface/archive/viewer.h>
#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/snippets/schemaorg/proto/schemaorg.pb.h>
#include <kernel/snippets/schemaorg/schemaorg_serializer.h>

#include <kernel/tarc/iface/tarcface.h>

#include <util/generic/string.h>

namespace NSnippets
{
    static int CountContentSents(const TArchiveMarkupZones& zones) {
        TVector< std::pair<int, int> > contentSpans;
        EArchiveZone contentZones[] = {AZ_MAIN_CONTENT, AZ_SEGCONTENT};
        for (size_t i = 0; i < Y_ARRAY_SIZE(contentZones); ++i) {
            const TArchiveZone& zone = zones.GetZone(contentZones[i]);
            for (auto span = zone.Spans.begin(); span != zone.Spans.end(); ++span) {
                contentSpans.push_back(std::make_pair(span->SentBeg, span->SentEnd));
            }
        }
        Sort(contentSpans.begin(), contentSpans.end());

        int res = 0;
        size_t i = 0;
        while (i < contentSpans.size()) {
            int l = contentSpans[i].first;
            int r = contentSpans[i].second;
            size_t j = i + 1;
            while (j < contentSpans.size() && contentSpans[j].first < r) {
                if (contentSpans[j].second > r) {
                    r = contentSpans[j].second;
                }
                ++j;
            }
            res += r - l + 1;
            i = j;
        }
        return res;
    }

    class TContentPreviewViewer::TImpl : public IArchiveViewer {
    private:
        struct TSchemaUnpack {
            NSchemaOrg::TTreeNode* Node;
            TSentsOrder Order;

            explicit TSchemaUnpack(NSchemaOrg::TTreeNode* node, int first, int count)
              : Node(node)
              , Order()
            {
                Order.PushBack(first, first + count - 1);
            }

            TSchemaUnpack(const TSchemaUnpack& other)
              : Node(other.Node)
              , Order()
            {
                for (TSentsRangeList::const_iterator it = other.Order.Sents.Begin(); it != other.Order.Sents.End(); ++it) {
                    Order.PushBack(it->FirstId, it->LastId);
                }
            }
        };

        typedef TList<TSchemaUnpack> TSchemaUnpacks;
        typedef THashSet<int> TUsefulSentNumHash;
        typedef TSimpleSharedPtr<TSentsOrder> TSentsOrderPtr;

    private:
        static const int MAX_UNPACK_SENTS = 100;

        const TConfig& Cfg;
        TArchiveStorage Storage;
        const TDocInfos& DocInfos;
        TUnpacker* Unpacker;
        int SentsCount;
        TSentsOrder All;
        TVector<TSentsOrderPtr> BadContentAlls;
        TArchiveView Result;
        TVector<TArchiveView> AdditionalResults;
        TUtf16String OgMeta;
        TSentsOrder ContentForExt;
        const TArchiveMarkupZones* Markup;
        THashSet<int> MainContent;
        THashSet<int> GoodContent;
        int GuessedSentCount;
        int ContentSentCount;
        bool HasForumDocMarkup;
        THolder<NSchemaOrg::TTreeNode> Schema;
        TSchemaUnpacks SchemaUnpacks;
        TVector<std::pair<ui16, ui16>> ForumQuoteSpans;

    private:
        void AddSchemaUnpacks() {
            if (!Schema.Get()) {
                return;
            }
            TVector<NSchemaOrg::TTreeNode*> v(1, Schema.Get());
            int schemed = 0;
            for (size_t i = 0; i < v.size(); ++i) {
                if (!v[i]->ItemtypesSize() && v[i]->HasSentBegin() && v[i]->HasSentCount() &&
                        !(v[i]->HasText() && v[i]->GetText().size())) {
                    int cnt = Min<int>(v[i]->GetSentCount(), MAX_UNPACK_SENTS - schemed);
                    if (cnt > 0) {
                        SchemaUnpacks.push_back(TSchemaUnpack(v[i], v[i]->GetSentBegin(), cnt));
                        Unpacker->AddRequest(SchemaUnpacks.back().Order);
                        schemed += cnt;
                        if (schemed >= MAX_UNPACK_SENTS) {
                            return;
                        }
                    }
                }
                for (size_t j = 0; j < v[i]->NodeSize(); ++j) {
                    v.push_back(v[i]->MutableNode(j));
                }
            }
        }

        void AddSchemaText() {
            if (Markup) {
                GuessedSentCount = GuessLastSent(*Markup) + 1;
                ContentSentCount = CountContentSents(*Markup);
            }
            if (!Cfg.SchemaPreview()) {
                return;
            }
            if (Markup) {
                static const EArchiveZone zones[] = {AZ_MAIN_CONTENT, AZ_SEGCONTENT, AZ_SEGREFERAT};
                for (size_t i = 0; i < Y_ARRAY_SIZE(zones); ++i) {
                    const TArchiveZone& zone = Markup->GetZone(zones[i]);
                    for (TVector<TArchiveZoneSpan>::const_iterator span = zone.Spans.begin(); span != zone.Spans.end(); ++span) {
                        for (ui16 n = span->SentBeg; n <= span->SentEnd; ++n) {
                            if (zones[i] == AZ_MAIN_CONTENT) {
                                MainContent.insert(n);
                            }
                            GoodContent.insert(n);
                        }
                    }
                }
            }
            for (TSchemaUnpacks::const_iterator it = SchemaUnpacks.begin(); it != SchemaUnpacks.end(); ++it) {
                TArchiveView archiveView;
                DumpResult(it->Order, archiveView);
                TUtf16String s;
                for (size_t k = 0; k < archiveView.Size(); ++k) {
                    if (s.size()) {
                        if (archiveView.Get(k)->IsParaStart) {
                            s += '\n';
                        } else {
                            s += ' ';
                        }
                    }
                    s += archiveView.Get(k)->Sent;
                }
                it->Node->SetText(WideToUTF8(s));
            }
        }

        void InitAll() {
            if (All.Empty()) {
                TUsefulSentNumHash goodSegmentsNumbers = GetGoodSegmentsSentNumbers(*Markup);
                auto isGood = [&goodSegmentsNumbers](const ui16 n) {
                    return goodSegmentsNumbers.find(n) != goodSegmentsNumbers.end();
                };
                const TArchiveZone& mainContentZone = Markup->GetZone(AZ_MAIN_CONTENT);
                if (!mainContentZone.Spans.empty()) {
                    AddSents(mainContentZone, All, SentsCount, isGood);
                }
            }

            if (!Markup->GetZone(AZ_FORUM_MESSAGE).Spans.empty()) {
                HasForumDocMarkup = true;
            }

            if (All.Empty()) {
                const TArchiveZone& mZone = Markup->GetZone(AZ_FORUM_MESSAGE);
                if (!mZone.Spans.empty()) {
                    AddSents(mZone.Spans[0], All, SentsCount, AlwaysTrue);
                }
            }

            if (All.Empty())
                TryToAddSomeMetaDescription();
        }

        void TryToAddSomeMetaDescription() {
            // meta & OG:descr
            TString ogmeta;
            TDocInfos::const_iterator meta = DocInfos.find("ultimate_description");
            TDocInfos::const_iterator metaQual = DocInfos.find("meta_quality");
            TDocInfos::const_iterator seeMeta = DocInfos.find("og_see_meta_descr");
            TDocInfos::const_iterator ogd = DocInfos.find("og_descr");
            TDocInfos::const_iterator ogQual = DocInfos.find("og_quality");
            if (ogQual != DocInfos.end() && ogQual->second == TStringBuf("yes")) {
                if (seeMeta != DocInfos.end() && seeMeta->second == TStringBuf("yes")) {
                    if (meta != DocInfos.end()) {
                        ogmeta = meta->second;
                    }
                } else {
                    if (ogd != DocInfos.end()) {
                        ogmeta = ogd->second;
                    }
                }
            } else if (metaQual != DocInfos.end() && metaQual->second == TStringBuf("yes")) {
                if (meta != DocInfos.end()) {
                    ogmeta = meta->second;
                }
            }
            if (ogmeta.size()) {
                OgMeta = UTF8ToWide(ogmeta);
                const TArchiveZone& contentZone = Markup->GetZone(AZ_SEGCONTENT);
                for (TVector<TArchiveZoneSpan>::const_iterator span = contentZone.Spans.begin(); span != contentZone.Spans.end(); ++span) {
                    AddSents(*span, ContentForExt, SentsCount, AlwaysTrue);
                }
            }
        }

        void AddBadContentAll(const TVector<EArchiveZone>& zoneTypes) {
            TSentsOrderPtr all(new TSentsOrder());

            int nSent = 0;
            TSentsOrderGenerator generator;
            for (const auto zoneType : zoneTypes) {
                const TArchiveZone& zone = Markup->GetZone(zoneType);
                if (!zone.Spans.empty())
                    AddSents(zone, generator, nSent, AlwaysTrue);
            }
            generator.SortAndMerge();
            generator.Complete(all.Get());
            BadContentAlls.push_back(all);
        }

        void InitBadContentAlls() {
            TVector<EArchiveZone> worstZones;
            worstZones.push_back(AZ_SEGAUX);
            worstZones.push_back(AZ_SEGCOPYRIGHT);
            AddBadContentAll(worstZones);

            TVector<EArchiveZone> worstZones2;
            worstZones2.push_back(AZ_SEGMENU);
            AddBadContentAll(worstZones2);

            TVector<EArchiveZone> worstZones3;
            worstZones3.push_back(AZ_SEGLINKS);
            worstZones3.push_back(AZ_ANCHOR);
            AddBadContentAll(worstZones3);

            TVector<EArchiveZone> badZones;
            badZones.push_back(AZ_SEGREFERAT);
            AddBadContentAll(badZones);

            TVector<EArchiveZone> sosoZones;
            sosoZones.push_back(AZ_SEGCONTENT);
            AddBadContentAll(sosoZones);
        }

        void AddMainResult(TVector<TArchiveView>& results) {
            results.push_back(TArchiveView());
            TArchiveView& result = results.back();
            DumpResult(All, result);
            if (result.Empty() && OgMeta) {
                result.PushBack(&*Storage.Add(ARC_MISC, 0, OgMeta, 0));
            }
            int maxSent = Markup ? GuessLastSent(*Markup) : 0;
            TUtf16String itemMark = u"\u2022 "; // bullet symbol plus space, • , &bull; , U+2022 , &#x2022; .
            if (Markup) {
                const TArchiveZone& forumsZone = Markup->GetZone(AZ_FORUM_INFO);
                if (!forumsZone.Spans.empty()) {
                    HasForumDocMarkup = true;
                }
                if (result.Empty()) {
                    const TArchiveZoneAttrs& forumAttrs = Markup->GetZoneAttrs(AZ_FORUM_INFO);
                    int mSents = 0;
                    if (!forumsZone.Spans.empty() && forumsZone.Spans[0].SentBeg <= maxSent * 0.4) {
                        for (TVector<TArchiveZoneSpan>::const_iterator ii = forumsZone.Spans.begin(), end = forumsZone.Spans.end(); ii != end; ++ii) {
                            if (mSents >= MAX_UNPACK_SENTS) {
                                break;
                            }
                            const TArchiveZoneSpan& span = *ii;
                            if (span.Empty()) {
                                continue;
                            }
                            const THashMap<TString, TUtf16String>* attributes = forumAttrs.GetSpanAttrs(span).AttrsHash;
                            if (!attributes) {
                                continue;
                            }
                            TUtf16String s = ExtractForumAttr(attributes, NArchiveZoneAttr::NForum::NAME);
                            if (!s.size()) {
                                continue;
                            }
                            s = itemMark + s;
                            result.PushBack(&*Storage.Add(ARC_MISC, mSents, s, SENT_IS_PARABEG));
                            ++mSents;
                        }
                    }
                }
            }
            if (Markup) {
                const TArchiveZone& forumTopicsZone = Markup->GetZone(AZ_FORUM_TOPIC_INFO);
                if (!forumTopicsZone.Spans.empty()) {
                    HasForumDocMarkup = true;
                }
                if (result.Empty()) {
                    const TArchiveZoneAttrs& forumTopicAttrs = Markup->GetZoneAttrs(AZ_FORUM_TOPIC_INFO);
                    int mSents = 0;
                    if (!forumTopicsZone.Spans.empty() && forumTopicsZone.Spans[0].SentBeg <= maxSent * 0.4) {
                        for (TVector<TArchiveZoneSpan>::const_iterator ii = forumTopicsZone.Spans.begin(), end = forumTopicsZone.Spans.end(); ii != end; ++ii) {
                            if (mSents >= MAX_UNPACK_SENTS) {
                                break;
                            }
                            const TArchiveZoneSpan& span = *ii;
                            if (span.Empty()) {
                                continue;
                            }
                            const THashMap<TString, TUtf16String>* attributes = forumTopicAttrs.GetSpanAttrs(span).AttrsHash;
                            if (!attributes) {
                                continue;
                            }
                            TUtf16String s = ExtractForumAttr(attributes, NArchiveZoneAttr::NForum::NAME);
                            if (!s.size()) {
                                continue;
                            }
                            s = itemMark + s;
                            result.PushBack(&*Storage.Add(ARC_MISC, mSents, s, SENT_IS_PARABEG));
                            ++mSents;
                        }
                    }
                }
            }
        }

        void AddBadContentResults(TVector<TArchiveView>& results) {
            for (const auto& all : BadContentAlls) {
                results.push_back(TArchiveView());
                TArchiveView& result = results.back();
                DumpResult(*all, result);
            }
        }

        static TUsefulSentNumHash GetGoodSegmentsSentNumbers(const TArchiveMarkupZones& zones) {
            static const EArchiveZone USEFUL_ARCHIVE_ZONES[] = {AZ_SEGCONTENT, AZ_SEGHEAD};
            static const size_t USEFUL_ARCHIVE_ZONES_COUNT = Y_ARRAY_SIZE(USEFUL_ARCHIVE_ZONES);
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

        static bool IsTrustedMetaEnd(TWtringBuf e) {
            size_t dots = 0;
            size_t spaces = 0;
            for (size_t i = 0; i < e.size(); ++i) {
                if (e[i] == '-') {
                    continue;
                }
                if (IsSpace(e.data() + i, 1)) {
                    ++spaces;
                    continue;
                }
                if (e[i] == '.') {
                    ++dots;
                    continue;
                }
                return true; //some meaningfull punct - not a damaged word
            }
            if (dots == 1 && dots + spaces == e.size() && e[0] == '.') { //fine too
                return true;
            }
            return false;
        }

        static bool IsTrustedMetaEnd(const TSentsInfo& si) {
            int w1 = si.WordCount() - 1;
            if (w1 < 0) {
                return false;
            }
            TWtringBuf m = si.Text;
            TWtringBuf e(m.data() + si.WordVal[w1].Word.EndOfs(), m.data() + m.size());
            if (!e.size()) {
                return false;
            }
            return IsTrustedMetaEnd(e);
        }

        template <class TSentsOrder, class TPredicate>
        static void AddSents(const TArchiveZoneSpan& span, TSentsOrder& sOrder, int& nSents, TPredicate p) {
            for (ui16 n = span.SentBeg; n <= span.SentEnd && nSents < MAX_UNPACK_SENTS; ++n) {
                if (p(n)) {
                    sOrder.PushBack(n, n);
                    ++nSents;
                }
            }
        }

        template <class TSentsOrder, class TPredicate>
        static void AddSents(const TArchiveZone& zone, TSentsOrder& sOrder, int& nSents, TPredicate p) {
            for (TVector<TArchiveZoneSpan>::const_iterator span = zone.Spans.begin(); span != zone.Spans.end(); ++span)
                AddSents(*span, sOrder, nSents, p);
        }

        static bool AlwaysTrue(const ui16 /*n*/) {
            return true;
        }

        void SaveForumQuoteSpans(const TArchiveMarkupZones& zones) {
            static const EArchiveZone zone = AZ_FORUM_QBODY;
            const TVector<TArchiveZoneSpan>& spans = zones.GetZone(zone).Spans;
            for (const TArchiveZoneSpan& span : spans) {
                if (span.Empty()) {
                    continue;
                }
                ForumQuoteSpans.push_back(std::make_pair(span.SentBeg, span.SentEnd));
            }
        }

    public:
        TImpl(const TConfig& cfg, const TDocInfos& docInfos)
          : Cfg(cfg)
          , Storage()
          , DocInfos(docInfos)
          , Unpacker(nullptr)
          , SentsCount(0)
          , Markup(nullptr)
          , MainContent()
          , GoodContent()
          , GuessedSentCount()
          , ContentSentCount()
          , HasForumDocMarkup(false)
          , Schema()
          , SchemaUnpacks()
        {
            if (Cfg.SchemaPreview() && docInfos.find("SchemaOrg") != docInfos.end()) {
                TString schema = docInfos.find("SchemaOrg")->second;
                Schema.Reset(new NSchemaOrg::TTreeNode());
                if (!NSchemaOrg::DeserializeFromBase64(schema, *Schema.Get())) {
                    Schema.Reset(nullptr);
                }
            }
        }

        void OnUnpacker(TUnpacker* unpacker) override {
            Unpacker = unpacker;
        }

        void OnMarkup(const TArchiveMarkupZones& zones) override {
            Markup = &zones;

            InitAll();
            if (!All.Empty()) {
                Unpacker->AddRequest(All);
            }
            if (!ContentForExt.Empty()) {
                Unpacker->AddRequest(ContentForExt);
            }

            if (Cfg.UseBadSegments()) {
                InitBadContentAlls();
                for (auto& all : BadContentAlls)
                    if (!all->Empty())
                        Unpacker->AddRequest(*all);
            }

            if (HasForumDocMarkup) {
                SaveForumQuoteSpans(zones);
            }

            AddSchemaUnpacks();
        }

        void OnEnd() override {
            AddSchemaText();

            TVector<TArchiveView> results;
            if (Cfg.UseBadSegments())
                AddBadContentResults(results);
            AddMainResult(results);

            if (results.size()) {
                Result = results[0];
                for (size_t i = 1; i < results.size(); ++i)
                    AdditionalResults.push_back(results[i]);
            }
        }

        const NSchemaOrg::TTreeNode* GetSchema() const {
            return Schema.Get();
        }
        const THashSet<int>& GetMainContent() const {
            return MainContent;
        }
        const THashSet<int>& GetGoodContent() const {
            return GoodContent;
        }
        int GuessSentCount() const {
            return GuessedSentCount;
        }
        int GetContentSentCount() const {
            return ContentSentCount;
        }
        bool IsForumDoc() const {
            return HasForumDocMarkup;
        }
        const TArchiveView& GetResult() const {
            return Result;
        }
        const TVector<TArchiveView>& GetAdditionalResults() const {
            return AdditionalResults;
        }
        const TVector<std::pair<ui16, ui16>>& GetForumQuoteSpans() const {
            return ForumQuoteSpans;
        }
    };

    TContentPreviewViewer::TContentPreviewViewer(const TConfig& cfg, const TDocInfos& docInfos)
      : Impl(new TImpl(cfg, docInfos))
    {
    }
    TContentPreviewViewer::~TContentPreviewViewer()
    {
    }
    IArchiveViewer& TContentPreviewViewer::GetViewer() {
        return *Impl.Get();
    }
    const TArchiveView& TContentPreviewViewer::GetResult() const {
        return Impl->GetResult();
    }
    const TVector<TArchiveView>& TContentPreviewViewer::GetAdditionalResults() const {
        return Impl->GetAdditionalResults();
    }
    const NSchemaOrg::TTreeNode* TContentPreviewViewer::GetSchema() const {
        return Impl->GetSchema();
    }
    const THashSet<int>& TContentPreviewViewer::GetMainContent() const {
        return Impl->GetMainContent();
    }
    const THashSet<int>& TContentPreviewViewer::GetGoodContent() const {
        return Impl->GetGoodContent();
    }
    int TContentPreviewViewer::GuessSentCount() const {
        return Impl->GuessSentCount();
    }
    int TContentPreviewViewer::GetContentSentCount() const {
        return Impl->GetContentSentCount();
    }
    bool TContentPreviewViewer::IsForumDoc() const {
        return Impl->IsForumDoc();
    }
    const TVector<std::pair<ui16, ui16>>& TContentPreviewViewer::GetForumQuoteSpans() const {
        return Impl->GetForumQuoteSpans();
    }
}
