#include "video_desc.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/archive/view/storage.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/sent_info/beautify.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/simple_textproc/decapital/decapital.h>
#include <kernel/snippets/smartcut/clearchar.h>
#include <kernel/snippets/title_trigram/title_trigram.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <util/generic/string.h>
#include <util/string/strip.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets {
    static const TUtf16String SPACE = u" ";
    static const TUtf16String WWW = u"www.";
    static const TUtf16String HTTP = u"http:";
    static const TUtf16String HTTPS = u"https:";

    class TVideoData {
    public:
        bool HasData;
        TUtf16String Descr;
        TUtf16String Meta;

    public:
        TVideoData(const TDocInfos& infos);
        void FixTags();
    };

    TVideoData::TVideoData(const TDocInfos& infos) {
        TStaticData data(infos, "vthumb");
        HasData = !data.Absent && !data.Attrs.empty();
        Descr = data.Attrs["desc"];
        TDocInfos::const_iterator meta = infos.find("ultimate_description");
        TDocInfos::const_iterator metaQual = infos.find("meta_quality");
        if (meta != infos.end() && metaQual != infos.end()) {
            if ("yes" != TString(metaQual->second)) {
                return;
            }
            Meta = UTF8ToWide(meta->second);
        }
    }

    static void Fix(TUtf16String& data, bool allowEmpty = true) {
        TWtringBuf ndata = CutFirstUrls(data);
        if (allowEmpty || ndata.size()) {
            data = ndata;
        }
        if (!data.size())
            return;
        if (!data.StartsWith(WWW) && !data.StartsWith(HTTP) && !data.StartsWith(HTTPS))
            ToUpper(data.begin(), 1);
        ClearChars(data, true);
    }

    static inline size_t FindLast(TWtringBuf s, const TWtringBuf& pattern) {
        size_t prev = TUtf16String::npos;
        size_t cur = s.find(pattern, 0);
        while (cur != TUtf16String::npos) {
            prev = cur;
            cur = s.find(pattern, cur + 1);
        }
        return prev;
    }

    static inline size_t LastEnd(size_t a, size_t b) {
        if (a == TUtf16String::npos) {
            return b;
        } else if (b == TUtf16String::npos) {
            return a;
        } else {
            return Max(a, b);
        }
    }

    static void CutTail(TUtf16String& res) {
        size_t p = FindLast(res, u"YouTube");
        p = LastEnd(p, FindLast(res, u"Видео"));
        p = LastEnd(p, FindLast(res, u"Vidéo"));
        p = LastEnd(p, FindLast(res, u"смотреть"));
        if (p == TUtf16String::npos) {
            return;
        }
        TWtringBuf tmp(res.data(), res.data() + p);
        p = FindLast(tmp, u" ::");
        p = LastEnd(p, FindLast(tmp, u" -"));
        p = LastEnd(p, FindLast(tmp, u" \u2014")); // em-dash
        if (p == TUtf16String::npos) {
            return;
        }
        res.resize(p);
    }

    static void FixTitle(TUtf16String& res) {
        CutTail(res);
        Fix(res, false);
    }

    static void UnSlash(TUtf16String& in) {
        TUtf16String res;
        size_t i = 0;
        while (i < in.size()) {
            if (i + 1 < in.size() && in[i] == '\\') {
                if (in[i + 1] == 'n') {
                    res += ' ';
                } else {
                    res += in[i + 1];
                }
                i += 2;
            } else {
                res += in[i];
                i += 1;
            }
        }
        in = res;
    }

    static constexpr size_t MIN_SNIPPET_LENGTH = 100;

    static bool ProcessCandidate(TUtf16String text, const TUtf16String& title,
                                 const TReplaceContext& ctx, TReplaceResult& snip)
    {
        // hack for broken video.yandex.ru data - has a lot of \char
        UnSlash(text);
        Fix(text);

        if (text.size() < MIN_SNIPPET_LENGTH) {
            return false;
        }

        double oldWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true)
                           .Add(title).Add(ctx.Snip).AddUrl(ctx.Url).GetWeight();
        double newWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true)
                           .Add(title).Add(text).AddUrl(ctx.Url).GetWeight();
        if (newWeight + 1E-7 < oldWeight) {
            return false;
        }

        DecapitalSentence(text, ctx.DocLangId);
        TSmartCutOptions options(ctx.Cfg);
        options.MaximizeLen = true;
        SmartCut(text, ctx.IH, ctx.LenCfg.GetNDesktopRowsLen(2), options);
        snip.UseText(text, "video_desc");
        return true;
    }

    void TVideoReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();

        // SNIPPETS-3550
        if (ctx.Cfg.IsMainPage()) {
            return;
        }

        TVideoData videoData(ctx.DocInfos);

        if (!videoData.HasData) {
            if (!ctx.Cfg.ForceVideoTitle()) {
                return;
            }
        }

        TReplaceResult snip;
        bool titleChanged = false;
        if (ctx.Cfg.VideoBlock()) {
            TUtf16String naturalTitle = ctx.NaturalTitle.GetTitleString();
            FixTitle(naturalTitle);
            snip.UseTitle(MakeSpecialTitle(naturalTitle, ctx.Cfg, ctx.QueryCtx));
            titleChanged = true;
        }
        const TUtf16String& title = (snip.GetTitle() ? snip.GetTitle()->GetTitleString()
                                               : ctx.SuperNaturalTitle.GetTitleString());

        bool snippetChanged = ProcessCandidate(videoData.Descr, title, ctx, snip) ||
                              ProcessCandidate(videoData.Meta, title, ctx, snip);

        if (ctx.Cfg.ForceVideoTitle() && !snippetChanged) {
            //verstka wants snip in headline to avoid if-s
            snip.UseText(ctx.Snip.GetRawTextWithEllipsis(), "video_desc");
            snippetChanged = true;
        }
        if (snippetChanged || titleChanged) {
            manager->Commit(snip, MRK_VIDEO);
        }
    }
}
