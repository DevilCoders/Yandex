#include "dmoz.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/delink/delink.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/titles/make_title/make_title.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <kernel/tarc/docdescr/docdescr.h>

#include <library/cpp/langs/langs.h>
#include <util/string/split.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

namespace NSnippets {
    namespace {

        class TDmozLangRecognizer {
            typedef THashMap<TUtf16String, ELanguage> TTopicLang;
            TTopicLang TopicLang;
        public:
            TDmozLangRecognizer() {
                TopicLang[u"Arts"] = LANG_ENG;
                TopicLang[u"Business"] = LANG_ENG;
                TopicLang[u"Computers"] = LANG_ENG;
                TopicLang[u"Games"] = LANG_ENG;
                TopicLang[u"Health"] = LANG_ENG;
                TopicLang[u"Home"] = LANG_ENG;
                TopicLang[u"Kids and Teens"] = LANG_ENG;
                TopicLang[u"News"] = LANG_ENG;
                TopicLang[u"Recreation"] = LANG_ENG;
                TopicLang[u"Reference"] = LANG_ENG;
                TopicLang[u"Science"] = LANG_ENG;
                TopicLang[u"Shopping"] = LANG_ENG;
                TopicLang[u"Society"] = LANG_ENG;
                TopicLang[u"Sports"] = LANG_ENG;
                TopicLang[u"World/Belarussian"] = LANG_BEL;
                TopicLang[u"World/Kazakh"] = LANG_KAZ;
                TopicLang[u"World/Russian"] = LANG_RUS;
                TopicLang[u"World/Tatar\u00E7a"] = LANG_TAT;
                TopicLang[u"World/Ukrainian"] = LANG_UKR;
                TopicLang[u"World/T\u00FCrk\u00E7e"] = LANG_TUR;
            }
            ELanguage GetLang(const TUtf16String&, const TUtf16String&, const TUtf16String& topic) const {
                TTopicLang::const_iterator it;
                size_t t = 0;
                for (int i = 0; i < 2; ++i) {
                    if (t >= topic.size()) {
                        break;
                    }
                    t = topic.find('/', t);
                    if (t == TUtf16String::npos) {
                        t = topic.size();
                    }
                    it = TopicLang.find(topic.substr(0, t));
                    if (it != TopicLang.end()) {
                        return it->second;
                    }
                    ++t;
                }
                return LANG_UNK;
            }
        };
        class TDmozDataSplitter {
        private:
            static bool TryField(TStringBuf& res, TStringBuf s, TStringBuf prefix) {
                if (s.StartsWith(prefix)) {
                    res = TStringBuf(s.data() + prefix.size(), s.size() - prefix.size());
                    return true;
                }
                return false;
            }
            static bool TryField(TUtf16String& res, TStringBuf s, TStringBuf prefix) {
                TStringBuf r;
                if (TryField(r, s, prefix)) {
                    res = UTF8ToWide(r.data(), r.size());
                    return true;
                }
                return false;
            }
        public:
            class TDump {
            public:
                TUtf16String Title;
                TUtf16String Desc;
                TUtf16String Topic;
                ELanguage Lang;
                void Clear() {
                    Title.clear();
                    Desc.clear();
                    Topic.clear();
                    Lang = LANG_UNK;
                }
                bool Consume(const char* b, const char* e, const char*) {
                    TStringBuf s(b, e);
                    TryField(Title, s, "title=");
                    TryField(Desc, s, "desc=");
                    TryField(Topic, s, "topic=");
                    return true;
                }
            };
        private:
            typedef TVector<TDump> TDumps;
            TUtf16String Title;
            TUtf16String Desc;
            TDumps Dumps;
            void ParseDump(TStringBuf s) {
                TStringDelimiter<const char> d("\x07,");
                TDump dump;
                SplitString(s.data(), s.data() + s.size(), d, dump);
                if (!dump.Title.size() || !dump.Desc.size() || !dump.Topic.size()) {
                    return;
                }
                dump.Lang = Singleton<TDmozLangRecognizer>()->GetLang(dump.Title, dump.Desc, dump.Topic);
                Dumps.push_back(dump);
            }
        public:
            TDmozDataSplitter() {
                Dumps.reserve(4);
            }
            bool Consume(const char* b, const char* e, const char*) {
                TStringBuf s(b, e);
                TryField(Title, s, "title=");
                TryField(Desc, s, "desc=");
                TStringBuf dump;
                if (TryField(dump, s, "dump=")) {
                    ParseDump(dump);
                }
                return true;
            }
            void Consume(const TString& attr) {
                TStringDelimiter<const char> d("\x07;");
                SplitString(attr.data(), attr.data() + attr.size(), d, *this);
            }
            bool LookupDump(ELanguage lang, TDump& result) const {
                TDumps::const_iterator candidate = Dumps.end();
                size_t maxLen = 0;
                for (TDumps::const_iterator ii = Dumps.begin(), end = Dumps.end(); ii != end; ++ii) {
                    if (ii->Lang == lang && ii->Desc.size() > maxLen) {
                        maxLen = ii->Desc.size();
                        candidate = ii;
                    }
                }
                if (candidate != Dumps.end()) {
                    result = *candidate;
                    return true;
                }
                return false;
            }
        };
    }

    class TDmozData::TImpl {
        TDmozDataSplitter DataSplitter;
    public:
        TImpl(const TDocInfos& docInfos) {
            const TDocInfos::const_iterator it = docInfos.find("dmoz");
            if (it != docInfos.end()) {
                DataSplitter.Consume(it->second);
            }
        }
        bool FindByLanguage(ELanguage lang, TUtf16String& title, TUtf16String& desc) {
            TDmozDataSplitter::TDump dump;
            if (DataSplitter.LookupDump(lang, dump) && dump.Title && dump.Desc) {
                title = dump.Title;
                desc = dump.Desc;
                return true;
            }
            return false;
        }
    };
    TDmozData::TDmozData(const TDocInfos& docInfos)
        : Impl(new TImpl(docInfos))
    {
    }
    TDmozData::~TDmozData() {
    }
    bool TDmozData::FindByLanguage(ELanguage lang, TUtf16String& title, TUtf16String& desc) {
        return Impl->FindByLanguage(lang, title, desc);
    }

    static constexpr double MIN_DMOZ_SIMILARITY_RATIO = 0.6;

    bool DmozBanned(const TConfig& cfg, const TDocInfos& docInfos) {
        if (!cfg.CatalogOptOutAllowed()) {
            return false;
        }
        const TDocInfos::const_iterator attr = docInfos.find("noodp");
        return attr != docInfos.end() && TStringBuf("yes") == attr->second;
    }

    void TDmozReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();
        if (DmozBanned(ctx.Cfg, ctx.DocInfos)) {
            return;
        }
        const char* const src = "dmoz";
        ELanguage wantLang = ctx.DocLangId;
        bool checkMatches = true;
        const ELanguage needLang = ctx.Cfg.GetForeignNavHackLang();
        if (ctx.Cfg.NavYComLangHack() && (ctx.IsNav || ctx.Cfg.IsMainPage()) && (ctx.Cfg.IsYandexCom() || needLang != LANG_UNK)) {
            wantLang = (needLang == LANG_UNK) ? LANG_ENG : needLang;
            checkMatches = false;
        }
        if (wantLang == LANG_UNK) {
            return;
        }
        TUtf16String title;
        TUtf16String desc;
        TDmozData data(ctx.DocInfos);
        if (!data.FindByLanguage(wantLang, title, desc)) {
            return;
        }
        const bool killLink = CanKillByLinkSnip(ctx);
        const TSnip emptySnip;
        const TSnip& realSnip = killLink ? emptySnip : ctx.Snip;
        if (realSnip.Snips.empty() && ctx.MetaDescr.MayUse()) { // SNIPPETS-798
            return;
        }
        if (ctx.IsNav && ctx.MetaDescr.MayUse() && (realSnip.Snips.empty() || ctx.IsByLink)) {
            return;
        }
        TSnipTitle cuttedTitle = MakeSpecialTitle(title, ctx.Cfg, ctx.QueryCtx);
        TSmartCutOptions options(ctx.Cfg);
        options.MaximizeLen = true;
        SmartCut(desc, ctx.IH, ctx.LenCfg.GetMaxSpecSnipLen(), options);
        TReplaceResult res;
        if (ctx.IsByLink) {
            if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(desc).Add(cuttedTitle).AddUrl(ctx.Url).Add(realSnip) >
                TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(desc).Add(cuttedTitle).AddUrl(ctx.Url))
            {
                res.SetPreserveSnip();
            }
            res.UseText(desc, src);
            res.UseTitle(cuttedTitle);
        }
        else if (!checkMatches) {
            res.UseText(desc, src);
            bool useDmozTitle = true;
            if (ctx.Cfg.SuppressDmozTitles()) {
                if (!SimilarTitleStrings(cuttedTitle.GetTitleString(), ctx.NaturalTitle.GetTitleString(), MIN_DMOZ_SIMILARITY_RATIO, true)) {
                    TUtf16String gluedTitleString = cuttedTitle.GetTitleString() + GetTitleSeparator(ctx.Cfg) +
                        ctx.SuperNaturalTitle.GetTitleString();
                    TMakeTitleOptions options2(ctx.Cfg);
                    options2.DefinitionMode = TDM_IGNORE;
                    options2.TitleGeneratingAlgo = TGA_NAIVE;
                    TSnipTitle gluedTitle = MakeTitle(gluedTitleString, ctx.Cfg, ctx.QueryCtx, options2);
                    if (!gluedTitle.GetTitleString().empty()) {
                        useDmozTitle = false;
                        res.UseTitle(gluedTitle);
                    }
                }
                if (useDmozTitle && ctx.NaturalTitle.GetMatchUserIdfSum() > cuttedTitle.GetMatchUserIdfSum() + 1E-7) {
                    useDmozTitle = false;
                    res.UseTitle(ctx.NaturalTitle);
                }
                if (useDmozTitle && ctx.NaturalTitle.GetMatchUserIdfSum() + 1E-7 > cuttedTitle.GetMatchUserIdfSum() && ctx.NaturalTitle.GetPixelLength() > cuttedTitle.GetPixelLength()) {
                    useDmozTitle = false;
                    res.UseTitle(ctx.NaturalTitle);
                }
            }

            if (useDmozTitle) {
                res.UseTitle(cuttedTitle);
            }
        }
        else if (ctx.Snip.Snips.empty()) {
            res.UseText(desc, src);
            if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(ctx.NaturalTitle).Add(desc).AddUrl(ctx.Url) < TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(cuttedTitle).Add(desc).AddUrl(ctx.Url)) {
                res.UseTitle(cuttedTitle);
            }
        }
        else if (ctx.IsNav) {
            if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(ctx.NaturalTitle).Add(ctx.MetaDescr.GetTextCopy()).Add(ctx.Snip).AddUrl(ctx.Url) < TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(cuttedTitle).Add(desc).AddUrl(ctx.Url)) {
                res.UseText(desc, src);
                res.UseTitle(cuttedTitle);
            }
            else if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(ctx.MetaDescr.GetTextCopy()).Add(ctx.Snip) < TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(desc)) {
                res.UseText(desc, src);
                if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(ctx.NaturalTitle) < TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(cuttedTitle)) {
                    res.UseTitle(cuttedTitle);
                }
            }
        }
        if (res.CanUse())
            manager->Commit(res, MRK_DMOZ);
        else
            manager->ReplacerDebug("contains less query words than original one", res);
    }
}
