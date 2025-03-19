#include "ext_owner.h"

#include <library/cpp/regex/pire/pire.h>

#include <util/generic/hash_set.h>
#include <util/stream/file.h>
#include <util/system/compat.h>
#include <util/string/vector.h>
#include <util/string/cast.h>

class TOwnerExtractor::TAreas {
public:
    Y_SAVELOAD_DEFINE(Regexp, Dead)

    NPire::TScanner Regexp;
    NPire::TScanner::State Dead;

    TAreas(const TVector<TString>& areas);
    void AddArea(NPire::TFsm& fsm, TStringBuf area);
};

TOwnerExtractor::TAreas::TAreas(const TVector<TString>& areas)
{
    NPire::TFsm fsm;

    // A hardcoded catch-all, which treats all uncaught second-level domains as owners
    AddArea(fsm, "raw:[a-z0-9_\\-]+\\.([a-z]{2,}|xn--[a-z0-9]{2,})");

    THashSet<TString> uniqueAreas;

    for (const TString& area : areas) {
        Y_ASSERT(!uniqueAreas.contains(area) && "Areas are not unique");
        if (!uniqueAreas.contains(area)) {
            AddArea(fsm, area);
            uniqueAreas.insert(area);
        }
    }

    Regexp = fsm.Compile<NPire::TScanner>();
    Regexp.Initialize(Dead);
    Regexp.Next(Dead, '\n'); // assume no pattern contains a newline, so we now have a Dead state
    Y_ASSERT(!Regexp.Final(Dead) && "Dead state final");
}

void TOwnerExtractor::TAreas::AddArea(NPire::TFsm& fsm, TStringBuf area) {
    TString full;

    TString suffix;
    // Search a regexp for owner name in path
    size_t space = area.find_first_of(" \t");
    if (space != TString::npos) {
        size_t notSpace = area.find_first_not_of(" \t", space + 1);
        if (notSpace == TString::npos) {
            notSpace = space + 1;
        }
        suffix = ToString(area.SubStr(notSpace));
        area.Trunc(space);
    }

    if (area.StartsWith("raw:")) {
        full = ToString(area.Skip(4));
    } else {
        if (area.StartsWith("http://"))
            area.Skip(7);
        // Quote all dots
        TString quoted(ToString(area));
        for (size_t i = 0; i < quoted.size(); ++i) {
            if (quoted[i] == '.') {
                quoted.replace(i, 1, "\\.");
                i += 2;
            }
        }
        // Prepend a regexp for area subdomains
        full = quoted.prepend("([a-z_0-9\\-]+\\.)?");
    }

    size_t slash = full.find('/');
    // Prefix (host part) fsm will be reversed and made case-insensitive
    TString prefix(LegacySubstr(full, 0, slash));
    // Suffix (path part)
    suffix.prepend(LegacySubstr(full, slash));

    NPire::TLexer prefLex(prefix.begin(), prefix.vend());
    prefLex.AddFeature(NPire::NFeatures::CaseInsensitive());
    NPire::TFsm curFsm = prefLex.Parse().Reverse();

    if (suffix.size()) {
        NPire::TLexer suffLex(suffix.begin(), suffix.vend());
        NPire::TFsm suffFsm = suffLex.Parse();
        curFsm += suffFsm;
    }

    if (fsm.Size() == 1) {
        fsm = curFsm;
    } else {
        fsm |= curFsm;
    }
}

static void ReadAreas(IInputStream& input, TVector<TString>& areas) {
    TString line;
    while (input.ReadLine(line)) {
        areas.push_back(line);
    }
}

TOwnerExtractor::TOwnerExtractor(const TString& areasFilenames /* = "/place/mfas/data/j-owners-spm.lst" */)
{
    TVector<TString> areas;
    TVector<TString> filenames = SplitString(areasFilenames, ":");
    for (TVector<TString>::const_iterator i = filenames.begin(), ie = filenames.end(); i != ie; ++i) {
        TFileInput in(*i);
        ReadAreas(in, areas);
    }
    Areas = new TAreas(areas);
}

TOwnerExtractor::TOwnerExtractor(IInputStream& input) {
    TVector<TString> areas;
    ReadAreas(input, areas);
    Areas = new TAreas(areas);
}

TOwnerExtractor::TOwnerExtractor(const TVector<TString>& areas)
    : Areas(new TAreas(areas))
{
}

TOwnerExtractor::~TOwnerExtractor()
{
}

static TStringBuf RemoveProtocol(TStringBuf url) {
    size_t slash = url.find('/');
    // remove anything that looks like protocol name
    if (slash != TStringBuf::npos && slash > 0 && slash < url.size() - 1 && url[slash - 1] == ':' && url[slash + 1] == '/') {
        url.Skip(slash + 2);
    }
    return url;
}

static std::pair<TStringBuf, TStringBuf> SplitUrl(TStringBuf url) {
    url = RemoveProtocol(url);
    size_t slash = url.find('/');
    if (slash != TStringBuf::npos) {
        return std::make_pair(TStringBuf(url.begin(), slash), TStringBuf(url.begin() + slash, url.end()));
    }
    return std::make_pair(url, TStringBuf());
}

TStringBuf TOwnerExtractor::GetOwner(TStringBuf url) const {
    std::pair<TStringBuf, TStringBuf> parts(SplitUrl(url));
    std::pair<TStringBuf, TStringBuf> owner(GetOwnerFromParts(parts.first, parts.second));

    // FIXME:
    // In certain cases we might return the port number as a part of an owner
    // (e.g. yet.another.blogs.com:9000/users/vasya).
    return TStringBuf(owner.first.begin(), owner.second.empty() ? owner.first.end() : owner.second.end());
}

TString TOwnerExtractor::GetOwnerNormalized(TStringBuf url) const {
    std::pair<TStringBuf, TStringBuf> parts(SplitUrl(url));
    std::pair<TStringBuf, TStringBuf> owner(GetOwnerFromParts(parts.first, parts.second));

    TString host(ToString(owner.first));
    host.to_lower();
    return host + ToString(owner.second);
}

template <typename TAreas>
static TStringBuf MatchHost(NPire::TScanner::State& state, TStringBuf host, const TAreas& areas) {
    if (host.empty()) {
        return TStringBuf();
    }

    TStringBuf hostWithPort = RemoveProtocol(host);
    if (hostWithPort.size() && hostWithPort[0] == '[') {
        host = hostWithPort.Trunc(hostWithPort.find(']') + 1);
    } else {
        host = hostWithPort.Before(':');
    }
    TStringBuf ret(host);
    for (const char* p = host.end() - 1; p >= host.begin() && state != areas.Dead; --p) {
        areas.Regexp.Next(state, (ui8)*p);
        if (areas.Regexp.Final(state)) {
            ret = TStringBuf(p, host.end());
        }
    }
    return ret;
}

std::pair<TStringBuf, TStringBuf> TOwnerExtractor::GetOwnerFromParts(TStringBuf host, TStringBuf path) const {
    NPire::TScanner::State state;
    Areas->Regexp.Initialize(state);

    TStringBuf foundInHost(MatchHost(state, host, *Areas));
    TStringBuf foundInPath;
    if (state != Areas->Dead) {
        for (const char* p = path.begin(); p < path.end(); ++p) {
            Areas->Regexp.Next(state, (unsigned char)*p);
            if (Areas->Regexp.Final(state)) {
                foundInPath = TStringBuf(path.begin(), p + 1);
            }
        }
    }

    // FIXME:
    // Host part of the owner must be downcased. However, it seems to be impossible
    // unless we make a copy of the given argument.
    return std::make_pair(foundInHost, foundInPath);
}

bool TOwnerExtractor::IsExtendable(TStringBuf host) const {
    NPire::TScanner::State state;
    Areas->Regexp.Initialize(state);
    MatchHost(state, host, *Areas);
    Areas->Regexp.Next(state, '/');
    return state != Areas->Dead;
}

void TOwnerExtractor::Save(IOutputStream* s) const {
    ::Save(s, *Areas);
}

void TOwnerExtractor::Load(IInputStream* s) {
    ::Load(s, *Areas);
}
