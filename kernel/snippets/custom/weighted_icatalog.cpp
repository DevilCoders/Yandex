#include "weighted_icatalog.h"
#include "replacer_weighter.h"
#include "extended_length.h"
#include "yaca.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_match/len_chooser.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/string.h>

namespace NSnippets
{
    void TWeightedIdealCatalogCandidateProducer::Produce(ELanguage lang, TList<TSpecSnippetCandidate>& candidates)
    {
        if (lang != LANG_RUS) {
            return;
        }
        if (Context.Cfg.ExpFlagOff("weighted_icatalog")) {
            return;
        }
        if (IsYacaBanned(Context)) {
            return;
        }

        const auto attr = Context.DocInfos.find("icatalog");
        if (attr == Context.DocInfos.end()) {
            return;
        }
        NSc::TValue desc = NSc::TValue::FromJson(attr->second);
        if (!desc.IsDict() || !desc.Has("snippet")) {
            return;
        }

        // TODO: Check for BROKEN_RUNEs
        TUtf16String snip = UTF8ToWide<true>(desc.Get("snippet").GetString());
        TUtf16String title = UTF8ToWide<true>(desc.Get("title").GetString());
        if (!snip) {
            return;
        }

        candidates.push_back(TSpecSnippetCandidate());
        TSpecSnippetCandidate& candidate = candidates.back();
        candidate.Source = "icatalog";
        TSmartCutOptions options(Context.Cfg);
        options.MaximizeLen = true;
        float maxLen = Context.LenCfg.GetMaxSpecSnipLen();
        float maxExtLen = GetExtSnipLen(Context.Cfg, Context.LenCfg);
        if (Context.Cfg.ExpFlagOff("weighted_icatalog_ext")) {
            maxExtLen = 0;
        }

        candidate.SetExtendedText(SmartCutWithExt(snip, Context.IH, maxLen, maxExtLen, options));

        if (title) {
            candidate.Title = MakeSpecialTitle(title, Context.Cfg, Context.QueryCtx);
        }
    }
}
