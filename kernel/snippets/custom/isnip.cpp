#include "isnip.h"
#include "extended_length.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/markers/markers.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <library/cpp/scheme/scheme.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/string/split.h>

namespace NSnippets {

    static bool TryExtractRequestText(const TReplaceContext& ctx, TString& requestText) {
        if (!ctx.QueryCtx.OrigTree) {
            return false;
        }
        try {
            requestText = WideToUTF8(ctx.QueryCtx.OrigRequestText);
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool TryGetSnipByQuery(const NSc::TValue& snippets, const TString& query, TUtf16String& isnipText) {
        for (const NSc::TValue& item : snippets.GetArray()) {
            if (item["query"].GetString() == query) {
                isnipText = UTF8ToWide(item["snippet"].GetString());
                return !!isnipText;
            }
        }
        return false;
    }

    static bool TryExtractISnipText(const TDocInfos& docInfos, const TString& query, const TString& keyByPriority, TUtf16String& isnipText) {
        const TDocInfos::const_iterator it = docInfos.find("isnip");
        if (it == docInfos.end()) {
            return false;
        }
        NSc::TValue data = NSc::TValue::FromJson(it->second);
        if (data.IsArray()) {
            return TryGetSnipByQuery(data, query, isnipText);
        } else if (data.IsDict()) {
            if (keyByPriority.empty()) {
                return false;
            }
            TVector<TString> keys;
            StringSplitter(keyByPriority).Split('|').AddTo(&keys);
            for (const TString& key : keys) {
                if (!data.Has(key)) {
                    continue;
                }
                if (TryGetSnipByQuery(data[key], query, isnipText)) {
                    return true;
                }
            }
        }
        return false;
    }

    static float CalcLocalCutLength(const TConfig& cfg, const TSnip& snip) {
        TWordSpanLen snipLengthCalcer(TCutParams::Pixel(cfg.GetYandexWidth(), cfg.GetSnipFontSize()));
        return std::ceil(snipLengthCalcer.CalcLength(snip.Snips));
    }

    static size_t GetFragmentCount(const TReplaceManager* manager) {
        size_t naturalFragmentCount = manager->GetContext().Snip.Snips.size();
        if (manager->GetResult().GetText().empty()) {
            return naturalFragmentCount;
        }
        if (manager->GetContext().IsByLink) {
            return naturalFragmentCount + 1;
        }
        return 1;
    }

    TISnipReplacer::TISnipReplacer()
        : IReplacer("isnip")
    {}

    void TISnipReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();
        if (!ctx.Cfg.ExpFlagOn("isnip_enable")) {
            return;
        }
        if (ctx.Cfg.ExpFlagOn("isnip_subst_only_natural") && !manager->GetResult().GetText().empty()) {
            return;
        }
        if (ctx.Cfg.ExpFlagOn("isnip_subst_only_1_frag") && GetFragmentCount(manager) != 1) {
            return;
        }
        TString requestText;
        if (!TryExtractRequestText(ctx, requestText)) {
            return;
        }
        const TString& dataKeys = ctx.Cfg.GetISnipDataKeys();
        TUtf16String isnipText;
        if (!TryExtractISnipText(ctx.DocInfos, requestText, dataKeys, isnipText)) {
            return;
        }
        float maxLen = ctx.LenCfg.GetMaxSnipLen();
        if (ctx.Cfg.ExpFlagOn("isnip_local_cut")) {
            maxLen = CalcLocalCutLength(ctx.Cfg, ctx.Snip);
        }
        TMultiCutResult cutResult = SmartCutWithExt(isnipText, ctx.IH, maxLen, GetExtSnipLen(ctx.Cfg, ctx.LenCfg), ctx.Cfg);
        if (!!cutResult.Short) {
            TReplaceResult result;
            result.UseTitle(ctx.SuperNaturalTitle);
            result.UseText(cutResult, "isnip");
            manager->Commit(result, MRK_ISNIP);
        }
    }
} // namespace NSnippets
