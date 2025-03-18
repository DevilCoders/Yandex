#include "infected_masks.h"

#include <util/stream/file.h>
#include <util/string/vector.h>
#include <util/string/strip.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>

#include <utility>
#include <util/string/split.h>
#include <util/string/cast.h>

namespace NInfectedMasks {
    TLiteMask::TLiteMask(TStringBuf host, TStringBuf pNq)
        : TLiteMask(host, pNq, TStringBuf(""))
    {
    }

    TLiteMask::TLiteMask(TStringBuf host, TStringBuf pNq, TStringBuf suf)
        : Host(host)
        , PathAndQuery(pNq)
        , Suffix(suf)
    {
    }

    void TLiteMask::Render(TString& res) const {
        res.assign(Host).append(PathAndQuery);
        if (Suffix) {
            res.append(Suffix);
        }
    }

    TString TLiteMask::ToString() const {
        TString res;
        Render(res);
        return res;
    }

    bool TLiteMask::operator==(const TLiteMask& o) const {
        return std::make_tuple(Host, PathAndQuery, Suffix) == std::make_tuple(o.Host, o.PathAndQuery, o.Suffix);
    }
}

template <>
void Out<NInfectedMasks::TLiteMask>(IOutputStream& o, TTypeTraits<NInfectedMasks::TLiteMask>::TFuncParam mask) {
    o << ToString(mask);
}

static void RemoveSpecialChars(TString& s) {
    TString tmp;
    tmp.reserve(s.size());
    for (size_t i = 0, sz = s.size(); i < sz; ++i) {
        auto c = s[i];
        if (strchr("\t\n\r", c)) {
            continue;
        } else {
            tmp.append(c);
        }
    }
    s.swap(tmp);
}

//
// Fully unescape the given string, then re-escape once.
//
static void SafeEscapeUrl(TString& url) {
    TString unquoted = url;
    UrlUnescape(unquoted);
    while (unquoted != url) {
        url = unquoted;
        UrlUnescape(unquoted);
    }
    UrlEscape(url);
}

static TString CanonicalizeHostname(const TString& hostname) {
    TString host = hostname;
    RemoveSpecialChars(host);
    SafeEscapeUrl(host);

    // Replace consecutive dots with a single dot.
    host.erase(Unique(host.begin(), host.vend(), [](char a, char b) {
                   return a == '.' && b == '.';
               }),
               host.vend());

    // Remove all leading and trailing dots
    if (!host.empty() && host[0] == '.') {
        host.erase(0, 1);
    }
    if (!host.empty() && host.back() == '.') {
        host.pop_back();
    }

    // If the hostname can be parsed as an IP address, it should be normalized to 4 dot-separated decimal values.
    // The client should handle any legal IP address encoding, including octal, hex, and fewer than 4 components.

    // Lowercase the whole string.
    host.to_lower();

    return host;
}

static TString CanonicalizeUrlpath(const TString& urlpath) {
    TString url = Strip(urlpath);

    // Start by stripping off the fragment identifier
    size_t pos = url.find('#');
    if (pos != TString::npos) {
        url = url.substr(0, pos);
    }

    // Remove any embedded tabs and CR/LF characters which aren't escaped
    RemoveSpecialChars(url);

    SafeEscapeUrl(url);

    if (!url) {
        return TString("/");
    }

    TVector<TString> pathComponents;
    StringSplitter(url).Split('/').SkipEmpty().Collect(&pathComponents);
    for (size_t i = 0; i < pathComponents.size();) {
        if (!pathComponents[i] || pathComponents[i] == ".") {
            pathComponents.erase(pathComponents.begin() + i, pathComponents.begin() + i + 1);
            continue;
        }
        // If the path component is '..' we skip it and remove the preceding path
        // component if there are any
        if (pathComponents[i] == "..") {
            pathComponents.erase(pathComponents.begin() + i, pathComponents.begin() + i + 1);
            if (i > 0) {
                --i;
                pathComponents.erase(pathComponents.begin() + i, pathComponents.begin() + i + 1);
            }
            continue;
        }
        ++i;
    }

    if (pathComponents.size() == 0) {
        return TString("/");
    }

    pathComponents.insert(pathComponents.begin(), TString()); // leading slash
    if (urlpath.back() == '/') {
        pathComponents.push_back(TString()); // restore tailing slash
    }

    return JoinStrings(pathComponents, "/");
}

TInfectedMasksGenerator::TInfectedMasksGenerator(const TString& url, NInfectedMasks::ECompatibility compatibility) {
    TStringBuf urlNoScheme = CutSchemePrefix(url);
    TStringBuf host = GetHost(urlNoScheme);
    TStringBuf pathQuery = GetPathAndQuery(urlNoScheme, true);
    Init(
        CanonicalizeHostname(ToString(host)),
        CanonicalizeUrlpath(ToString(pathQuery)),
        compatibility
    );
}

TInfectedMasksGenerator::TInfectedMasksGenerator(
        const TString& canonHost,
        const TString& canonPath,
        NInfectedMasks::ECompatibility compatibility
) {
    Init(canonHost, canonPath, compatibility);
}

void TInfectedMasksGenerator::Init(TStringBuf canonHost, TStringBuf canonPath, NInfectedMasks::ECompatibility compatibility) {
    using namespace NInfectedMasks;
    Url.assign(canonHost).append(canonPath);
    GenerateMasksFast(Masks, Url, compatibility);
    Next();
}

void TInfectedMasksGenerator::SetMask() {
    if (IsValid()) {
        const auto& mask = Masks[MaskIdx];
        mask.Render(MaskBuffer);
    } else {
        MaskBuffer.clear();
    }
}

namespace NInfectedMasks {
    void GenerateMasksFast(TVector<TLiteMask>& masks, const TStringBuf normUrl, const ECompatibility comp) {
        masks.clear();

        const TStringBuf host = GetHost(normUrl);
        TStringBuf pathAndQuery = GetPathAndQuery(normUrl, true);
        if (!pathAndQuery) {
            pathAndQuery = "/";
        }
        TStringBuf path = pathAndQuery.Before('?');
        if (!path) {
            path = "/";
        }
        const size_t hostSz = host.size();
        const size_t pathSz = path.size();
        const size_t pathAndQuerySz = pathAndQuery.size();

        if (!hostSz) {
            return;
        }

        size_t hostPos = 0;
        size_t hostLastPos = hostSz;

        for (ui32 hostI = 0; hostI < 5; ++hostI) {
            TStringBuf hostSuf;

            if (0 == hostI) {
                hostSuf = host;
            } else {
                if (1 == hostI) {
                    hostLastPos = host.rfind('.');

                    if (TStringBuf::npos == hostLastPos) {
                        break;
                    }

                    ui32 dotTot = 0;

                    size_t dotPos = hostLastPos;
                    for (dotTot = 0; dotTot < 4; ++dotTot) {
                        size_t next = host.rfind('.', dotPos);
                        if (TStringBuf::npos == next) {
                            break;
                        } else {
                            dotPos = next;
                        }
                    }

                    hostPos = dotPos;
                } else {
                    hostPos = host.find('.', hostPos + 1);
                }

                if (hostPos < hostLastPos) {
                    hostSuf = host.SubStr(hostPos + 1);
                } else {
                    break;
                }
            }

            ui32 pathI = 0;

            masks.emplace_back(hostSuf, pathAndQuery);
            pathI += 1;

            if (pathSz < pathAndQuerySz) {
                masks.emplace_back(hostSuf, path);
                pathI += 1;
            }

            // условие pathSz == pathAndQuerySz добавлено для совместимости со старым кодом.
            if (C_GOOGLE != comp && pathSz == pathAndQuerySz && path.back() != '/') {
                masks.emplace_back(hostSuf, path, TStringBuf("/"));
            }

            if (pathSz > 1) {
                size_t pathPos = 0;
                for (; pathI < 6u; ++pathI) {
                    TStringBuf pathPref;
                    if (pathPos < pathSz - 1) {
                        pathPref = path.SubStr(0, pathPos + 1);
                        pathPos = path.find('/', pathPos + 1);
                    } else {
                        break;
                    }
                    masks.emplace_back(hostSuf, pathPref);
                }
            }
        }

        if (C_YANDEX == comp) {
            SortBy(masks, [](const TLiteMask& mask) {
                return std::make_tuple(mask.PathAndQuery.size(), mask.Suffix.size(), mask.Host.size());
            });
        }
    }
}

void TInfectedMasks::AddMask(TString mask, TString data) {
    TString host;
    size_t pos = mask.find('/');
    if (pos == TString::npos) {
        host = mask;
        mask += '/';
    } else {
        host = mask.substr(0, pos);
    }
    Domains.insert(ToString(GetParentDomain(host, 2)));
    Masks.insert(std::pair<TString, TString>(mask, data));
}

void TInfectedMasks::Init(const TString& filePath) {
    auto data = TBlob::FromFileContent(filePath);
    TStringBuf dataBuf(data.AsCharPtr(), data.Size());
    TStringBuf line;
    while (dataBuf.ReadLine(line)) {
        Y_ASSERT(line);
        auto v0 = line.NextTok('\t');
        auto v1 = line;
        AddMask(TString(v0), TString(v1));
    }
}

TMasksRange TInfectedMasks::GetRange(const TString& hostname, const TString& urlpath) const {
    TString host = CanonicalizeHostname(ToString(GetHost(CutSchemePrefix(hostname))));
    TString domain = ToString(GetParentDomain(host, 2));

    // Optimization: Don't need to perform all the lookups if nothing in the domain is infected
    if (!Domains.contains(domain)) {
        return TMasksRange(Masks.end(), Masks.end());
    }

    TString url = CanonicalizeUrlpath(urlpath);

    for (TInfectedMasksGenerator generator(host, url); generator.HasNext(); generator.Next()) {
        TMasksRange range = Masks.equal_range(generator.Current());
        if (range.first != Masks.end()) {
            return range;
        }
    }

    return TMasksRange(Masks.end(), Masks.end());
}

TMasksRange TInfectedMasks::GetRange(const TString& url) const {
    TStringBuf urlNoScheme = CutSchemePrefix(url);
    TStringBuf host = GetHost(urlNoScheme);
    TStringBuf pathQuery = GetPathAndQuery(urlNoScheme, true);
    return GetRange(ToString(host), ToString(pathQuery));
}

bool TInfectedMasks::IsInfectedUrl(const TString& hostname, const TString& urlpath) const {
    return GetRange(hostname, urlpath).first != Masks.end();
}

bool TInfectedMasks::IsInfectedUrl(const TString& url) const {
    return GetRange(url).first != Masks.end();
}

TVector<TString>* TInfectedMasks::GetData(const TString& url) const {
    THolder<TVector<TString>> vs(new TVector<TString>);

    TMasksRange range = GetRange(url);

    for (TMasksIterator it = range.first; it != range.second; it++) {
        vs->push_back(it->second);
    }

    return vs.Release();
}
