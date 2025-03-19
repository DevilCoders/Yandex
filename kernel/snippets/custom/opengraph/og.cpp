#include "og.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/delink/delink.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/split.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets
{
    struct TOgTextWhitelist
    {
        typedef THashMap<TStringBuf, bool> TDomains;
        TDomains Domains;

        TOgTextWhitelist()
        {
            Domains["ttnetmuzik.com.tr"] = true;
            Domains["sahibinden.com"] = true;
            Domains["mynet.com"] = true;
            Domains["milliyet.com.tr"] = true;
            Domains["dizi-mag.org"] = true;
            Domains["otoraba.com"] = true;
            Domains["sabah.com.tr"] = true;
            Domains["eksisozluk.com"] = true;
            Domains["sikayetvar.com"] = true;
            Domains["ensonhaber.com"] = true;
            Domains["startv.com.tr"] = true;
            Domains["oyunmoyun.com"] = true;
            Domains["fotomac.com.tr"] = true;
            Domains["tamindir.com"] = true;
            Domains["hurriyetemlak.com"] = true;
            Domains["hurriyet.com.tr"] = true;
            Domains["milliyetemlak.com"] = true;
            Domains["haberturk.com"] = true;
            Domains["milliyet.tv"] = true;
            Domains["haber7.com"] = true;
            Domains["webrazzi.com"] = true;
            Domains["fanatik.com.tr"] = true;
            Domains["haberler.com"] = true;
            Domains["blogcu.com"] = true;
            Domains["izlesene.com"] = true;
            Domains["oyuncehennemi.com"] = true;
            Domains["oyungemisi.com"] = true;
            Domains["chip.com.tr"] = true;
            Domains["mackolik.com"] = true;
            Domains["oyunoyna.com"] = true;
            Domains["radikal.com.tr"] = true;

            Domains["videonuz.ensonhaber.com"] = false;
        }

        bool CheckSite(const TString& url) const
        {
            TStringBuf host = CutWWWPrefix(GetOnlyHost(url));
            for (size_t dot = 0; dot < host.size(); ++dot) {
                if (dot == 0 || host[dot - 1] == '.') {
                    TDomains::const_iterator ii = Domains.find(host.Skip(dot));
                    if (ii != Domains.end()) {
                        return ii->second;
                    }
                }
            }
            return false;
        }
    };

    TUtf16String GetOgDescr(const TDocInfos& infos) {
        TDocInfos::const_iterator oi = infos.find("ultimate_description");
        TUtf16String res;
        if (oi != infos.end())
            res = UTF8ToWide(TString(oi->second));
        return res;
    }

    TOgTitleData::TOgTitleData(const TDocInfos& docInfos) {
        TDocInfos::const_iterator oi = docInfos.find("og_attrs");
        if (oi == docInfos.end()) {
            return;
        }
        HasAttrs = true;
        TVector<TString> ogVec;
        StringSplitter(oi->second).Split('\t').Collect(&ogVec);
        for (size_t i = 0; i + 1 < ogVec.size(); i += 2) {
            const TString& name = ogVec[i];
            const TString& value = ogVec[i + 1];
            if (name == "og:title") {
                Title = UTF8ToWide(value);
            }
            if (name == "og:site_name") {
                Sitename = UTF8ToWide(value);
            }
        }
    }

    TOgTextReplacer::TOgTextReplacer()
        : IReplacer("og")
    {
    }


    void TOgTextReplacer::DoWork(TReplaceManager* manager)
    {
        const TReplaceContext& ctx = manager->GetContext();

        if (ctx.Cfg.DisableOgTextReplacer()) {
            return;
        }
        const TOgTextWhitelist& whitelist = Default<TOgTextWhitelist>();
        if (!whitelist.CheckSite(ctx.Url)) {
            return;
        }

        TOgTitleData ogTitleData(ctx.DocInfos);
        if (!ogTitleData.HasAttrs) {
            return;
        }

        TUtf16String ogTitle(ogTitleData.Title);
        TUtf16String ogDescr = GetOgDescr(ctx.DocInfos);

        if (!ogTitle && !ogDescr) {
            return;
        }

        TSnipTitle cutOgTitle;
        if (!!ogTitle) {
            cutOgTitle = MakeSpecialTitle(ogTitle, ctx.Cfg, ctx.QueryCtx);
        }

        TUtf16String cutOgDescr = ogDescr;
        if (!!cutOgDescr) {
            SmartCut(cutOgDescr, ctx.IH, ctx.LenCfg.GetMaxTextSpecSnipLen(), TSmartCutOptions(ctx.Cfg));
        }

        bool wantOgTitle = false;
        bool wantOgBody = false;

        const bool killLink = CanKillByLinkSnip(ctx);
        if (!!ogTitle) {
            TSimpleSnipCmp natTitleCmp(ctx.QueryCtx, ctx.Cfg, false, true); natTitleCmp.Add(ctx.NaturalTitle);
            TSimpleSnipCmp ogTitleCmp(ctx.QueryCtx, ctx.Cfg, false, true); ogTitleCmp.Add(cutOgTitle);
            wantOgTitle = (ogTitleCmp.GetWeight() > natTitleCmp.GetWeight());
        }
        if (!!cutOgDescr) {
            TSimpleSnipCmp natBody(ctx.QueryCtx, ctx.Cfg, false, true); natBody.Add(killLink ? TSnip() : ctx.Snip);
            TSimpleSnipCmp ogBody(ctx.QueryCtx, ctx.Cfg, false, true); ogBody.Add(cutOgDescr);
            wantOgBody = (ogBody.GetWeight() > natBody.GetWeight());
        }

        if (!wantOgTitle && !wantOgBody) {
            return;
        }

        TReplaceResult result;
        if (wantOgBody) {
            result.UseText(cutOgDescr, "ogtext");
        } else {
            result.UseSnip(killLink ? TSnip() : ctx.Snip, "ogtext");
        }
        if (wantOgTitle) {
            result.UseTitle(cutOgTitle);
        }
        manager->Commit(result, MRK_OGTEXT);
    }
}
