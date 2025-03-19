#include "socnet.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/lang_check.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/snip_proto/snip_proto_2.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>

#include <util/generic/string.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/charset/wide.h>

namespace NSnippets {

static const TStringBuf SOCNET_HOSTS[] = {
    ".vk.com",
    ".vkontakte.ru",
    ".twitter.com",
    ".facebook.com",
    ".linkedin.com",
};

static bool IsSocnetHost(const TString& url) {
    const TString host = "." + TString{CutWWWPrefix(GetOnlyHost(url))};
    for (const TStringBuf& socnet : SOCNET_HOSTS) {
        if (host.EndsWith(socnet)) {
            return true;
        }
    }
    return false;
}

void TSocnetDataReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& repCtx = manager->GetContext();
    if (!IsSocnetHost(repCtx.Url) || repCtx.IsByLink) {
        return;
    }
    const bool noCyr = repCtx.Cfg.SuppressCyrForTr() && !repCtx.QueryCtx.CyrillicQuery;
    TSnip snip = GetBestSnip(repCtx.SentsMInfo, repCtx.SuperNaturalTitle, repCtx.Cfg, "", repCtx.LenCfg, repCtx.SnipWordSpanLen, manager->GetCallback(), repCtx.IsByLink, 2.0);
    TUtf16String descr = snip.GetRawTextWithEllipsis();
    if (noCyr && HasTooManyCyrillicWords(descr, 2)) {
        descr.clear();
    }
    if (manager->IsBodyReplaced()) {
        TUtf16String spec = manager->GetResult().GetText();
        if (noCyr && HasTooManyCyrillicWords(spec, 2)) {
            spec.clear();
        }
        if (spec.size() > descr.size()) {
            descr = spec;
        }
    }
    if (descr) {
        NJson::TJsonValue res(NJson::JSON_MAP);
        res["genericsnip"] = NJson::TJsonValue(WideToUTF8(descr));
        manager->GetExtraSnipAttrs().AddClickLikeSnipJson("socnetsnip_extra", NJson::WriteJson(res, false, true));
    }
}

}

