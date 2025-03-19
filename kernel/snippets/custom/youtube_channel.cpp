#include "youtube_channel.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/opengraph/og.h>
#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/schemaorg/youtube_channel.h>

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/lemmer/core/language.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/json_reader.h>

namespace NSnippets {

static const TUtf16String TITLE_SUFFIX    = u" - YouTube";
static const TString REPLACER        = "youtube_channel";
static const TString REPLACER_IMG    = REPLACER + "_img";
static const TString ADAPTER_NAME    = "youtube_channel";
static const size_t ACCEPT_BODY_LEN = 60;

static bool IsYouTubeChannelLikeUrl(const TString& url) {
    TStringBuf cut = url;
    static constexpr TStringBuf http = "http://";
    static constexpr TStringBuf https = "https://";
    if (!cut.SkipPrefix(http)) {
        cut.SkipPrefix(https);
    }

    cut.SkipPrefix(TStringBuf("www."));
    if (!cut.SkipPrefix(TStringBuf("youtube.com/"))) {
        return false;
    }

    return (
        cut.StartsWith(TStringBuf("user")) ||
        cut.StartsWith(TStringBuf("channel")) ||
        cut.StartsWith(TStringBuf("show"))
    );
}

static TString GetLastAccess(const TDocInfos& docInfos) {
    static constexpr TStringBuf attrName = "lastaccess";
    const auto it = docInfos.find(attrName);
    return (it == docInfos.end()) ? "" : it->second;
}

static NJson::TJsonValue GetVThumbJson(const TDocInfos& docInfos) {
    NJson::TJsonValue res;
    static constexpr TStringBuf attrName = "video_thumb";
    const auto it = docInfos.find(attrName);
    if (it == docInfos.cend()) {
        return res;
    }
    NJson::TParserCallbacks parser(res);
    NJson::ReadJsonFast(it->second, &parser);
    return res;
}

static TString GetAvatarUrl(const NJson::TJsonValue& vthumbJson) {
    const NJson::TJsonValue& photoArr = vthumbJson["photo"];
    if (photoArr.Has(0)) { // we're OK to show any pic presented in "avatarnica"
        return TString(photoArr[0].GetString());
    }
    return TString();
}

TYoutubeChannelReplacer::TYoutubeChannelReplacer(const TSchemaOrgArchiveViewer& arcViewer)
    : IReplacer(REPLACER)
    , ArcViewer(arcViewer)
{
}

void TYoutubeChannelReplacer::DoWork(TReplaceManager* manager) {
    const NSchemaOrg::TYoutubeChannel* uc = ArcViewer.GetYoutubeChannel();
    if (!uc) {
        return;
    }
    const TReplaceContext& ctx = manager->GetContext();
    if (!IsYouTubeChannelLikeUrl(ctx.Url)) {
        return;
    }
    TUtf16String body = uc->Description ? uc->Description : GetOgDescr(ctx.DocInfos);
    if (body.length() < ACCEPT_BODY_LEN) {
        manager->ReplacerDebug("Description is too short");
        return;
    }

    SmartCut(body, ctx.IH, ctx.LenCfg.GetMaxTextSpecSnipLen(), TSmartCutOptions(ctx.Cfg));
    if (body.length() < ACCEPT_BODY_LEN) {
        manager->ReplacerDebug("Description after cut is too short");
        return;
    }

    TReplaceResult res;
    if (uc->Name) {
        res.UseTitle(MakeSpecialTitle(uc->Name + TITLE_SUFFIX, ctx.Cfg, ctx.QueryCtx));
    }

    const TString* replacerName = &REPLACER;
    EMarker marker = MRK_YOUTUBE_CHANNEL;

    if (ctx.Cfg.UseYoutubeChannelImage()) {
        NJson::TJsonValue vthumb = GetVThumbJson(ctx.DocInfos);
        if (vthumb["src_type"].GetString() == REPLACER) {
            TString image = GetAvatarUrl(vthumb);
            if (image.empty()) {
                return;
            }
            TString la = GetLastAccess(ctx.DocInfos);

            TStringStream stream;
            NJson::TJsonWriter json(&stream, false);
            json.OpenMap();
            json.Write("content_plugin", "true");
            json.Write("block_type", "construct");
            json.Write("features");
            json.OpenMap();
            json.Write("type", REPLACER);
            json.Write("image", image);
            json.Write("name", WideToUTF8(uc->Name));
            json.Write("description", WideToUTF8(body));
            json.Write("isfamilyfriendly", WideToUTF8(uc->IsFamilyFriendly));
            if (la) {
                json.Write("timestamp", la);
            }
            json.CloseMap();
            json.CloseMap();
            json.Flush(); // usefull flush

            replacerName = &REPLACER_IMG;
            marker = MRK_YOUTUBE_CHANNEL_IMG;
            manager->GetExtraSnipAttrs().AddClickLikeSnipJson(ADAPTER_NAME, stream.Str());
        }
    } else {
        manager->ReplacerDebug("vthumb[\"src_type\"] is not " + REPLACER);
    }

    res.UseText(body, *replacerName);
    manager->Commit(res, marker);
}

};
