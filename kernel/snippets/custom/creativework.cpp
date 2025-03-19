#include "creativework.h"
#include "extended_length.h"

#include <kernel/snippets/archive/staticattr.h>

#include <kernel/snippets/config/config.h>

#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>
#include <kernel/snippets/custom/trash_classifier/trash_classifier.h>

#include <kernel/snippets/cut/cut.h>

#include <kernel/snippets/read_helper/read_helper.h>

#include <kernel/snippets/schemaorg/creativework.h>
#include <kernel/snippets/schemaorg/schemaorg_parse.h>
#include <kernel/snippets/schemaorg/videoobj.h>

#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/extra_attrs.h>

#include <kernel/snippets/simple_cmp/simple_cmp.h>

#include <kernel/snippets/titles/make_title/util_title.h>

#include <kernel/lemmer/alpha/abc.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/string.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/subst.h>

namespace NSnippets {
    static const char* const HEADLINE_SRC = "creativework_snip";
    static constexpr size_t MIN_VIDEO_OBJ_GOOD_LEN = 70;
    static constexpr size_t MIN_LENGTH = 64;
    static constexpr size_t MAX_LIST_COUNT = 3;

    inline void AddProperty(TString& res, const TString& pName, const TString& pValue) {
        if (res.size()) {
            res += "\x07;";
        }
        res += pName;
        res += "=";
        res += pValue;
    }

    inline void AddProperty(TString& res, const TString& pName, const TUtf16String& pValue) {
        AddProperty(res, pName, WideToUTF8(pValue));
    }

    TString GetAvatarUrl(const TDocInfos& docInfos) {
        static constexpr TStringBuf attrName = "video_thumb";
        const auto it = docInfos.find(attrName);
        if (it == docInfos.cend())
            return TString();
        const TStringBuf attrVal = TStringBuf(it->second);

        NSc::TValue json = NSc::TValue::FromJson(attrVal);
        const NSc::TValue& photoArr = json.Get("photo");
        if (photoArr.Has(0)) // we're OK to show any pic presented in "avatarnica"
            return TString(photoArr.Get(0).GetString());
        return TString();
    }

    inline bool HasGoodVideoData(const NSchemaOrg::TCreativeWork& creativeWork) {
        return creativeWork.Name && creativeWork.VideoThumbUrl &&
            creativeWork.Description.size() >= MIN_VIDEO_OBJ_GOOD_LEN;
    }

    TString GetSnipVthumb(const NSchemaOrg::TCreativeWork& creativeWork, const TString& docUrl,
        const TString& avatarUrl)
    {
        TString res;
        // properties from schema.org markup
        AddProperty(res, "thumb", creativeWork.VideoThumbUrl);
        TDuration dur = NSchemaOrg::ParseDuration(WideToUTF8(creativeWork.VideoDuration));
        if (dur.Seconds() > 0) {
            AddProperty(res, "dur", ToString(dur.Seconds()));
        }
        AddProperty(res, "avatar", avatarUrl);
        // some natural properties
        AddProperty(res, "VisibleURL", AddSchemePrefix(docUrl));
        TStringBuf host = GetOnlyHost(docUrl);
        TString hostName = Singleton<NSchemaOrg::TVideoHostWhiteList>()->GetVideoHost(host);
        Y_ASSERT(hostName.size() != 0);
        AddProperty(res, "hst", hostName);
        // stub properties
        AddProperty(res, "html5", "y");
        AddProperty(res, "embed", "y");
        AddProperty(res, "nid", "12345678-11-11"); // fake id
        return res;
    }

    static bool CanUseTitle(const TReplaceContext& ctx, const TSnipTitle& baseTitle, const TSnipTitle& newTitle) {
        if (!newTitle.GetTitleString()) {
            return false;
        }
        if (ctx.Cfg.EliminateDefinitions() &&
            baseTitle.GetTitleString().StartsWith(newTitle.GetTitleString()) &&
            ctx.Cfg.IsTitleContractionOk(baseTitle.GetTitleString().size(), newTitle.GetTitleString().size()))
        {
            // in EliminateDefinitions mode we prefer shorter titles without sitename and other endings
            return true;
        }
        return !baseTitle.GetTitleString().StartsWith(newTitle.GetTitleString()) &&
               TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(baseTitle) <=
               TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(newTitle);
    }

    bool TryBuildDescription(const NSchemaOrg::TCreativeWork* creativeWork, TReplaceManager* manager,
        TUtf16String& desc, TMultiCutResult& extDesc)
    {
        const TReplaceContext& ctx = manager->GetContext();
        const TUtf16String& description = creativeWork->Description ?
            creativeWork->Description : creativeWork->ArticleBody;
        if (!description) {
            manager->ReplacerDebug("description and articleBody fields are missing");
            return false;
        }

        TUtf16String authors = creativeWork->GetAuthors(MAX_LIST_COUNT);
        if (authors) {
            desc += authors;
            if (!IsTerminal(desc.back())) {
                desc += '.';
            }
            desc += ' ';
        }
        TUtf16String genres = creativeWork->GetGenres(MAX_LIST_COUNT);
        if (genres) {
            const NLemmer::TAlphabetWordNormalizer* wordNormalizer =
                NLemmer::GetAlphaRules(ctx.DocLangId);
            wordNormalizer->ToTitle(genres);
            desc += genres + u". ";
        }
        desc += description;

        if (desc.size() >= MIN_LENGTH) {
            extDesc = CutDescription(desc, ctx, ctx.LenCfg.GetMaxTextSpecSnipLen());
            desc = extDesc.Short;
        }
        if (desc.size() < MIN_LENGTH) {
            manager->ReplacerDebug("headline is too short", TReplaceResult().UseText(desc, HEADLINE_SRC));
            return false;
        }

        if (NTrashClassifier::IsTrash(ctx, desc)) {
            manager->ReplacerDebug("the text is recognized as unreadable", TReplaceResult().UseText(desc, HEADLINE_SRC));
            return false;
        }
        if (extDesc.Long && NTrashClassifier::IsTrash(ctx, extDesc.Long)) {
            manager->ReplacerDebug("the extended text is recognized as unreadable");
            extDesc = TMultiCutResult(desc);
        }
        return true;
    }

    void TCreativeWorkReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();

        const NSchemaOrg::TCreativeWork* creativeWork = ArcViewer.GetCreativeWork();
        if (!creativeWork) {
            return;
        }

        // manual vthumb part
        if (ctx.Cfg.ExpFlagOn("schema_vthumb") && HasGoodVideoData(*creativeWork)) {
            const TString avatarUrl = GetAvatarUrl(ctx.DocInfos);
            if (avatarUrl) {
                const TString vthumb = GetSnipVthumb(*creativeWork, ctx.Url, avatarUrl);
                manager->GetExtraSnipAttrs().SetSchemaVthumb(vthumb);
                manager->GetExtraSnipAttrs().Markers.SetMarker(MRK_SCHEMA_VTHUMB);
                manager->ReplacerDebug("Snippet vthumb was created and passed");
            }
        }

        if (ctx.Cfg.IsMainPage()) {
            manager->ReplacerDebug("the main page is not processed");
            return;
        }

        TUtf16String desc;
        TMultiCutResult extDesc;
        bool canUseDesc = TryBuildDescription(creativeWork, manager, desc, extDesc);

        if (!canUseDesc && !ctx.Cfg.EliminateDefinitions()) {
            return;
        }

        const TUtf16String& title = creativeWork->Name ? creativeWork->Name : creativeWork->Headline;
        TSnipTitle newTitle;
        if (title) {
            newTitle = MakeSpecialTitle(title, ctx.Cfg, ctx.QueryCtx);
        }

        bool useDesc = false;
        bool useTitle = CanUseTitle(ctx, ctx.NaturalTitle, newTitle);
        if (canUseDesc) {
            double oldWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true)
                .Add(ctx.SuperNaturalTitle).Add(ctx.Snip).GetWeight();
            double newWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true)
                .Add(useTitle ? newTitle : ctx.NaturalTitle).Add(desc).GetWeight();
            if (newWeight >= oldWeight) {
                useDesc = true;
            }
        }
        if (!useDesc && ctx.SuperNaturalTitle.GetTitleString() != ctx.NaturalTitle.GetTitleString()) {
            useTitle = CanUseTitle(ctx, ctx.SuperNaturalTitle, newTitle);
        }

        if (!useDesc && !useTitle) {
            manager->ReplacerDebug("contains less non stop query words than original one",
                TReplaceResult().UseText(desc, HEADLINE_SRC).UseTitle(newTitle));
            return;
        }
        TReplaceResult res;
        if (useDesc) {
            res.UseText(extDesc, HEADLINE_SRC);
        }
        if (useTitle) {
            res.UseTitle(newTitle);
        }
        manager->Commit(res, MRK_CREATIVE_WORK);
    }

    TCreativeWorkReplacer::TCreativeWorkReplacer(const TSchemaOrgArchiveViewer& arcViewer)
        : IReplacer("creative_work")
        , ArcViewer(arcViewer)
    {
    }
}
