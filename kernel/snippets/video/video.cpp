#include "video.h"

#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/archive/view/storage.h>
#include <kernel/snippets/archive/unpacker/unpacker.h>

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/iface/archive/manip.h>
#include <kernel/snippets/iface/archive/sent.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/smartcut/smartcut.h>

#include <kernel/snippets/titles/make_title/make_title.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <library/cpp/charset/wide.h>
#include <library/cpp/langmask/langmask.h>

#include <util/string/cast.h>
#include <util/string/split.h>

namespace NSnippets {

namespace {

    const TUtf16String KEY_LINKLANG = u"linklang";
    const TUtf16String KEY_TITLEWEIGHT = u"titlew";
    const TUtf16String KEY_TITLEZONE = u"tz";
    const TUtf16String KEY_LINKZONE = u"lz";
    const TUtf16String KEY_MAIN = u"main";
    const TUtf16String KEY_HIDE = u"hide";
    const TUtf16String ONE = u"1";
    const TUtf16String ZERO = u"0";

    const wchar16 ATTR_DELIM = '\t';

    inline bool IsLanguageGoodForFace(EFaceType face, ELanguage lang) {
        static const TLangMask faceLangMasks[] = {
            {LANG_ENG, LANG_RUS}, // ftDefault
            {LANG_ENG}, // ftYandexCom
            {LANG_ENG, LANG_TUR, LANG_GER, LANG_FRE, LANG_ARA, LANG_AZE}, // ftYandexComTr
            {LANG_ENG, LANG_CZE, LANG_SLO}, // ftYandexCz
            {LANG_ENG, LANG_KAZ}, // ftYandexKz
            {LANG_ENG, LANG_UKR, LANG_RUS}, // ftYandexUa
        };

        if (static_cast<size_t>(face) >= Y_ARRAY_SIZE(faceLangMasks)) {
            Y_ASSERT(!"unknown face type");
            return false;
        }

        return faceLangMasks[face].SafeTest(lang);
    }

    bool IsGoodLang(ELanguage lang, const TLangMask& queryLangMask, EFaceType face) {
        return queryLangMask.SafeTest(lang) || IsLanguageGoodForFace(face, lang);
    }

    inline bool IsLanguageBadForFace(EFaceType face, ELanguage lang) {
        static const TLangMask faceLangMasks[] = {
            {}, // ftDefault
            {}, // ftYandexCom
            {LANG_UKR, LANG_RUS}, // ftYandexComTr
            {LANG_UKR, LANG_RUS}, // ftYandexCz
            {}, // ftYandexKz
            {}, // ftYandexUa
        };

        if (static_cast<size_t>(face) >= Y_ARRAY_SIZE(faceLangMasks)) {
            Y_ASSERT(!"unknown face type");
            return false;
        }

        return faceLangMasks[face].SafeTest(lang);
    }

    bool IsBadLang(ELanguage lang, const TLangMask& queryLangMask, EFaceType face) {
        if (lang == LANG_UNK) {
            return !queryLangMask.Empty();
        }
        return !queryLangMask.SafeTest(lang) && IsLanguageBadForFace(face, lang);
    }


    struct TVideoTitleCand {
        TSnipTitle Title;
        ELanguage Lang;
        int Tic;
        bool IsNaturalTitle;

        TVideoTitleCand()
          : Title()
          , Lang(LANG_UNK)
          , Tic(0)
          , IsNaturalTitle(false)
        {
        }

        TVideoTitleCand(const TSnipTitle& title, ELanguage lang, int tic, bool isNaturalTitle)
            : Title(title)
            , Lang(lang)
            , Tic(tic)
            , IsNaturalTitle(isNaturalTitle)
        {
        }
    };

    class ITitleRanker {
    public:
        virtual void Look(const TVideoTitleCand& t) = 0;
        virtual const TVideoTitleCand& GetBest() const = 0;
        virtual float GetWeight() const = 0;

        virtual ~ITitleRanker() {}
    };


    class TVideoTitleRanker: public ITitleRanker {
    public:
        TVideoTitleRanker(const TConfig& cfg, const TQueryy& query)
          : Query(query)
          , QueryLangMask(cfg.GetQueryLangMask())
          , FaceType(cfg.GetFaceType())
          , Best()
          , Weight(0)
        {
        }

        void Look(const TVideoTitleCand& t) override {
            const auto cur = GetTitleWeight(t);
            if (cur > Weight) {
                Weight = cur;
                Best = t;
            }
        }

        const TVideoTitleCand& GetBest() const override {
            return Best;
        }

        float GetWeight() const override {
            return Weight;
        }

    protected:
        virtual float GetTitleWeight(const TVideoTitleCand& t) const {
            if (t.Title.GetTitleString().empty() || IsBadLang(t.Lang, QueryLangMask, FaceType)) {
                return 0;
            }
            float weight = t.Tic;
            if (IsGoodLang(t.Lang, QueryLangMask, FaceType)) {
                weight += 100;
            }
            return weight;
        }

        void SetBest(const TVideoTitleCand& best) {
            Best = best;
        }

        void SetWeight(float weight) {
            Weight = weight;
        }

        TLangMask GetQueryLangMask() const {
            return QueryLangMask;
        }

        EFaceType GetFaceType() const {
            return FaceType;
        }

        const TQueryy& GetQuery() const {
            return Query;
        }

    private:
        const TQueryy& Query;
        const TLangMask QueryLangMask;
        const EFaceType FaceType;

        TVideoTitleCand Best;
        float Weight;
    };

    class TVideoHeaderBasedTitleRanker: public TVideoTitleRanker {
    public:
        using TVideoTitleRanker::TVideoTitleRanker;

        void Look(const TVideoTitleCand& t) override {
            const auto cur = GetTitleWeight(t);
            if (!cur) {
                return;
            }
            if (CompareTitleCandidates(t.Title, GetBest().Title)) {
                SetWeight(cur);
                SetBest(t);
            }
        }
    private:
        bool CompareTitleCandidates(const TSnipTitle& title1, const TSnipTitle& title2) {
            return IsCandidateBetterThanTitle(GetQuery(), title1, title2, false, false);
        }
    };

    class TVideoLinearTitleRanker: public TVideoTitleRanker {
    public:
        using TVideoTitleRanker::TVideoTitleRanker;
    protected:
        float GetTitleWeight(const TVideoTitleCand& t) const override {
            if (t.Title.GetTitleString().empty()) {
                return 0;
            }

            const auto w =
                +1.0 * t.Title.GetPLMScore()
                +1.0 * t.Title.GetSynonymsCount()
                +1.0 * t.Title.GetQueryWordsRatio()
                -1.0 * t.Title.GetTitleString().size() * (1. / 50)
                +1.0 * (t.Tic / 1e6)
                +1.0 * t.IsNaturalTitle
                -1.0 * IsBadLang(t.Lang, GetQueryLangMask(), GetFaceType())
                -1.0 * GetQuery().SumIdfLog
                ;

            return w;
        }
    };

    class TVideoFactorsGetter {
    public:
        bool Hide = false;
        bool Main = false;
        bool Description = false;

    private:
        int Idx = 0;
        TString Cur;

    public:
        bool Consume(const char* b, const char* e, const char*);
        void ConsumeAttr(TStringBuf textAttr);
    };

    bool TVideoFactorsGetter::Consume(const char* b, const char* e, const char*) {
        if (Idx % 2 == 0) {
            Cur.assign(b, e);
        } else {
            if (Cur == TStringBuf("hide")) {
                Hide = FromStringWithDefault<int>(TStringBuf(b, e - b)) == 1;
            } else if (Cur == TStringBuf("main")) {
                Main = TStringBuf(b, e - b) == TStringBuf("1");
            } else if (Cur == TStringBuf("lz")) {
                Description = true;
            }
        }
        ++Idx;
        return true;
    }

    void TVideoFactorsGetter::ConsumeAttr(TStringBuf textAttr) {
        TCharDelimiter<const char> c('\t');
        SplitString(textAttr.data(), textAttr.data() + textAttr.size(), c, *this);
    }

    bool IsMainDescr(const TUtf16String& sent, const TString& attr) {
        if (!sent) {
            return false;
        }
        TVideoFactorsGetter factors;
        factors.ConsumeAttr(attr);
        if (factors.Hide) {
            return false;
        }
        return factors.Main && factors.Description;
    }

    void NewHideVideoSnippets(TVector<TZonedString>& snipVec, TVector<TString>& attrVec, const TConfig& cfg, const TLengthChooser& lenCfg, TArchiveStorage& store, TArc& arc) {
        bool nothingToHide = true;
        for (size_t i = 0; i < attrVec.size() && i < snipVec.size(); ++i) {
            TVideoFactorsGetter factors;
            factors.ConsumeAttr(attrVec[i]);
            if (factors.Hide) {
                nothingToHide = false;
                break;
            }
        }
        if (nothingToHide && snipVec.size() != 0) {
            return;
        }
        if (arc.IsValid()) {
            TSentsOrder all;
            all.PushBack(1, 65535);
            TArchiveMarkup markup;
            TUnpacker unpacker(cfg, &store, &markup, ARC_TEXT);
            unpacker.AddRequest(all);
            arc.PrefetchData();
            TBlob doctext = arc.GetData();
            if (!doctext.Empty()) {
                unpacker.UnpText(doctext.AsUnsignedCharPtr());
                TArchiveView view;
                DumpResult(all, view);
                for (size_t i = 0; i < view.Size(); ++i) {
                    TString attr = WideToUTF8(view.Get(i)->Attr);
                    TUtf16String sent = TUtf16String(view.Get(i)->Sent);
                    if (IsMainDescr(sent, attr)) {
                        snipVec.clear();
                        attrVec.clear();
                        TTextCuttingOptions options;
                        options.StopWordsFilter = &cfg.GetStopWordsFilter();
                        options.MaximizeLen = true;
                        options.AddEllipsisToShortText = true;
                        sent = SmartCutSymbol(sent, lenCfg.GetMaxSnipLen(), options); // TODO: length stuff
                        snipVec.push_back(sent);
                        attrVec.push_back(attr);
                    }
                }
            }
        }
    }
} // namespace

    bool GenerateVideoTitle(TSnipTitle& resTitle, const TArchiveView& textView, const TQueryy& query, const TMakeTitleOptions& options, const TDocInfos& docInfos, const TConfig& config, const TSnipTitle& naturalTitle) {
        if (config.IsVideoExp())
            return false;

        std::unique_ptr<ITitleRanker> ranker;
        auto titleExperiment = false;
        if (config.ExpFlagOn("video_headerbased")) {
            titleExperiment = true;
            ranker = std::make_unique<TVideoHeaderBasedTitleRanker>(config, query);
        } else if (config.ExpFlagOn("video_linear")) {
            titleExperiment = true;
            ranker = std::make_unique<TVideoLinearTitleRanker>(config, query);
        } else {
            ranker = std::make_unique<TVideoTitleRanker>(config, query);
        }

        const auto titleLangIt = docInfos.find("titlelang");
        if (titleLangIt != docInfos.end()) {
            const auto titleLang = static_cast<ELanguage>(FromStringWithDefault<int>(titleLangIt->second));
            ranker->Look(TVideoTitleCand(naturalTitle, titleLang, 1000000, true));
        }
        if (!ranker->GetWeight() || titleExperiment) {
            if (titleLangIt == docInfos.end()) { // sometimes titlelang is not set
                ranker->Look(TVideoTitleCand(naturalTitle, LANG_ENG, 1000, true));
            }

            for (size_t i = 0; i < textView.Size(); ++i) {
                bool isHidden = false;
                bool isTitle = false;
                bool isCanonicalUrl = false;
                int tic = 0;
                int lang = LANG_UNK;
                char zone = '\0';
                char link = '\0';
                TWtringBuf attr = textView.Get(i)->Attr;
                while (!attr.empty()) {
                    const TWtringBuf& key = attr.NextTok(ATTR_DELIM);
                    const TWtringBuf& value = attr.NextTok(ATTR_DELIM);

                    if (key == KEY_TITLEWEIGHT) {
                        isTitle = TryFromString(value, tic);
                    } else if (key == KEY_LINKLANG) {
                        TryFromString(value, lang);
                    } else if (key == KEY_HIDE) {
                        isHidden = value == ONE;
                    } else if (key == KEY_MAIN) {
                        isCanonicalUrl = value == ONE;
                    } else if (key == KEY_TITLEZONE && !value.empty()) {
                        zone = value[0];
                    } else if (key == KEY_LINKZONE) {
                        link = value[0];
                    }
                }
                if (isTitle || (titleExperiment && isCanonicalUrl && link == 'd')) {
                    if (isCanonicalUrl)
                        tic += 1000000;
                    if (!isHidden)
                        tic += 100000;
                    if ('v' == zone)
                        tic += 100;
                    const auto title = MakeTitle(ToWtring(textView.Get(i)->Sent), config, query, options);
                    ranker->Look(TVideoTitleCand(title, static_cast<ELanguage>(lang), tic, false));
                }
            }
        }
        if (ranker->GetWeight()) {
            if (ranker->GetBest().IsNaturalTitle || titleExperiment && !IsCandidateBetterThanTitle(query, ranker->GetBest().Title, naturalTitle, false, false)) {
                resTitle = naturalTitle;
            } else {
                resTitle = ranker->GetBest().Title;
            }
            return true;
        }
        return false;
    }

    void HideVideoSnippets(TVector<TZonedString>& snipVec, TVector<TString>& attrVec, const TConfig& cfg, const TLengthChooser& lenCfg, TArchiveStorage& store, TArc& arc) {
        NewHideVideoSnippets(snipVec, attrVec, cfg, lenCfg, store, arc);
    }

    double TreatVideoPassageAttrWeight(const TString& textAttr, const TConfig& cfg)
    {
        double videoAttrWeight = 0.0;

        TVideoFactorsGetter factors;
        factors.ConsumeAttr(textAttr);

        const EFaceType ft = cfg.GetFaceType();
        bool isYandexDef = ft == ftDefault;

        if (isYandexDef && factors.Hide)
            videoAttrWeight -= 100;

        return videoAttrWeight;
    }

    double TreatVideoPassageAttrWeight(const TWtringBuf& attribute, const TConfig& cfg)
    {
        const TString textAttr = WideToChar(attribute.data(), attribute.size(), CODES_YANDEX);
        return TreatVideoPassageAttrWeight(textAttr, cfg);
    }

}
