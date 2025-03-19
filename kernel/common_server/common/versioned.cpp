#include "versioned.h"

#include <util/string/builder.h>
#include <util/string/cast.h>

bool TVersionInfo::Parse(TStringBuf v, TVersionInfo& result) {
    if (!v.StartsWith("v")) {
        return false;
    }
    v = v.Skip(1);
    TStringBuf x;
    TStringBuf yz;
    TStringBuf y;
    TStringBuf z;
    if (v.TrySplit('.', x, yz)) {
        if (TryFromString(x, result.Major)) {
            if (yz.TrySplit('.', y, z)) {
                return TryFromString(y, result.Minor) && TryFromString(z, result.Patch);
            } else {
                return TryFromString(yz, result.Minor);
            }
        }
    } else {
        return TryFromString(v, result.Major);
    }
    return false;
}

bool TVersionInfo::Match(const TVersionInfo& other) const {
    if (other.Major == Undef) {
        return true;
    }
    if (Major == other.Major) {
        if (other.Minor == Undef) {
            return true;
        }
        if (Minor == other.Minor) {
            if (other.Patch == Undef || Patch == other.Patch) {
                return true;
            }
        }
    }
    return false;
}

TString TVersionInfo::ToString() const {
    if (Major == Undef) {
        return TString();
    }
    TStringStream ss;
    ss << "v" << Major;
    if (Minor != Undef) {
        ss << "." << Minor;
        if (Patch != Undef) {
            ss << "." << Patch;
        }
    }
    return ss.Str();
}

TVersionedKey::TVersionedKey(const TString& key)
    : Name(key)
{
    TStringBuf buf(key);
    TStringBuf r;
    TStringBuf l;
    buf.TrySplit('/', l, r);
    if (!!r && !!l && TVersionInfo::Parse(l, Version)) {
        Name = r;
    }
    Normalize();
}

TVersionedKey::TVersionedKey(const TString& name, const TVersionInfo& version)
    : Name(name)
    , Version(version)
{
    Normalize();
}

bool TVersionedKey::Match(const TVersionedKey& other) const {
    // current versioned key fits for the other
    return Name == other.Name && Version.Match(other.Version);
}

TString TVersionedKey::ToString() const {
    if (!Version.IsDefined()) {
        return Name;
    }

    return TStringBuilder() << Version.ToString() << "/" << Name;
}

void TVersionedKey::Normalize() {
    while (Name.EndsWith('/')) {
        Name.pop_back();
    }
}
