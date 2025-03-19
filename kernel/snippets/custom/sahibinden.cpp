#include "sahibinden.h"
#include "yaca.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/simple_textproc/capital/capital.h>
#include <kernel/snippets/titles/url_titles.h>

#include <library/cpp/containers/comptrie/set.h>
#include <library/cpp/containers/comptrie/loader/loader.h>

#include <util/string/subst.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace NSnippets {

static const char* const HOST = "sahibinden.com";
static const char* const REPL_ID = "robots_txt_stub";
static const TUtf16String SAHIBINDEN = u"Sahibinden";
static const TUtf16String CATALOG_MSG = u"Kullanıcıların eklediği sıfır ve ikinci el emlak, vasıta, giyim, elektronik vb. ürünlerin satışı. Ürünlerin resimleri, açıklamaları ve fiyatları. Şehre, fiyata ve özelliklere göre ürün arama. Online mağaza açma imkanı.";

static bool IsSahibindenUrl(const TString& url) {
    return CutWWWPrefix(GetOnlyHost(url)) == HOST;
}

static TUtf16String GetYaca(const TDocInfos& docInfos, const ELanguage lang, const TConfig& cfg) {
    return TYacaData(docInfos, lang, cfg).Desc;
}

static TUtf16String BuildTitleFromUrl(const TString& url, const TConfig& cfg) {
    TUtf16String wurl = ConvertUrlToTitle(url);
    if (wurl.back() == '/') {
        wurl.pop_back();
    }
    size_t pos = wurl.find('/');
    if (pos == TUtf16String::npos) {
        return SAHIBINDEN;
    }
    wurl = wurl.substr(pos + 1);
    SubstGlobal(wurl, u"/", GetTitleSeparator(cfg));
    SubstGlobal(wurl, wchar16('-'), wchar16(' '));
    for (size_t i = 0; i < wurl.size(); ++i)
        if (i == 0 || wurl[i - 1] == ' ')
            ToUpper(wurl.begin() + i, 1);
    size_t prefixLength = wurl.size();
    size_t questionMarkPos = wurl.rfind('?');
    if (questionMarkPos != TUtf16String::npos)
        prefixLength = questionMarkPos;
    if (prefixLength >= 4 && TWtringBuf(wurl.begin() + prefixLength - 4, 4) == u".php")
        prefixLength -= 4;
    return SAHIBINDEN + GetTitleSeparator(cfg) + wurl.substr(0, prefixLength);
}

void TSahibindenFakeReplacer::DoWork(TReplaceManager* manager) {
    Y_ASSERT(manager);
    const TReplaceContext& ctx = manager->GetContext();
    if (ctx.Cfg.UseTurkey() && IsSahibindenUrl(ctx.Url)) {
        if (GetYaca(ctx.DocInfos, LANG_TUR, ctx.Cfg).size() || GetYaca(ctx.DocInfos, LANG_ENG, ctx.Cfg).size()) {
            return;
        }
        TReplaceResult res;
        res.UseText(CATALOG_MSG, REPL_ID);
        if (ctx.Cfg.BuildSahibindenTitles()) {
            TUtf16String titleString = BuildTitleFromUrl(ctx.Url, ctx.Cfg);
            res.UseTitle(MakeSpecialTitle(titleString, ctx.Cfg, ctx.QueryCtx));
        }
        manager->Commit(res, MRK_SAHIBINDEN_FAKE);
    }
}

static const unsigned char SAHIBINDEN_DATA[] = {
    #include <kernel/snippets/custom/data/sahibinden.inc>
};

using TS2WTrie = TCompactTrie<TString::char_type, TUtf16String>;
static const TS2WTrie SAHIBINDEN_TEMPLATES = LoadTrieFromArchive<TS2WTrie>("/sahibinden.trie", SAHIBINDEN_DATA);

using TStrTrie = TCompactTrieSet<TString::char_type>;
static const TStrTrie SAHIBINDEN_CARS = LoadTrieFromArchive<TStrTrie>("/sahibinden_cars.trie", SAHIBINDEN_DATA);

static const TUtf16String MARK_LOCATION = u"[LOC]";
static const TUtf16String MARK_QUERY = u"[QUERY]";
static const TUtf16String MARK_LABEL = u"[LABEL]";
static const TUtf16String MARK_GEO = u"[GEO]";
static const TUtf16String MARK_CAR = u"[CAR]";
static const TUtf16String MARK_TITLE = u"[TITLE]";
static const TUtf16String E_SUFFIX_CHARS = u"eiöü";
static const TUtf16String A_SUFFIX_CHARS = u"aıоu";
static const TString GEO_SUFFIX_DA = "da";
static const TString GEO_SUFFIX_DE = "de";
static const TString GEO_SUFFIX_DAKI = "daki";
static const TString GEO_SUFFIX_DEKI = "deki";
static const TString BAD_CAR_SUFFIXES[] = {"otomatic", "ikinci-el", "benzin-lpg"};
static const TString TITLE_END = "/detay";

static void AddGeoLocSuffix(TString& word, const TString& a, const TString& e) {
    for (ssize_t i = word.size() - 1; i >= 0; --i) {
        if (A_SUFFIX_CHARS.Contains(word[i])) {
            word.append('\'').append(a);
            return;
        }
        if (E_SUFFIX_CHARS.Contains(word[i])) {
            word.append('\'').append(e);
            return;
        }
    }
}

static bool FillTemplate(TString tail, TUtf16String& snippet, TUtf16String& title, const TConfig& cfg) {
    if (tail.empty())
        return false;

    // snippet: description template[\tmark]
    size_t tab = snippet.rfind('\t');
    if (tab == TUtf16String::npos)
        return !snippet.empty(); // no marks, no replace needed

    TUtf16String mark = snippet.substr(tab + 1);
    snippet.resize(tab);
    if (snippet.empty())
        return false;

    size_t len = 0;
    bool notQuery = true;
    ELanguage langId = LANG_TUR;
    if (mark == MARK_CAR && SAHIBINDEN_CARS.FindLongestPrefix(tail, &len)) {
        if (len < tail.size()) {
            for (const TString& s : BAD_CAR_SUFFIXES)
                if (tail.EndsWith(s))
                    return false;
        }
        // tail может быть таким: модель/регион
        size_t slash = tail.find('/', len);
        if (slash != TString::npos) {
            TString location = tail.substr(slash + 1);
            AddGeoLocSuffix(location, GEO_SUFFIX_DA, GEO_SUFFIX_DE);
            SubstGlobal(location, '-', ' ');
            location.append(' ');
            snippet[0] = 's'; // Sahibinden -> sahibinden
            snippet.prepend(Capitalize(UTF8ToWide(location), LANG_TUR));
            tail.resize(slash);
        }
    } else if (mark == MARK_LOCATION) {
        AddGeoLocSuffix(tail, GEO_SUFFIX_DA, GEO_SUFFIX_DE);
    } else if (mark == MARK_GEO) {
        AddGeoLocSuffix(tail, GEO_SUFFIX_DAKI, GEO_SUFFIX_DEKI);
    } else if (mark == MARK_QUERY) {
        notQuery = false;
        langId = LANG_ENG;
        size_t begin = tail.find_first_of('=');
        if (begin++ == TString::npos)
            return false;
        size_t end = tail.find_first_of('&');
        tail = tail.substr(begin, end == TString::npos ? end : end - begin);
        tail = CGIUnescapeRet(tail);
        if (tail.empty())
            return false;
    } else if (mark == MARK_LABEL) {
        langId = LANG_ENG;
    } else if (mark == MARK_TITLE) {
        size_t pos = tail.find(TITLE_END);
        if (pos == TString::npos)
            return false;
        pos = tail.rfind('-', pos);
        if (pos == TString::npos)
            return false;
        tail.resize(pos);
        SubstGlobal(tail, '-', ' ');
        title = Capitalize(UTF8ToWide(tail), LANG_TUR);
        SubstGlobal(snippet, MARK_TITLE, title);
        title.prepend(SAHIBINDEN + GetTitleSeparator(cfg));
        return true;
    } else
        return false;

    if (notQuery && tail.Contains('?'))
        return false;

    SubstGlobal(tail, '-', ' ');
    SubstGlobal(snippet, mark, Capitalize(UTF8ToWide(tail), langId));
    return true;
}

void TSahibindenTemplatesReplacer::DoWork(TReplaceManager* manager) {
    Y_ASSERT(manager);
    const TReplaceContext& ctx = manager->GetContext();
    if (!ctx.Cfg.UseTurkey())
        return;

    if (GetYaca(ctx.DocInfos, LANG_TUR, ctx.Cfg).size() || GetYaca(ctx.DocInfos, LANG_ENG, ctx.Cfg).size())
        return;

    TString urlWithoutScheme(CutSchemePrefix(ctx.Url));
    size_t len = 0;
    TUtf16String snippet, title;
    if (SAHIBINDEN_TEMPLATES.FindLongestPrefix(urlWithoutScheme, &len, &snippet) &&
        FillTemplate(urlWithoutScheme.substr(len), snippet, title, ctx.Cfg)) {
        SmartCut(snippet, ctx.IH, ctx.LenCfg.GetMaxTextSpecSnipLen(), TSmartCutOptions(ctx.Cfg));
        TReplaceResult res;
        res.UseText(snippet, "sahibinden_template");
        if (title) {
            res.UseTitle(MakeSpecialTitle(title, ctx.Cfg, ctx.QueryCtx));
        } else if (ctx.Cfg.BuildSahibindenTitles()) {
            TUtf16String titleString = BuildTitleFromUrl(ctx.Url, ctx.Cfg);
            res.UseTitle(MakeSpecialTitle(titleString, ctx.Cfg, ctx.QueryCtx));
        }
        manager->Commit(res, MRK_SAHIBINDEN);
    }
}

}
