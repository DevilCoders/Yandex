#include "movie.h"
#include "extended_length.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>
#include <kernel/snippets/custom/trash_classifier/trash_classifier.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/schemaorg/movie.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <kernel/lemmer/alpha/abc.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets {

static const TString BLACKLISTED_HOSTS[] = {
    "kinofilms.ua", // all persons are listed twice: in Russian and then in English
};
static const TString HOST_IMDB = "imdb.com";
static const TString HEADLINE_SRC = "schema_movie";
static constexpr size_t MIN_MOVIE_SNIP_LENGTH = 50;

TMovieReplacer::TMovieReplacer(const TSchemaOrgArchiveViewer& arcViewer)
    : IReplacer("schema_movie")
    , ArcViewer(arcViewer)
{
}

void TMovieReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& ctx = manager->GetContext();

    const NSchemaOrg::TMovie* movie = ArcViewer.GetMovie();
    if (!movie) {
        return;
    }

    if (ctx.Cfg.IsMainPage()) {
        manager->ReplacerDebug("the main page is not processed");
        return;
    }

    TStringBuf host = CutWWWPrefix(GetOnlyHost(ctx.Url));
    for (const TString& s : BLACKLISTED_HOSTS) {
        if (host == s) {
            manager->ReplacerDebug("the host is blacklisted");
            return;
        }
    }

    if (movie->ContainsBadText()) {
        manager->ReplacerDebug("html markup is found in the Movie fields");
        return;
    }

    ELanguage localeLang = ctx.DocLangId;
    if (localeLang == LANG_UNK) {
        localeLang = ctx.Cfg.GetUILang();
    }
    const NLemmer::TAlphabetWordNormalizer* wordNormalizer =
        NLemmer::GetAlphaRules(localeLang);
    NSchemaOrg::TMovieSents movieSents(localeLang,
        [=](TUtf16String& s) { wordNormalizer->ToTitle(s); },
        [=](TUtf16String& s) { wordNormalizer->ToLower(s); });
    movieSents.FormatMovie(*movie, ctx.Cfg.ExpFlagOn("movie_rating"));

    TMultiCutResult extDesc;
    extDesc = CutDescription(movieSents.GetResult(), ctx, ctx.LenCfg.GetMaxSpecSnipLen());
    TUtf16String headline = extDesc.Short;
    if (!headline) {
        manager->ReplacerDebug("Movie fields are empty");
        return;
    }

    TReplaceResult res;
    res.UseText(extDesc, HEADLINE_SRC);

    if (headline.size() < MIN_MOVIE_SNIP_LENGTH) {
        manager->ReplacerDebug("headline is too short", TReplaceResult().UseText(headline, HEADLINE_SRC));
        return;
    }

    TUtf16String newTitle;
    if (host == HOST_IMDB) {
        newTitle = movie->GetIMDBTitle(ctx.NaturalTitle.GetTitleString(), ctx.Cfg.GetUILang());
    }
    if (newTitle) {
        res.UseTitle(MakeSpecialTitle(newTitle, ctx.Cfg, ctx.QueryCtx));
    }

    manager->Commit(res, MRK_SCHEMA_MOVIE);
}

} // namespace NSnippets
