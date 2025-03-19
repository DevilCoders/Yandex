#include "info.h"
#include "normalize_url.h"
#include "url_canonizer.h"

#include <library/cpp/string_utils/url/url.h>
#include <util/generic/algorithm.h>

#include <kernel/urlid/urlhash.h>

namespace NMango
{

#include "urls_data.inc"

enum TLivejournalHostType
{
    NOT_LIVEJOURNAL,
    HOME,
    PICS,
    JOURNAL,
};

TLivejournalHostType GetLivejournalHostType(TStringBuf host)
{
    if (host.empty() || !host.EndsWith("livejournal.com"))
        return NOT_LIVEJOURNAL;
    if (host == "livejournal.com")
        return HOME;
    if (host == "pics.livejournal.com")
        return PICS;
    return JOURNAL;
}

THttpURL::TParsedState CheckHost(const TStringBuf &host)
{
    if (host.empty())
        return THttpURL::ParsedBadFormat;
    for (size_t i = 0; i < host.size(); ++i) {
        if (!isalnum((const unsigned char)host[i]) && host[i] != '.' && host[i] != '-' && host[i] != '_')
            return THttpURL::ParsedBadFormat;
        if (i < host.size() - 1 && host[i] == '.' && host[i + 1] == '.')
            return THttpURL::ParsedBadFormat;
    }
    return THttpURL::ParsedOK;
}

// TUrlInfo
TUrlInfo::TUrlInfo(const TString &url, const TString &baseUrl)
    : Valid(false)
{
    if (url.size() > 1024) {
        // Long urls may cause problems (for example, they might not fit in MapReduce record key).
        return;
    }
    THttpURL::TParsedState result = Url.Parse(url.data(), THttpURL::FeaturesRobot | THttpURL::FeatureToLower | THttpURL::FeatureEscapeUnescaped, baseUrl.data());
    if (result == THttpURL::ParsedOK) {
        bool allowedScheme = (Url.GetScheme() == THttpURL::SchemeHTTP) || (Url.GetScheme() == THttpURL::SchemeHTTPS);
        Valid = allowedScheme && (THttpURL::ParsedOK == CheckHost(Url.GetField(THttpURL::FieldHost)));
    }
}

TUrlInfo::TUrlInfo(const THttpURL &url)
    : Url(url), Valid(true)
{}

TString TUrlInfo::GetRaw() const
{
    Y_ASSERT(IsValid());
    return Url.PrintS();
}

TStringBuf TUrlInfo::GetHost() const
{
    Y_ASSERT(IsValid());
    return CutWWWPrefix(Url.GetField(THttpURL::FieldHost));
}

TStringBuf TUrlInfo::GetPath() const
{
    Y_ASSERT(IsValid());
    return Url.GetField(THttpURL::FieldPath);
}

bool TUrlInfo::IsDeprecated() const
{
    return !IsValid()
         || IsTrash()
         || IsIn(DEPRECATED_DOC_PATTERNS)
         || IsImage();
}

bool TUrlInfo::IsTrash() const
{
    return !IsValid()
         || IsIn(TRASH_DOC_PATTERNS)
         || (IsMorda() && IsTiny());
}

bool TUrlInfo::IsMorda() const
{
    if (IsValid()) {
        TStringBuf path = GetPath();
        return (path.empty() || path == "/") && Url.GetField(THttpURL::FieldQuery).empty();
    }
    return false;
}

bool TUrlInfo::IsCrawledByPPB() const
{
    return IsValid() && GetLivejournalHostType(GetHost()) == JOURNAL;
}

bool TUrlInfo::IsUrlMangler() const {
    return IsValid() && IsIn(URL_MANGLER_PATTERNS);
}

bool TUrlInfo::IsTiny() const
{
    return IsValid() && (IsIn(SHORTENERS) || IsUrlMangler());
}

bool TUrlInfo::IsImage() const {
    TStringBuf path = GetPath();
    for (TVector<TString>::const_iterator it = IMAGE_EXTENSIONS.begin(); it != IMAGE_EXTENSIONS.end(); ++it) {
        if (path.EndsWith("." + *it))
            return true;
    }
    return false;
}

bool TUrlInfo::IsYoutubeVideo() const {
    return GetHost() == "youtube.com" && GetPath() == "/watch";
}

bool TUrlInfo::NeedToDownload() const
{
    if (!IsValid() || IsTrash() || IsImage() || IsDeprecated())
        return false;
    TLivejournalHostType type = GetLivejournalHostType(GetHost());
    return (type != PICS) && (type != JOURNAL);
}

static bool WildcardMatch(const char *pattern, const char *str) {
    while (*str) {
        if (*pattern == '*') {
            while (*pattern == '*') {
                ++pattern;
            }
            if (!*pattern) {
                return true;
            }
            while (*str) {
                if (WildcardMatch(pattern, str)) {
                    return true;
                }
                ++str;
            }
            return false;
        } else {
            if (*pattern != *str) {
                return false;
            }
            ++pattern;
            ++str;
        }
    }
    while (*pattern == '*') {
        ++pattern;
    }
    return !*pattern;
}

bool TUrlInfo::IsIn(const TVector<TString> &list) const
{
    return Find(list.begin(), list.end(), TString{GetHost()}) != list.end();
}

bool TUrlInfo::IsIn(const TVector< std::pair<TString, TString> > &list) const
{
    const TString &host = TString{GetHost()};
    const TString &path = TString{GetPath()};

    for (TVector< std::pair<TString, TString> >::const_iterator it = list.begin(); it != list.end(); ++it) {
        if (WildcardMatch(it->first.data(), host.data()) && WildcardMatch(it->second.data(), path.data())) {
            return true;
        }
    }

    return false;
}

TString TUrlInfo::GetCanonized() const
{
    const static TURLCanonizer canonizer(true, false);
    return TString{CutHttpPrefix(canonizer.Canonize(NormalizeUrl(GetRaw())))};
}

ui32 TUrlInfo::GetOutcomingHash() const
{
    return GetOutcomingHash32(UrlHash64(GetCanonized()));
}

void FixHttpScheme(TString& url)
{
    THttpURL _url;
    THttpURL::TParsedState result = _url.Parse(url.data(), THttpURL::FeaturesRobot | THttpURL::FeatureToLower | THttpURL::FeatureEscapeUnescaped, "");
    if (result == THttpURL::ParsedOK && _url.GetScheme() == THttpURL::SchemeEmpty) {
        url = "http://" + url;
    }
}

} // namespace NMango

