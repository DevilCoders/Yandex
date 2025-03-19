#pragma once

#include <util/generic/string.h>
#include <library/cpp/containers/str_hash/str_hash.h>
#include <util/folder/dirut.h>
#include <util/memory/blob.h>

// domain in h, subdomain of any domain in h, or 2nd level domain
template<class TDom2Set>
const char* GetHostOwner(const TDom2Set &h, const char *host, bool& inDom2) {
    return GetHostOwner(h, TStringBuf(host), inDom2).data();
}

template<class TDom2Set>
const char* GetHostOwner(const TDom2Set &h, const char *host) {
    bool inDom2;
    return GetHostOwner(h, host, inDom2);
}

// **********************
// domain in h, subdomain of any domain in h, or 2nd level domain
template<class TDom2Set>
TStringBuf GetHostOwner(const TDom2Set &h, TStringBuf host, bool& inDom2) {
    if (!host.length()) {
        inDom2 = false;
        return host;
//        return TStringBuf(0, (size_t)0);
    }
    // first we eat scheme and port
    if (const char *colon = (char*)memchr(host.data(), ':', host.length())) {
        // might be scheme prefix
        if (colon + 3 < host.data() + host.length() && colon[1] == '/' && colon[2] == '/') {
            colon += 3;
            host = TStringBuf(colon, host.length() - (colon - host.data()));
            colon = (char*)memchr(host.data(), ':', host.length());
        }
        // now it's port
        if (colon)
            host = TStringBuf(host.data(), colon - host.data());
    }
    // then cut lower domain parts one by one, searching in h
    if (h.find(host) != h.end()) {
        inDom2 = true;
        return host;
    }
    TStringBuf host_sub = host;
    const char *dot_r = (char*)memrchr(host.data(), '.', host.length());
    if (dot_r) {
        const char *next_dot;
        while ((next_dot = (char*)memchr(host.data(), '.', host.length())) != dot_r) {
            next_dot++;
            if (*next_dot == '.')
                break;
            host = TStringBuf(next_dot, host.length() - (next_dot - host.data()));
            if (h.find(host) != h.end()) {
                inDom2 = true;
                return host_sub;
            }
            host_sub = host;
        }
    }
    inDom2 = false;
    return host_sub;
}

// Returns true if next possible suffix found, otherwise returns false
template <class TDom2Set>
bool GetNextHostSuffixOrOwner(const TDom2Set& h, TStringBuf& host) {
    host.NextTok('.');
    return !host.empty() && !h.contains(host) && host.Contains('.');
}

template<class TDom2Set>
TStringBuf GetHostOwner(const TDom2Set &h, TStringBuf host) {
    bool inDom2;
    return GetHostOwner(h, host, inDom2);
}
// **********************

inline TStringBuf GetOwnerPart(const TStringBuf& owner) {
    const auto dotPos = owner.find('.');
    return owner.substr(0, dotPos != TStringBuf::npos ? dotPos + 1 : dotPos);
}

class TOwnerCanonizer
{
private:
    HashSet Dom2Set;
public:
    TOwnerCanonizer() { }
    TOwnerCanonizer(const TString& ownersFilename) {
        LoadDom2(ownersFilename);
    }

    TStringBuf GetUrlOwner(const TStringBuf& url) const;

    TStringBuf GetHostOwner(const TStringBuf& host) const {
        return ::GetHostOwner(Dom2Set, host);
    }

    bool GetNextHostSuffixOrOwner(TStringBuf& host) const {
        return ::GetNextHostSuffixOrOwner(Dom2Set, host);
    }

    bool InDom2(const TStringBuf& host) const {
        bool inDom2;
        ::GetHostOwner(Dom2Set, host, inDom2);
        return inDom2;
    }

    void AddDom2(const char* host);

    void LoadDom2(TBlob ownersfile);
    void UnloadDom2(TBlob ownersfile);

    void LoadDom2(const TString& ownersFilename);
    void UnloadDom2(const TString& ownersFilename);

    // areas.lst
    void LoadTrueOwners(const TString& urlrulesdir);
    void LoadTrueOwners(); // compiled

    static const TOwnerCanonizer& WithTrueOwners();

    // 2ld.list and ungrouped.list
    void LoadSerpCategOwners(const TString& urlrulesdir);
    void LoadSerpCategOwners(); // compiled

    static const TOwnerCanonizer& WithSerpCategOwners();
};

class TDomAttrCanonizer : public TOwnerCanonizer
{
public:
    TDomAttrCanonizer(const TString& urlrulesDir) {
        TExistenceChecker ec(true);
        LoadDom2(ec.Check((urlrulesDir + "/2ld.list").data()));
        LoadDom2(ec.Check((urlrulesDir + "/ungrouped.list").data()));
    }

    TDomAttrCanonizer(const TBlob& _2ldList, const TBlob& ungroupedList) {
        LoadDom2(_2ldList);
        LoadDom2(ungroupedList);
    }

    TStringBuf GetUrlDAttr(const TStringBuf& url) const {
        return GetUrlOwner(url);
    }
};

namespace NOwner {
    const char AREAS_LST_FILENAME[] = "areas.lst"; // true owners
    const char _2LD_LIST_FILENAME[] = "2ld.list";  // "serp categs" - the owners used for grouping
    const char UNGROUPED_LIST_FILENAME[] = "ungrouped.list"; // more "serp categs" - used for ungrouping
}

// loads compiled-in file from yweb/urlrules
void CompiledDom2(TOwnerCanonizer& c, const TString& filename, bool load = true);
