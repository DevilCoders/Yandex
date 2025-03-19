#include "yaca.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/data/yaca_list.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/lang_check.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <util/string/subst.h>

namespace NSnippets
{
    static const char* const HEADLINE_SRC = "yaca";
    static const size_t MIN_SHORT_YACA_TITLE_LEN = 3;

    bool IsYacaForced(const TReplaceContext& ctx) {
        return ctx.IsNav || TForcedYacaUrls::GetDefault().Contains(ctx.Url);
    }

    bool IsYacaBanned(const TReplaceContext& ctx) {
        if (!ctx.Cfg.CatalogOptOutAllowed()) {
            return false;
        }
        const auto attr = ctx.DocInfos.find("noyaca");
        return attr != ctx.DocInfos.end() && TStringBuf(attr->second) == "yes";
    }

    TYacaData::TYacaData(const TDocInfos& docInfos, const ELanguage lang, const TConfig& cfg) {
        if (lang == LANG_RUS) {
            TStaticData data(docInfos, "catalog");
            Title = data.Attrs["title"];
            Desc = data.Attrs["desc"];
        } else if (lang == LANG_UKR) {
            TStaticData data(docInfos, "catalog");
            Title = data.Attrs["title_ua"];
            Desc = data.Attrs["desc_ua"];
        } else if (lang == LANG_ENG) {
            TStaticData data1(docInfos, "yaca_en");
            Title = data1.Attrs["title"];
            Desc = data1.Attrs["snippet"];
            if (Title.empty() || Desc.empty()) {
                TStaticData data2(docInfos, "catalog");
                Title = data2.Attrs["title_en"];
                Desc = data2.Attrs["desc_en"];
            }
        } else if (lang == LANG_TUR) {
            TStaticData data(docInfos, "yaca_tr");
            Title = data.Attrs["title"];
            Desc = data.Attrs["snippet"];
        }
        if (cfg.ShortYacaTitles() && Title.size() > cfg.ShortYacaTitleLen()) {
            size_t from = Title.find('"');
            size_t to = Title.rfind('"');
            if (from != to && to - from - 1 >= MIN_SHORT_YACA_TITLE_LEN && to - from - 1 <= cfg.ShortYacaTitleLen()) {
                Title = Title.substr(from + 1, to - from - 1);
            }
        }
        if (!cfg.QuotesInYaca()) {
            SubstGlobal(Title, u"\"", TUtf16String());
        }
    }

    static void DoReplace(TReplaceManager* manager, const TYacaData& yacaData) {
        const TReplaceContext& ctx = manager->GetContext();
        TUtf16String desc = yacaData.Desc;
        TSmartCutOptions options(ctx.Cfg);
        options.MaximizeLen = true;
        SmartCut(desc, ctx.IH, ctx.LenCfg.GetMaxSpecSnipLen(), options);
        TReplaceResult res;
        res.UseText(desc, HEADLINE_SRC);
        res.UseTitle(MakeSpecialTitle(yacaData.Title, ctx.Cfg, ctx.QueryCtx));
        manager->Commit(res, MRK_YACA);
    }

    void TYacaReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();
        if (IsYacaBanned(ctx)) {
            return;
        }
        const ELanguage foreignNavHackLang = ctx.Cfg.GetForeignNavHackLang();
        if (foreignNavHackLang == LANG_TUR) {
            TYacaData yacaData(ctx.DocInfos, LANG_TUR, ctx.Cfg);
            if (yacaData.Title && yacaData.Desc) {
                DoReplace(manager, yacaData);
                return;
            }
        }
        if (ctx.Cfg.IsYandexCom() || foreignNavHackLang != LANG_UNK) {
            TYacaData yacaData(ctx.DocInfos, LANG_ENG, ctx.Cfg);
            if (yacaData.Title && yacaData.Desc) {
                DoReplace(manager, yacaData);
                return;
            }
        }
        if (foreignNavHackLang != LANG_UNK && foreignNavHackLang != LANG_RUS) {
            return;
        }
        if (ctx.Cfg.NavYComLangHack() && ctx.Cfg.IsYandexCom() && ctx.IsNav) {
            return;
        }
        ELanguage yacaLang = ctx.DocLangId == LANG_UKR ? LANG_UKR : LANG_RUS;
        TYacaData yacaData(ctx.DocInfos, yacaLang, ctx.Cfg);
        if (!yacaData.Title || !yacaData.Desc) {
            return;
        }

        TSnipTitle cuttedTitle = MakeSpecialTitle(yacaData.Title, ctx.Cfg, ctx.QueryCtx);
        TUtf16String desc = yacaData.Desc;
        TSmartCutOptions options(ctx.Cfg);
        options.MaximizeLen = true;
        SmartCut(desc, ctx.IH, ctx.LenCfg.GetMaxSpecSnipLen(), options);
        if (ctx.Cfg.IsYandexCom() && ctx.DocLangId != LANG_UNK &&
            !HasWordsOfAlphabet(desc, ctx.DocLangId))
        {
            return;
        }

        TReplaceResult res;

        if (ctx.IsByLink) {
            if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(desc).Add(cuttedTitle).AddUrl(ctx.Url).Add(ctx.Snip) >
                TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(desc).Add(cuttedTitle).AddUrl(ctx.Url))
            {
                res.SetPreserveSnip();
            }
            res.UseText(desc, HEADLINE_SRC);
            res.UseTitle(cuttedTitle);
        } else if (ctx.Snip.Snips.empty()) {
            res.UseText(desc, HEADLINE_SRC);
            if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(ctx.NaturalTitle).Add(ctx.MetaDescr.GetTextCopy()).AddUrl(ctx.Url) <=
                TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(cuttedTitle).Add(desc).AddUrl(ctx.Url))
            {
                res.UseTitle(cuttedTitle);
            }
        } else if (IsYacaForced(ctx)) {
            if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(ctx.NaturalTitle).Add(ctx.MetaDescr.GetTextCopy()).Add(ctx.Snip).AddUrl(ctx.Url) <=
                TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(cuttedTitle).Add(desc).AddUrl(ctx.Url))
            {
                res.UseText(desc, HEADLINE_SRC);
                res.UseTitle(cuttedTitle);
            } else if (ctx.IsNav) {
                if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(ctx.MetaDescr.GetTextCopy()).Add(ctx.Snip) <=
                    TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(desc))
                {
                    res.UseText(desc, HEADLINE_SRC);
                    if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(ctx.NaturalTitle) <=
                        TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(cuttedTitle))
                    {
                        res.UseTitle(cuttedTitle);
                    }
                }
            }
        }

        if (res.CanUse()) {
            manager->Commit(res, MRK_YACA);
        }
    }
}

