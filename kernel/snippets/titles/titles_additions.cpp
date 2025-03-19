#include "titles_additions.h"
#include "url_titles.h"

#include <kernel/snippets/titles/make_title/util_title.h>

#include <kernel/snippets/archive/view/view.h>

#include <kernel/snippets/config/config.h>

#include <kernel/snippets/custom/forums_handler/forums_handler.h>
#include <kernel/snippets/custom/opengraph/og.h>
#include <kernel/snippets/custom/remove_emoji/remove_emoji.h>

#include <kernel/snippets/qtree/query.h>

#include <kernel/snippets/sent_info/beautify.h>
#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/snippets/sent_match/callback.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/lang_check.h>

#include <kernel/snippets/simple_textproc/capital/capital.h>

#include <kernel/snippets/smartcut/consts.h>

#include <kernel/snippets/urlmenu/dump/dump.h>

#include <kernel/matrixnet/mn_sse.h>
#include <kernel/web_factors_info/factors_gen.h>

#include <kernel/lemmer/alpha/abc.h>
#include <library/cpp/unicode/punycode/punycode.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/ascii.h>
#include <util/charset/unidata.h>
#include <util/string/cast.h>

extern const NMatrixnet::TMnSseInfo staticMnRegions;

namespace NSnippets
{

static TUrlMenuVector GetArcMenu(const TString& urlmenu) {
    if (urlmenu.empty()) {
        return TUrlMenuVector();
    }
    TUrlMenuVector arcMenu;
    if (!NUrlMenu::Deserialize(arcMenu, urlmenu)) {
        return TUrlMenuVector();
    }
    return arcMenu;
}

TSnipTitleSupplementer::TSnipTitleSupplementer(const TConfig& cfg, const TQueryy& query, const TMakeTitleOptions& options, ELanguage lang,
                                               const TString& url, const TString& urlmenu,  const TDefinition& naturalTitleDefinition, bool questionTitle, ISnippetsCallback& callback)
    : Cfg(cfg)
    , Query(query)
    , Options(options)
    , ForbidCuttingOptions(options)
    , Lang(lang)
    , Url(url)
    , ArcMenu(GetArcMenu(urlmenu))
    , NaturalTitleDefinition(naturalTitleDefinition)
    , QuestionTitle(questionTitle)
    , Callback(callback)
{
    Options.DefinitionMode = TDM_USE;
    ForbidCuttingOptions.TitleGeneratingAlgo = TGA_NAIVE;
    ForbidCuttingOptions.DefinitionMode = TDM_IGNORE;
    ForbidCuttingOptions.AllowBreakMultiToken = true;
}

static inline void PrintTitleDebugOutput(const TSnipTitle& resBefore, const TSnipTitle& resAfter, ISnippetsCallback& callback, const TString additionName) {
    if (!!callback.GetDebugOutput()) {
        callback.GetDebugOutput()->Print(false, "%s: \"%s\" -> \"%s\"", additionName.data(), WideToUTF8(resBefore.GetTitleString()).data(), WideToUTF8(resAfter.GetTitleString()).data());
    }
}

static const TUtf16String FORUM_CYR(u"форум");
static const TUtf16String FORUM_LAT(u"forum");

static const TString FORUM_URL_SUBTOKENS[] = {
   "/forum/",
   "/forums/",
   ".forum.",
   "viewtopic.php",
   "viewforum.php",
   "showthread",
   "showforum",
   "showtopic",
};

static const TString FORUM_URL_PREFIX("forum.");

static bool IsForum(const TForumMarkupViewer& forumsViewer, const TString& url, bool tunedForumClassifier) {
    if (tunedForumClassifier) {
        if (forumsViewer.PageKind != TForumMarkupViewer::NONE && forumsViewer.IsValid()) {
            return true;
        }
        for (size_t i = 0; i < Y_ARRAY_SIZE(FORUM_URL_SUBTOKENS); ++i) {
            if (url.Contains(FORUM_URL_SUBTOKENS[i])) {
                return true;
            }
        }
        return CutSchemePrefix(url).StartsWith(FORUM_URL_PREFIX);
    } else {
        return (forumsViewer.PageKind != TForumMarkupViewer::NONE);
    }
}

static const TUtf16String TORRENT_CYR(u"торрент");
static const TUtf16String TORRENT_LAT(u"torrent");

static bool IsTorrentTitle(const TSnipTitle& res) {
    TUtf16String fullTitleStringLowered = res.GetSentsInfo() ? res.GetSentsInfo()->Text : res.GetTitleString();
    fullTitleStringLowered.to_lower();
    return fullTitleStringLowered.Contains(TORRENT_CYR)
           || fullTitleStringLowered.Contains(TORRENT_LAT);
}

void TSnipTitleSupplementer::AddForumToTitle(TSnipTitle& res, const TForumMarkupViewer& forumsViewer)
{
    if (!IsForum(forumsViewer, Url, Cfg.TunedForumClassifier())) {
        return;
    }
    if (Cfg.TunedForumClassifier() && (GetOnlyHost(Url) == "rutracker.org" || IsTorrentTitle(res))) {
        return;
    }
    TUtf16String forumString;
    switch (Lang) {
        case LANG_RUS:
        case LANG_UKR:
        case LANG_BEL:
        case LANG_KAZ:
            forumString = FORUM_CYR;
            break;
        case LANG_ENG:
        case LANG_TUR:
            forumString = FORUM_LAT;
            break;
        default:
            return;
    }
    Y_ASSERT(forumString.size());
    ToUpper(forumString.begin(), 1);
    TUtf16String titleStringLowered(res.GetTitleString());
    titleStringLowered.to_lower();
    if (titleStringLowered.find(FORUM_CYR) != TUtf16String::npos || titleStringLowered.find(FORUM_LAT) != TUtf16String::npos) {
        return;
    }
    TUtf16String titleStringWithForum = res.GetTitleString();
    EraseFullStop(titleStringWithForum);
    titleStringWithForum += GetTitleSeparator(Cfg) + forumString;
    TSnipTitle resWithForum = MakeTitle(titleStringWithForum, Cfg, Query, ForbidCuttingOptions);
    if (!resWithForum.GetTitleString().empty()) {
        PrintTitleDebugOutput(res, resWithForum, Callback, "Forum Title Addition");
        res = resWithForum;
    }
}

static const TUtf16String CATALOG_CYR(u"каталог");
static const TUtf16String CATALOG_ENG(u"catalog");
static const TUtf16String CATALOG_TUR(u"katalog");

static const TString CATALOG_URL_TOKENS[] = {
    "/catalog/",
    "/catalogue/",
    "/katalog/",
};

static constexpr double MAX_CATALOG_TITLE_LENGTH_RATIO = 0.4;

void TSnipTitleSupplementer::AddCatalogToTitle(TSnipTitle& res) {
    bool urlContainsCatalogToken = false;
    for (size_t i = 0; i < Y_ARRAY_SIZE(CATALOG_URL_TOKENS); ++i) {
        if (Url.Contains(CATALOG_URL_TOKENS[i])) {
            urlContainsCatalogToken = true;
            break;
        }
    }
    if (!urlContainsCatalogToken) {
        return;
    }
    TUtf16String catalogString;
    switch (Lang) {
        case LANG_RUS:
        case LANG_UKR:
        case LANG_BEL:
        case LANG_KAZ:
            catalogString = CATALOG_CYR;
            break;
        case LANG_ENG:
            catalogString = CATALOG_ENG;
            break;
        case LANG_TUR:
            catalogString = CATALOG_TUR;
            break;
        default:
            return;
    }
    if (res.GetPixelLength() > MAX_CATALOG_TITLE_LENGTH_RATIO * Options.PixelsInLine) {
        return;
    }
    TUtf16String titleStringLowered = res.GetTitleString();
    titleStringLowered.to_lower();
    if (titleStringLowered.Contains(CATALOG_CYR) ||
        titleStringLowered.Contains(CATALOG_ENG) ||
        titleStringLowered.Contains(CATALOG_TUR))
    {
        return;
    }
    TUtf16String titleStringWithCatalog = res.GetTitleString();
    Y_ASSERT(catalogString.size());
    ToUpper(catalogString.begin(), 1);
    EraseFullStop(titleStringWithCatalog);
    titleStringWithCatalog += GetTitleSeparator(Cfg) + catalogString;
    TSnipTitle resWithCatalog = MakeTitle(titleStringWithCatalog, Cfg, Query, ForbidCuttingOptions);
    if (!resWithCatalog.GetTitleString().empty()) { // just for case
        PrintTitleDebugOutput(res, resWithCatalog, Callback, "Catalog Title Addition");
        res = resWithCatalog;
    }
}

static const TUtf16String GOOD_URLMENU_SUBTOKENS_RU[] = {
    u"форум",
    u"каталог",
    u"новости",
    u"статьи",
    u"фильмы",
    u"видео",
    u"сообщества",
    u"рецепты",
    u"фото",
    u"игры",
    u"вакансии",
    u"сериалы",
    u"отзывы",
    u"картинки",
    u"объявления",
};

static const TUtf16String GOOD_URLMENU_SUBTOKENS_TR[] = {
    u"video",
    u"program",
    u"müzik",
    u"oyun",
    u"foto",
    u"ilan",
    u"kitap",
    u"haber",
    u"canli",
    u"dizi",
    u"duyuru",
    u"forum",
    u"fragman",
    u"tv",
    u"blog",
    u"download",
    u"resim",
};

static bool ContainsStringFromArray(const TUtf16String& token, const TUtf16String* goodSubtokens, size_t goodSubtokensNumber) {
    for (size_t i = 0; i < goodSubtokensNumber; ++i)
        if (token.Contains(goodSubtokens[i])) {
            return true;
        }
    return false;
}

static bool ContainsGoodSubtokens(const TUtf16String& token, ELanguage lang) {
    TUtf16String tokenLowered(token);
    tokenLowered.to_lower();
    switch (lang) {
        case LANG_RUS:
            return ContainsStringFromArray(tokenLowered, GOOD_URLMENU_SUBTOKENS_RU, Y_ARRAY_SIZE(GOOD_URLMENU_SUBTOKENS_RU));
        case LANG_TUR:
            return ContainsStringFromArray(tokenLowered, GOOD_URLMENU_SUBTOKENS_TR, Y_ARRAY_SIZE(GOOD_URLMENU_SUBTOKENS_TR));
        default:
            return false;
    }
}

static constexpr size_t MAX_SPACES_NUMBER = 2;

static bool ReadableUrlmenuToken(const TUtf16String& token) {
    if (token.empty()) {
        return false;
    }
    size_t spaces = 0;
    for (size_t i = 0; i < token.size(); ++i) {
        auto symbol = token[i];
        if (!IsSpace(symbol) && !IsAlpha(symbol) && !IsDigit(symbol)) {
            return false;
        }
        if (IsSpace(symbol)) {
            ++spaces;
        }
        if (i > 0 && IsUpper(symbol) && !IsSpace(token[i - 1])) {
            return false;
        }
    }
    return spaces <= MAX_SPACES_NUMBER;
}

static constexpr size_t MAX_TOKEN_LENGTH = 15;
static constexpr double MAX_URLMENU_TITLE_LENGTH_RATIO = 0.7;
static constexpr double MIN_URLMENU_SIMILARITY_RATIO = 0.7;

void TSnipTitleSupplementer::AddUrlmenuToTitle(TSnipTitle& res)
{
    if (Lang != LANG_RUS && Lang != LANG_TUR) {
        return;
    }
    if (GetOnlyHost(Url) == "www.youtube.com") {
        return;
    }
    if (ArcMenu.empty()) {
         return;
    }
    if (res.GetPixelLength() > MAX_URLMENU_TITLE_LENGTH_RATIO * Options.PixelsInLine) {
        return;
    }
    size_t total = ArcMenu.size();
    if (total != 3)
        return;
    const TUtf16String& urlmenuToken = ArcMenu[1].second;
    if (urlmenuToken.size() > MAX_TOKEN_LENGTH) {
        return;
    }
    if (!ReadableUrlmenuToken(urlmenuToken)) {
        return;
    }
    TUtf16String titleWithUrlmenu = res.GetTitleString();
    if (SimilarTitleStrings(res.GetTitleString(), urlmenuToken, MIN_URLMENU_SIMILARITY_RATIO)) {
        return;
    }
    EraseFullStop(titleWithUrlmenu);
    titleWithUrlmenu += GetTitleSeparator(Cfg) + urlmenuToken;
    TSnipTitle resWithUrlmenu = MakeTitle(titleWithUrlmenu, Cfg, Query, ForbidCuttingOptions);
    if (!resWithUrlmenu.GetTitleString().empty() &&
        (resWithUrlmenu.GetSynonymsCount() > res.GetSynonymsCount() || ContainsGoodSubtokens(urlmenuToken, Lang)))
    {
        PrintTitleDebugOutput(res, resWithUrlmenu, Callback, "Urlmenu Title Addition");
        res = resWithUrlmenu;
    }
}

static constexpr double MIN_URL_SIMILARITY_RATIO = 0.7;
static constexpr double MIN_URL_LENGTH = 7;
static constexpr double MAX_URL_TAIL_OVERBALANCING = 6;
static constexpr double MAX_URL_TITLE_OVERBALANCING = 3;

void TSnipTitleSupplementer::AddHostNameToUrlTitle(TSnipTitle& res) {
    TUtf16String titleWithHostName = res.GetTitleString();
    if (!LooksLikeUrl(titleWithHostName)) {
        return;
    }
    TUtf16String urlTail = UTF8ToWide(GetPathAndQuery(Url, false));
    if (!SimilarTitleStrings(titleWithHostName, urlTail, MIN_URL_SIMILARITY_RATIO)) {
        return;
    }
    if (titleWithHostName.size() < MIN_URL_LENGTH ||
        urlTail.size() > titleWithHostName.size() + MAX_URL_TAIL_OVERBALANCING ||
        titleWithHostName.size() > urlTail.size() + MAX_URL_TITLE_OVERBALANCING)
    {
        return;
    }
    TUtf16String host = ForcePunycodeToHostName(CutWWWPrefix(GetOnlyHost(Url)));
    switch (Cfg.GetUILang()) {
        case LANG_RUS:
            titleWithHostName = titleWithHostName + u" на " + host;
            break;
        case LANG_ENG:
            titleWithHostName = titleWithHostName + u" at " + host;
            break;
        case LANG_TUR:
            switch (Cfg.HostNameForUrlTitles()) {
                case 1:
                    titleWithHostName = host + u" sitesindeki " + titleWithHostName + u" sayfası";
                    break;
                case 2:
                    titleWithHostName = titleWithHostName + u" sitesinde " + host;
                    break;
                case 3:
                    titleWithHostName = host + u": " + titleWithHostName;
                    break;
                case 4:
                    titleWithHostName = titleWithHostName + u" (" + host + u")";
                    break;
                default:
                    return;
            }
            break;
        default:
            return;
    }
    TSnipTitle resWithHostName = MakeTitle(titleWithHostName, Cfg, Query, ForbidCuttingOptions);
    if (!resWithHostName.GetTitleString().empty()) {
        PrintTitleDebugOutput(res, resWithHostName, Callback, "Hostname To Url Title Addition");
        res = resWithHostName;
    }
}

namespace {
    static const TUtf16String PIPE_SUFFIX(u" | ");
    static const TUtf16String COLONS_SUFFIX(u" :: ");
    static const TUtf16String DASH_SUFFIX(u" - ");
    static const TUtf16String HYPHEN_SUFFIX(u" — ");

    static bool IsPopularHost(const TConfig& config) {
        float hostRank;
        return config.GetRankingFactor(FI_VISITORS_RETURN_MONTH_NUMBER, hostRank) && hostRank >= config.GetPopularHostRank();
    }
}

void TSnipTitleSupplementer::AddPopularHostNameToTitle(TSnipTitle& res) {
    if (!IsPopularHost(Cfg))
        return;

    TUtf16String host = ForcePunycodeToHostName(CutWWWPrefix(GetOnlyHost(Url)));
    ToUpper(host.begin(), 1);
    TUtf16String newTitle = res.GetTitleString();
    switch (Cfg.GetTitlePartDelimiterType()) {
        case DELIM_PIPE:
            newTitle += PIPE_SUFFIX;
            break;
        case DELIM_COLONS:
            newTitle += COLONS_SUFFIX;
            break;
        case DELIM_DASH:
            newTitle += DASH_SUFFIX;
            break;
        case DELIM_HYPHEN:
            newTitle += HYPHEN_SUFFIX;
            break;
        default:
            return;
    }
    newTitle += host;
    TSnipTitle newSnipTitle = MakeTitle(newTitle, Cfg, Query, ForbidCuttingOptions);
    if (!newSnipTitle.GetTitleString().empty()) {
        PrintTitleDebugOutput(res, newSnipTitle, Callback, "Popular Hostname Title Addition");
        res = newSnipTitle;
    }
}

static const struct { TString Url; TUtf16String Definition; } NEWS_TITLES_DEFINITIONS[] = {
    {"www.haberler.com",       u"Haberler.com"},
    {"www.fanatik.com.tr",     u"Fanatik"},
    {"www.haberturk.com",      u"Habertürk"},
    {"www.hurriyet.com.tr",    u"Hürriyet"},
    {"www.samanyoluhaber.com", u"Samanyolu Haber"},
    {"www.sondakika.com",      u"Sondakika.com"},
    {"www.internethaber.com",  u"İnternet Haber"},
    {"www.ensonhaber.com",     u"Ensonhaber.com"},
    {"www.milliyet.com.tr",    u"Milliyet"},
    {"www.haber7.com",         u"Haber7.com"},
    {"www.haber3.com",         u"Haber3.com"},
    {"www.gazetevatan.com",    u"Gazete Vatan"},
    {"www.mynet.com/haber",    u"Mynet Haber"},
    {"www.trthaber.com",       u"TRT Haber"},
    {"www.sabah.com.tr",       u"Sabah"},
    {"www.dha.com.tr",         u"DHA"},
    {"www.resmigazete.gov.tr", u"Resmi Gazete"},
    {"www.donanimhaber.com",   u"DonanımHaber"},
    {"www.radikal.com.tr",     u"Radikal"},
    {"www.stargazete.com",     u"Stargazete.com"},
    {"www.takvim.com.tr",      u"Takvim"},
    {"www.aksam.com.tr",       u"Akşam"},
    {"www.zaman.com.tr",       u"Zaman"},
    {"www.iha.com.tr",         u"İHA"},
    {"www.ntvmsnbc.com",       u"ntvmsnbc"},
    {"www.cnnturk.com",        u"CNN Türk"},
    {"www.aa.com.tr",          u"Anadolu Ajansı"},
};

static TUtf16String GetLatinAlphasLowered(const TUtf16String& line, size_t prefixSize) {
    TUtf16String latinAlphasLowered;
    for (size_t i = 0; i < prefixSize; ++i) {
       if (!IsAsciiAlpha(line[i])) {
           continue;
       }
       latinAlphasLowered.push_back(ToLower(line[i]));
    }
    return latinAlphasLowered;
}

static const TUtf16String HABER_TOKEN(u"Haber");
static const TUtf16String DOT_COM_TOKEN(u".com");

static bool TitleContainsDefinition(const TUtf16String& definition, const TUtf16String& titleString) {
    size_t definitionSize = definition.size();
    if (definition.EndsWith(HABER_TOKEN)) {
        definitionSize -= HABER_TOKEN.size();
    } else {
        if (definition.EndsWith(DOT_COM_TOKEN)) {
            definitionSize -= DOT_COM_TOKEN.size();
        }
    }
    TUtf16String definitionLatinAlphasLowered(GetLatinAlphasLowered(definition, definitionSize));
    TUtf16String titleLatinAlphasLowered(GetLatinAlphasLowered(titleString, titleString.size()));
    return titleLatinAlphasLowered.Contains(definitionLatinAlphasLowered);
}

inline bool IsInnerUrl(const TString& url, const TString& host)
{
    TString hostPrefix = host + "/";
    TStringBuf tmpUrl = CutSchemePrefix(url);
    return tmpUrl.StartsWith(hostPrefix) && tmpUrl.size() > hostPrefix.size();
}

void TSnipTitleSupplementer::AddDefinitionToNewsTitle(TSnipTitle& res) {
    for (const auto& urlDefinition : NEWS_TITLES_DEFINITIONS) {
        if (!IsInnerUrl(Url, urlDefinition.Url)) {
            continue;
        }
        TUtf16String titleWithDefinition = res.GetTitleString();
        if (TitleContainsDefinition(urlDefinition.Definition, titleWithDefinition)) {
            return;
        }
        titleWithDefinition = urlDefinition.Definition + u": " + titleWithDefinition;
        TSnipTitle resWithDefinition = MakeTitle(titleWithDefinition, Cfg, Query, Options);
        if (resWithDefinition.GetSynonymsCount() >= res.GetSynonymsCount()) {
            PrintTitleDebugOutput(res, resWithDefinition, Callback, "Definition To News Title Addition");
            res = resWithDefinition;
        }
        break;
    }
}

static constexpr double MAX_SPLITTED_HOSTNAME_TITLE_LENGTH_RATIO = 0.8;

void TSnipTitleSupplementer::AddSplittedHostNameToTitle(TSnipTitle& res, const TSentsInfo* sentsInfo) {
    if (!res.GetTitleString().size()) {
        return;
    }
    if (Options.TitleCutMethod == TCM_SYMBOL || res.GetPixelLength() > MAX_SPLITTED_HOSTNAME_TITLE_LENGTH_RATIO * Options.PixelsInLine) {
        return;
    }
    TUrlTitleTokenizer urlTitleTokenizer(Query, sentsInfo, Lang, Cfg.GetStopWordsFilter());
    TUtf16String host = ForcePunycodeToHostName(GetOnlyHost(Url));
    if (urlTitleTokenizer.GenerateUrlBasedTitle(host, res.GetTitleString())) {
        TUtf16String titleWithHost = res.GetTitleString() + GetTitleSeparator(Cfg) + host;
        TSnipTitle resWithHost = MakeTitle(titleWithHost, Cfg, Query, ForbidCuttingOptions);
        if (!resWithHost.GetTitleString().empty() && resWithHost.GetSynonymsCount() > res.GetSynonymsCount()) {
            PrintTitleDebugOutput(res, resWithHost, Callback, "Splitted Hostname To Title Addition");
            res = resWithHost;
        }
    }
}

static bool TitleIsBetterThanUrl(const TSnipTitle& res, const TSnipTitle& resSplitted) {
    if (!resSplitted.GetTitleString().size())
         return false;
    if (resSplitted.GetMatchUserIdfSum() > res.GetMatchUserIdfSum() + 1E-7)
         return true;
    return (resSplitted.GetMatchUserIdfSum() > res.GetMatchUserIdfSum() - 1E-7 && resSplitted.GetLogMatchIdfSum() > res.GetLogMatchIdfSum() - 1E-7);
}

void TSnipTitleSupplementer::GenerateUrlBasedTitle(TSnipTitle& res, const TSentsInfo* sentsInfo) {
    if (res.GetTitleString().size()) {
        return;
    }
    TUrlTitleTokenizer urlTitleTokenizer(Query, sentsInfo, Lang, Cfg.GetStopWordsFilter());
    TUtf16String wurl = ConvertUrlToTitle(Url);
    TMakeTitleOptions newOptions = Options;
    newOptions.DefinitionMode = TDM_IGNORE;
    newOptions.AllowBreakMultiToken = true;
    res = MakeTitle(wurl, Cfg, Query, newOptions);
    if (!!Callback.GetDebugOutput()) {
        Callback.GetDebugOutput()->Print(false, "Empty title was replaced by url (%s)", WideToUTF8(res.GetTitleString()).data());
    }
    if (!Cfg.GenerateUrlBasedTitles())
        return;
    if (urlTitleTokenizer.GenerateUrlBasedTitle(wurl)) {
        newOptions.TitleGeneratingAlgo = TGA_NAIVE;
        TSnipTitle resSplitted = MakeTitle(wurl, Cfg, Query, newOptions);
        if (!resSplitted.GetTitleString().size()) {
            size_t lastTokenStart = wurl.rfind('|');
            if (lastTokenStart != TUtf16String::npos && lastTokenStart + 2 < wurl.size()) {
                TUtf16String wurlLastToken = wurl.substr(lastTokenStart + 2);
                resSplitted = MakeTitle(wurlLastToken, Cfg, Query, newOptions);
            }
        }
        if (TitleIsBetterThanUrl(res, resSplitted)) {
            PrintTitleDebugOutput(res, resSplitted, Callback, "Url Based Title");
            res = resSplitted;
        }
    }
}

static constexpr size_t MIN_BNA_TITLE_LENGTH = 8;
static constexpr size_t MAX_URLMENU_WORD_LENGTH = 10;

static bool IsAlphaOrDigit(wchar16 symbol) {
    return IsAlpha(symbol) || IsDigit(symbol);
}

void TSnipTitleSupplementer::GenerateBNATitle(TSnipTitle& res) {
    const TUtf16String& titleString(res.GetTitleString());
    if (!titleString.EndsWith(BOUNDARY_ELLIPSIS) || titleString.size() <= 3) {
        return;
    }
    if (!res.GetSentsInfo()) {
        return; // just for case
    }
    const TUtf16String& fullTitleString(res.GetSentsInfo()->Text);
    if (!fullTitleString.StartsWith(TWtringBuf(titleString.data(), titleString.size() - 3))) {
        return; // just for case
    }
    size_t prefixLen = 0;
    for (size_t i = MIN_BNA_TITLE_LENGTH; i + 1 < fullTitleString.size(); ++i) {
        wchar16 symbol = fullTitleString[i];
        if (i >= titleString.size() - 3 && IsAlphaOrDigit(symbol)) {
            break;
        }
        if (IsAlphaOrDigit(symbol) || IsSpace(symbol)) {
            continue;
        }
        if (IsAlphaOrDigit(fullTitleString[i + 1]) &&
            symbol != '(' && symbol != '[' && symbol != '{') {
            continue;
        }
        if (IsAlphaOrDigit(fullTitleString[i - 1]) && symbol == '.') {
            continue;
        }
        prefixLen = i;
        if (symbol == '\"' || symbol == '\'' || symbol == '!' || symbol == '?' ||
            symbol == ')' || symbol == ']' || symbol == '}') {
            ++prefixLen;
        }
        break;
    }
    if (prefixLen > 0) {
        if (titleString[prefixLen - 1] == '.') {
            --prefixLen;
        }
        TSnipTitle resPrefix = MakeTitle(titleString.substr(0, prefixLen), Cfg, Query, ForbidCuttingOptions);
        if (resPrefix.GetTitleString().size()) {
            PrintTitleDebugOutput(res, resPrefix, Callback, "BNA Title (prefix of the natural one)");
            res = resPrefix;
            return;
        }
    }
    if (ArcMenu.size() <= 1) {
        return;
    }
    TUtf16String lastUrlmenuToken(ArcMenu.back().second);
    if (lastUrlmenuToken.Contains(u"/") || lastUrlmenuToken.Contains(u"?") || lastUrlmenuToken.Contains(u"%")) {
        return;
    }
    if (lastUrlmenuToken.size() > MAX_URLMENU_WORD_LENGTH && !lastUrlmenuToken.Contains(u" ")) {
        return;
    }
    TSnipTitle resFromUrlmenu = MakeTitle(lastUrlmenuToken, Cfg, Query, ForbidCuttingOptions);
    if (resFromUrlmenu.GetTitleString().size()) {
        PrintTitleDebugOutput(res, resFromUrlmenu, Callback, "BNA Title (based on urlmenu)");
        res = resFromUrlmenu;
    }
}

static constexpr size_t MAX_METADESCR_LENGTH = 100;
static constexpr double METADESCR_SIMILARITY_RATIO = 0.4;
static constexpr double MAX_TITLE_PIXEL_LEN_RATIO_TR = 0.95;
static constexpr double TITLE_LENGTHS_RATIO = 1.4;
static constexpr double EXTREMELY_SHORT_TITLE_LENGTH_RATIO = 0.3;
static constexpr double RELATIVELY_SHORT_TITLE_LENGTH_RATIO = 0.5;

static const TString IGNORE_META_DESCRIPTION_HOSTS[] = {
    "vk.com",
    "facebook.com",
    "twitter.com",
    "youtube.com",
};

static float TitleLines(TSnipTitle& res, const TMakeTitleOptions& options) {
    return ceil(res.GetPixelLength() / (options.PixelsInLine + 1));
}

void TSnipTitleSupplementer::GenerateMetaDescriptionBasedTitle(TSnipTitle& res, const TMetaDescription& metaDescr) {
    if (!metaDescr.MayUse()) {
        return;
    }
    if (Lang == LANG_UNK || !HasWordsOfAlphabet(metaDescr.GetTextCopy(), Lang)) {
        return;
    }

    TString host = ToString(CutWWWPrefix(GetOnlyHost(Url)));
    for (size_t i = 0; i < Y_ARRAY_SIZE(IGNORE_META_DESCRIPTION_HOSTS); ++i) {
        if (host.Contains(IGNORE_META_DESCRIPTION_HOSTS[i])) {
             return;
        }
    }

    const TUtf16String& metaDescrString(metaDescr.GetTextCopy());
    if (metaDescrString.size() > MAX_METADESCR_LENGTH) {
        return;
    }

    bool similarStrings = SimilarTitleStrings(res.GetTitleString(), metaDescrString, METADESCR_SIMILARITY_RATIO);
    TSnipTitle resMetaDescr;
    if (!similarStrings) {
        TUtf16String titleStringConcatenation(res.GetTitleString());
        EraseFullStop(titleStringConcatenation);
        titleStringConcatenation += GetTitleSeparator(Cfg) + metaDescrString;
        resMetaDescr = MakeTitle(titleStringConcatenation, Cfg, Query, ForbidCuttingOptions);
        if (!resMetaDescr.GetTitleString().empty()) {
           PrintTitleDebugOutput(res, resMetaDescr, Callback, "Meta Description Title Addition");
           res = resMetaDescr;
           return;
        }
    }

    TMakeTitleOptions newOptions = ForbidCuttingOptions;
    if (!Cfg.UseTurkey()) {
        newOptions.TitleGeneratingAlgo = TGA_PREFIX;
    } else {
        newOptions.PixelsInLine *= MAX_TITLE_PIXEL_LEN_RATIO_TR;
    }
    resMetaDescr = MakeTitle(metaDescrString, Cfg, Query, newOptions);
    const TDefinition& resDefinition = res.GetDefinition();
    const TDefinition& resMetaDescrDefinition = resMetaDescr.GetDefinition();
    if (Cfg.UseTurkey() &&
        resDefinition.GetDefinitionType() != NOT_FOUND &&
        resMetaDescrDefinition.GetDefinitionType() == NOT_FOUND &&
        resMetaDescr.GetSynonymsCount() <= res.GetSynonymsCount())
    {
        return;
    }
    if (resMetaDescr.GetLogMatchIdfSum() < res.GetLogMatchIdfSum()) {
        return;
    }
    bool fuzzySubstring = (similarStrings && resMetaDescr.GetPixelLength() > TITLE_LENGTHS_RATIO * res.GetPixelLength());
    bool shouldChange = false;
    bool equalLengthInLines = (TitleLines(res, newOptions) == TitleLines(resMetaDescr, newOptions));
    if (resMetaDescr.GetTitleString() == metaDescrString && (!Cfg.ExpFlagOn("metadescr_titles") || equalLengthInLines)) {
        if (resMetaDescr.GetSynonymsCount() > res.GetSynonymsCount()
            || resMetaDescr.GetSynonymsCount() == res.GetSynonymsCount() && fuzzySubstring) {
            shouldChange = true;
        }
    } else {
        if (res.GetPixelLength() < EXTREMELY_SHORT_TITLE_LENGTH_RATIO * Options.PixelsInLine
            && resMetaDescr.GetSynonymsCount() >= res.GetSynonymsCount()
            && fuzzySubstring) {
            shouldChange = true;
        } else {
            if (res.GetPixelLength() < RELATIVELY_SHORT_TITLE_LENGTH_RATIO * Options.PixelsInLine
                && resMetaDescr.GetSynonymsCount() > res.GetSynonymsCount()) {
                shouldChange = true;
            }
        }
    }
    if (shouldChange) {
         PrintTitleDebugOutput(res, resMetaDescr, Callback, "Meta Description Based Title");
         res = resMetaDescr;
    }
}

static constexpr double MINIMAL_TITLE_LENGTH_RATIO = 1.3;
static constexpr double MINIMAL_UIDF_RATIO = 1.3;

static bool TwoLineTitleIsBetter(const TSnipTitle& res, const TSnipTitle& resTwoLine, bool questionTitle) {
    if (questionTitle) {
        return resTwoLine.GetMatchUserIdfSum() + 1E-7 > res.GetMatchUserIdfSum();
    }
    if (resTwoLine.GetMatchUserIdfSum() < res.GetMatchUserIdfSum() * MINIMAL_UIDF_RATIO) {
        return false;
    };
    return resTwoLine.GetSynonymsCount() >= res.GetSynonymsCount() + 2;
}

void TSnipTitleSupplementer::GenerateTwoLineTitle(TSnipTitle& res) {
    if (!Cfg.TwoLineTitles() && !QuestionTitle) {
        return;
    }

    float factorValue = 0.f;
    if (!Cfg.GetRankingFactor(FI_QCLASS_DOWNLOAD, factorValue) || factorValue > 0.9f) {
        return;
    }

    if (!res.GetSentsInfo()) {
        return; // just for case
    }
    TUtf16String fullTitleString(res.GetSentsInfo()->Text);
    if (fullTitleString.size() < res.GetTitleString().size() * MINIMAL_TITLE_LENGTH_RATIO) {
        return;
    }

    TMakeTitleOptions newOptions = ForbidCuttingOptions;
    newOptions.TitleGeneratingAlgo = TGA_PREFIX;
    newOptions.MaxTitleLengthInRows = 2.f;
    if (Cfg.CapitalizeEachWordTitleLetter()) {
        newOptions.PixelsInLine *= 0.95; // reserving some space for title capitalization
    }
    TSnipTitle resTwoLine = MakeTitle(fullTitleString, Cfg, Query, newOptions);
    if (TwoLineTitleIsBetter(res, resTwoLine, QuestionTitle)) {
        PrintTitleDebugOutput(res, resTwoLine, Callback, "Two Line Title Generator");
        res = resTwoLine;
    }
}

static const TUtf16String TURKISH_REGIONS[] = {
    u"İstanbul",
    u"Ankara",
    u"İzmir",
    u"Bursa",
    u"Adana",
    u"Gaziantep",
    u"Konya",
    u"Antalya",
    u"Kayseri",
    u"Diyarbakır",
    u"Mersin",
    u"Eskişehir",
    u"Sakarya",
    u"Samsun",
    u"Şanlıurfa",
    u"Denizli",
    u"Kahramanmaraş",
    u"Malatya",
    u"Erzurum",
    u"Van",
    u"Batman",
    u"Elazığ",
    u"Sivas",
    u"Manisa",
    u"İzmit",
};

static constexpr double MIN_REGION_SIMILARITY_RATIO = 0.8;

void TSnipTitleSupplementer::AddUrlRegionToTitle(TSnipTitle& res) {
    if (Lang != LANG_TUR) {
        return;
    }
    for (size_t regionNum = 0; regionNum < Y_ARRAY_SIZE(TURKISH_REGIONS); ++regionNum)  {
        TString tokenToSearch = "/" + GetLatinLettersAndDigits(TURKISH_REGIONS[regionNum]) + "/";
        if (!Url.Contains(tokenToSearch)) {
            continue;
        }
        if (SimilarTitleStrings(res.GetTitleString(), TURKISH_REGIONS[regionNum], MIN_REGION_SIMILARITY_RATIO)) {
            return;
        }
        TUtf16String titleWithRegion = res.GetTitleString() + GetTitleSeparator(Cfg) + TURKISH_REGIONS[regionNum];
        TSnipTitle resWithRegion = MakeTitle(titleWithRegion, Cfg, Query, ForbidCuttingOptions);
        if (!resWithRegion.GetTitleString().empty()) {
            PrintTitleDebugOutput(res, resWithRegion, Callback, "Url Region To Title Addition");
            res = resWithRegion;
        }
        return;
    }
}

static bool IsRelevRegion(const TConfig& cfg) {
    if (cfg.MnRelevRegion()) {
        if (staticMnRegions.MaxFactorIndex() >= cfg.GetRankingFactorsCount()) {
            return false; // just for case
        }
        return staticMnRegions.DoCalcRelev(cfg.GetRankingFactors()) > cfg.MnRelevRegionThreshold();
    } else {
        float factorValue = 0.f;
        if (!cfg.GetRankingFactor(FI_AUX_TEXT_BM25, factorValue) || factorValue < 1E-7f)
        {
            return false;
        }
        if (cfg.GetRankingFactor(FI_URL_CLICKS_MAX_GEO_CITY_FRC_RATIO, factorValue) && factorValue > 0.6f)
        {
            return true;
        }
        if (cfg.GetRankingFactor(FI_DOPP_URL_SESSION_CLICKS_FRC_CITY, factorValue) && factorValue > 0.8f)
        {
            return true;
        }
        if (cfg.GetRankingFactor(FI_GEO_CITY_URL_REGION_CITY, factorValue) && factorValue > 1E-7f)
        {
            return true;
        }
        if (cfg.GetRankingFactor(FI_GEO_RELEV_REGION_CITY, factorValue) && factorValue > 1E-7f &&
            cfg.GetRankingFactor(FI_AUX_LINK_BM25, factorValue) && factorValue > 1E-7f)
        {
            return true;
        }
        return false;
    }
}

static const struct { TUtf16String RelevName; TUtf16String CorrectedName; } REGION_NAME_CORRECTIONS[] = {
    {u"санкт петербург", u"Санкт-Петербург"},
    {u"нижний новгород", u"Нижний Новгород"},
    {u"ростов на дону", u"Ростов-на-Дону"},
};

static const TString WIKIPEDIA_SUFFIX = ".wikipedia.org";

void TSnipTitleSupplementer::AddUserRegionToTitle(TSnipTitle& res) {
    if (!Query.RegionQuery.Get()) {
        return;
    }

    if (Cfg.UseTurkey()) {
        if (Lang != LANG_TUR || Cfg.GetUILang() != LANG_TUR) {
            return;
        }
    } else {
        if (Lang != LANG_RUS || Cfg.GetUILang() != LANG_RUS) {
            return;
        }
    }

    if (GetOnlyHost(Url).EndsWith(WIKIPEDIA_SUFFIX)) {
        return;
    }

    if (!IsRelevRegion(Cfg)) {
        return;
    }

    TUtf16String userRegionName(UTF8ToWide(Cfg.GetRelevRegionName()));
    if (userRegionName.empty()) {
        return;
    }
    if (userRegionName.Contains(' ')) {
        bool corrected = false;
        for (const auto& region : REGION_NAME_CORRECTIONS) {
            if (userRegionName == region.RelevName) {
                userRegionName = region.CorrectedName;
                corrected = true;
                break;
            }
        }
        if (!corrected) {
            return;
        }
    } else {
        Y_ASSERT(Query.RegionQuery.Get());
        if (!Query.RegionQuery->Form2Positions.contains(userRegionName)) {
            return;
        }
    }
    const NLemmer::TAlphabetWordNormalizer* wordNormalizer = NLemmer::GetAlphaRules(Lang);
    userRegionName[0] = wordNormalizer->ToUpper(userRegionName[0]);

    if (res.HasRegionMatch() || SimilarTitleStrings(res.GetTitleString(), userRegionName, MIN_REGION_SIMILARITY_RATIO)) {
        return;
    }

    TUtf16String titleWithRegion = res.GetTitleString() + u" - " + userRegionName;
    TSnipTitle resWithRegion = MakeTitle(titleWithRegion, Cfg, Query, ForbidCuttingOptions);
    if (!resWithRegion.GetTitleString().empty()) {
        PrintTitleDebugOutput(res, resWithRegion, Callback, "User Region To Title Addition");
        res = resWithRegion;
    }
}

static const TString HOST_DEFINITIONS_IN_TITLES = "host_definitions_in_titles";
static constexpr double MIN_DEFINITION_SIMILARITY_RATIO = 0.8;

void TSnipTitleSupplementer::AddHostDefinitionToTitle(TSnipTitle& res) {
    if (Cfg.IsPadReport()) {
        if (!Cfg.ExpFlagOn(HOST_DEFINITIONS_IN_TITLES)) {
            return;
        }
    } else {
        if (Cfg.ExpFlagOff(HOST_DEFINITIONS_IN_TITLES)) {
            return;
        }
    }
    if (NaturalTitleDefinition.GetDefinitionType() == NOT_FOUND) {
        return;
    }
    TUtf16String titleStringWithDefinition = res.GetTitleString();
    if (SimilarTitleStrings(titleStringWithDefinition, NaturalTitleDefinition.GetDefinitionString(), MIN_DEFINITION_SIMILARITY_RATIO)) {
        return;
    }
    if (NaturalTitleDefinition.GetDefinitionType() == FORWARD) {
        titleStringWithDefinition = NaturalTitleDefinition.GetDefinitionString() + titleStringWithDefinition;
    } else {
        titleStringWithDefinition += NaturalTitleDefinition.GetDefinitionString();
    }
    TSnipTitle resWithDefinition = MakeTitle(titleStringWithDefinition, Cfg, Query, ForbidCuttingOptions);
    if (!resWithDefinition.GetTitleString().empty()) {
        PrintTitleDebugOutput(res, resWithDefinition, Callback, "Definition To Title Addition");
        res = resWithDefinition;
    }
}

static constexpr int MIN_LENGTH_DIFF = 4;
static constexpr double DEFINITION_SIMILARITY_RATIO = 0.6;

static bool ShouldEliminateDefinition(const TSnipTitle res, const TSnipTitle resWithoutDefinition, const TConfig& cfg, bool questionTitle) {
    if (cfg.EliminateDefinitions() && !resWithoutDefinition.GetTitleString().empty()) {
        return true;
    }
    if (questionTitle && !resWithoutDefinition.GetTitleString().empty()) {
        return true;
    }
    if (resWithoutDefinition.GetSynonymsCount() > res.GetSynonymsCount()) {
        return true;
    }

    // SNIPPETS-4011
    if (!cfg.ExpFlagOn("eliminate_forward_definitions") &&
        res.GetDefinition().GetDefinitionType() == FORWARD)
    {
        return false;
    }

    if (!res.GetTitleString().Contains(resWithoutDefinition.GetTitleString())) {
        if (cfg.ExpFlagOn("eliminate_definitions_with_duplicates") &&
            SimilarTitleStrings(resWithoutDefinition.GetTitleString(), res.GetDefinition().GetDefinitionString(), DEFINITION_SIMILARITY_RATIO))
        {
            return true;
        }
        float hostPopularity = 0.f;
        if (resWithoutDefinition.GetLogMatchIdfSum() + 1E-7 > res.GetLogMatchIdfSum() &&
            cfg.GetRankingFactor(FI_REMOVED_98, hostPopularity) &&
            hostPopularity < cfg.DefinitionPopularityThreshold())
        {
            return true;
        }
    }
    return false;
}

void TSnipTitleSupplementer::EliminateDefinitionFromTitle(TSnipTitle& res) {
    const TDefinition& definition = res.GetDefinition();
    if (definition.GetDefinitionType() == NOT_FOUND) {
        return;
    }
    TUtf16String fullTitleString = res.GetSentsInfo() ? res.GetSentsInfo()->Text : res.GetTitleString();
    if (QuestionTitle && fullTitleString.size() < res.GetTitleString().size() + MIN_LENGTH_DIFF) {
        return;
    }
    TMakeTitleOptions newOptions = (QuestionTitle ? ForbidCuttingOptions : Options);
    newOptions.DefinitionMode = TDM_ELIMINATE;
    newOptions.Url = Url;
    newOptions.HostnamesForDefinitions = true;
    TSnipTitle resWithoutDefinition = MakeTitle(fullTitleString, Cfg, Query, newOptions);
    if (ShouldEliminateDefinition(res, resWithoutDefinition, Cfg, QuestionTitle)) {
        PrintTitleDebugOutput(res, resWithoutDefinition, Callback, "Eliminating Definition");
        res = resWithoutDefinition;
        if (QuestionTitle) {
            TUtf16String titleStringCapitalized(res.GetTitleString());
            if (titleStringCapitalized.empty()) {
                return; // just for case
            }
            ToUpper(titleStringCapitalized.begin(), 1);
            Y_ASSERT(res.GetTitleString().size());
            if (titleStringCapitalized[0] == res.GetTitleString()[0]) {
                return;
            }
            TSnipTitle resCapitalized = MakeTitle(titleStringCapitalized, Cfg, Query, ForbidCuttingOptions);
            if (!resCapitalized.GetTitleString().empty()) {
                PrintTitleDebugOutput(res, resWithoutDefinition, Callback, "Question title capitalization");
                res = resCapitalized;
            }
        }
    }
}

static constexpr double MIN_OG_TITLE_SIMILARITY_RATIO = 0.7;
static constexpr double MIN_OG_TITLE_LENGTH_RATIO = 0.8;
static constexpr double MAX_OG_SITENAME_SIMILARITY_RATIO = 0.4;

void TSnipTitleSupplementer::GenerateOpenGraphBasedTitle(TSnipTitle& res, const TDocInfos& docInfos) {
    TOgTitleData ogTitleData(docInfos);
    if (!ogTitleData.HasAttrs) {
        return;
    }
    const TUtf16String& ogTitle(ogTitleData.Title);
    const TUtf16String& ogSitename(ogTitleData.Sitename);
    TUtf16String titleString = res.GetTitleString();
    if (ogTitle.size()) {
        if (Cfg.EliminateDefinitions()) {
            if ((!ogTitle.StartsWith(titleString) || !Cfg.IsTitleContractionOk(ogTitle.size(), titleString.size())) &&
                (!ogSitename || !ogTitle.EndsWith(ogSitename)))
            {
                TSnipTitle resOpenGraphTitle = MakeTitle(ogTitle, Cfg, Query, ForbidCuttingOptions);
                if (!resOpenGraphTitle.GetTitleString().empty()) {
                    PrintTitleDebugOutput(res, resOpenGraphTitle, Callback, "Open Graph Title Substitution");
                    res = resOpenGraphTitle;
                }
            }
        } else if (titleString.StartsWith(BOUNDARY_ELLIPSIS) || titleString.EndsWith(BOUNDARY_ELLIPSIS)) {
            if (SimilarTitleStrings(titleString, ogTitle, MIN_OG_TITLE_SIMILARITY_RATIO)) {
                TSnipTitle resOpenGraphTitle = MakeTitle(ogTitle, Cfg, Query, ForbidCuttingOptions);
                if (!resOpenGraphTitle.GetTitleString().empty() &&
                    resOpenGraphTitle.GetSynonymsCount() >= res.GetSynonymsCount() &&
                    resOpenGraphTitle.GetLogMatchIdfSum() + 1E-7 > res.GetLogMatchIdfSum() &&
                    resOpenGraphTitle.GetPixelLength() > MIN_OG_TITLE_LENGTH_RATIO * res.GetPixelLength())
                {
                    PrintTitleDebugOutput(res, resOpenGraphTitle, Callback, "Open Graph Title Substitution");
                    res = resOpenGraphTitle;
                }
            }
        }
    }
    if (ogSitename.size()) {
        if (Cfg.EliminateDefinitions()) {
            return;
        }
        TUtf16String titleStringWithSitename = res.GetTitleString();
        if (LooksLikeUrl(ogSitename)) {
            return;
        }
        bool looksLikeHostname = true;
        for (size_t i = 0; i < ogSitename.size(); ++i) {
           if (!IsAsciiAlpha(ogSitename[i]) && !IsDigit(ogSitename[i])) {
               looksLikeHostname = false;
               break;
           }
        }
        if (looksLikeHostname) {
            return;
        }
        if (SimilarTitleStrings(titleStringWithSitename, ogSitename, MAX_OG_SITENAME_SIMILARITY_RATIO)) {
            return;
        }
        titleStringWithSitename += GetTitleSeparator(Cfg) + ogSitename;
        TSnipTitle resWithSitename = MakeTitle(titleStringWithSitename, Cfg, Query, ForbidCuttingOptions);
        if (!resWithSitename.GetTitleString().empty()) {
            PrintTitleDebugOutput(res, resWithSitename, Callback, "Open Graph Sitename To Title Addition");
            res = resWithSitename;
        }
    }
}

static bool IsBadSymbol(wchar16 symbol) {
    if (symbol > wchar16(0) && symbol <= wchar16(0xBF)) { // various useful symbols
        return false;
    }
    if (symbol == wchar16(0x2116)) { // №
        return false;
    }
    if (symbol == wchar16(0x2122)) { // ™
        return false;
    }
    return NUnicode::CharHasType(symbol, ULL(1) << (So_OTHER));
}

void TSnipTitleSupplementer::EraseBadSymbolsFromTitle(TSnipTitle& res) {
    const TUtf16String& titleString = res.GetTitleString();
    auto it = FindIf(titleString.begin(), titleString.end(), IsBadSymbol);
    if (it == titleString.end()) {
        return;
    }
    TUtf16String titleStringFiltered;
    for (wchar16 symbol : titleString) {
        if (!IsBadSymbol(symbol)) {
            titleStringFiltered.push_back(symbol);
        }
    }
    TMakeTitleOptions newOptions = ForbidCuttingOptions;
    newOptions.TitleGeneratingAlgo = TGA_PREFIX;
    TSnipTitle resFiltered = MakeTitle(titleStringFiltered, Cfg, Query, newOptions);
    if (resFiltered.GetLogMatchIdfSum() + 1E-7 > res.GetLogMatchIdfSum()) {
        PrintTitleDebugOutput(res, resFiltered, Callback, "Erasing Bad Symbols From Title");
        res = resFiltered;
    }
}

static const TUtf16String TURKEY_DIACRITIC = u"ÇĞİÖŞÜçğöşüı";

void TSnipTitleSupplementer::CapitalizeWordTitleLetters(TSnipTitle& res) {
    const TUtf16String& title = res.GetTitleString();
    if (!title)
        return;

    ELanguage langId = Lang;
    if (title.find_first_of(TURKEY_DIACRITIC) != TUtf16String::npos)
        langId = LANG_TUR;

    TUtf16String titleUp = Capitalize(title, langId);

    TMakeTitleOptions options = ForbidCuttingOptions;
    options.TitleGeneratingAlgo = TGA_PREFIX;
    if (Cfg.TwoLineTitles() && res.GetPixelLength() > options.PixelsInLine) {
        options.PixelsInLine *= 2;
    }
    TSnipTitle resCapitalized = MakeTitle(titleUp, Cfg, Query, options);
    if (resCapitalized.GetLogMatchIdfSum() + 1E-7 > res.GetLogMatchIdfSum() &&
        resCapitalized.GetTitleString() != title) {
        PrintTitleDebugOutput(res, resCapitalized, Callback, "Title Capitalization");
        res = resCapitalized;
    }
}

void TSnipTitleSupplementer::TransformTitle(TSnipTitle& res, TTransformFunc transformFunc) {
    TUtf16String titleString = res.GetTitleString();
    if (transformFunc(titleString)) {
        TMakeTitleOptions newOptions = ForbidCuttingOptions;
        newOptions.TitleGeneratingAlgo = TGA_PREFIX;
        res = MakeTitle(titleString, Cfg, Query, newOptions);
    }
}

}
