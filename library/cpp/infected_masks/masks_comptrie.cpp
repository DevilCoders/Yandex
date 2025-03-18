#include "masks_comptrie.h"

#include <library/cpp/uri/uri.h>
#include <library/cpp/uri/parse.h>
#include <library/cpp/uri/other.h>

#include <util/string/split.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/join.h>

TCategMaskComptrie::TCategMaskComptrie() {
    OwnerCanonizer.LoadTrueOwners();
}

void TCategMaskComptrie::Init(const TBlob& blob) {
    Trie.Init(blob);
}

inline size_t KeepingSplit(const TStringBuf& s, const TSetDelimiter<const char>& delim, TVector<TStringBuf>& res) {
    using TCS = TContainerConsumer<TVector<TStringBuf>>;
    using TSCS = TSkipEmptyTokens<TCS>;
    using TKSCS = TKeepDelimiters<TSCS>;
    res.clear();
    TCS res1(&res);
    TSCS res2(&res1);
    TKSCS consumer(&res2);
    SplitString(s.data(), s.data() + s.size(), delim, consumer);
    return res.size();
};

void TCategMaskComptrie::FindPath(TString& host, TStringBuf& urlPath, TResult& values, TTrie currentSubtrie) const {
    if (currentSubtrie.IsEmpty())
        return;
    TVector<TStringBuf> pathParts;
    KeepingSplit(urlPath, "/?&", pathParts);
    TString currentPath("/");

    for (auto part = pathParts.begin() + 1;; ++part) {
        TStringBuf value;
        if (currentSubtrie.Find("*", &value) && !(part != pathParts.end() && *part == "/"))
            values.push_back(std::make_pair(host + currentPath, value));

        if (part == pathParts.end())
            break;

        currentPath += *part;
        currentSubtrie = currentSubtrie.FindTails(*part);
    }
}

void TCategMaskComptrie::Find(TTrie categSubtrie, TStringBuf hostname, TStringBuf urlPath, TResult& values) const {
    TVector<TStringBuf> hostParts;
    KeepingSplit(hostname, ".", hostParts);
    TTrie currentSubtrie(categSubtrie);

    TString currentHost;

    for (auto part = hostParts.rbegin();; ++part) {
        TTrie generic = currentSubtrie.FindTails("/");
        FindPath(currentHost, urlPath, values, generic);

        if (part == hostParts.rend())
            break;

        currentHost = *part + currentHost;
        currentSubtrie = currentSubtrie.FindTails(*part);
    }
}

bool TCategMaskComptrie::Find(const TStringBuf& key, TResult& values) const {
    values.clear();
    TStringBuf prefix, url;
    key.RSplit(' ', prefix, url);
    TTrie categSubtrie = Trie.FindTails(prefix);

    if (categSubtrie.IsEmpty())
        return false;

    categSubtrie = categSubtrie.FindTails("\t");
    TString host(GetHost(url));
    host.to_lower();
    Find(categSubtrie, host, GetPathAndQuery(url), values);

    return values.size();
}

bool TCategMaskComptrie::FindExact(const TStringBuf& key, TStringBuf& value) const {
    return Trie.Find(key, &value);
}

bool TCategMaskComptrie::FindByUrl(const TStringBuf& url, TCategMaskComptrie::TResult& values) const {
    return Find(OwnerCanonizer.GetUrlOwner(url) + TString(" ") + url, values);
}

TString TCategMaskComptrie::GetInfectedMaskFromInternalKey(TStringBuf key) {
    key = key.RAfter('\t');
    TStringBuf host, path;
    size_t split = key.find_first_of("/*?");
    key.SplitAt(split, host, path);
    TVector<TStringBuf> hostParts;
    KeepingSplit(host, ".", hostParts);
    return JoinRange("", hostParts.rbegin(), hostParts.rend()) + path.Before('*');
}
