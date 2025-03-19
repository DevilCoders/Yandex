#pragma once

#include <tuple>
#include <util/generic/string.h>

class TVersionInfo {
public:
    static bool Parse(TStringBuf v, TVersionInfo& result);

public:
    const static ui32 Undef = Max<ui32>();

private:
    ui32 Major;
    ui32 Minor;
    ui32 Patch;

public:
    TVersionInfo()
        : Major(Undef)
        , Minor(Undef)
        , Patch(Undef)
    {
    }

    TVersionInfo(ui32 major, ui32 minor, ui32 patch)
        : Major(major)
        , Minor(minor)
        , Patch(patch)
    {
    }

    bool IsDefined() const {
        return Major != Undef;
    }

    ui32 GetMajor() const {
        return Major;
    }
    ui32 GetMinor() const {
        return Minor;
    }
    ui32 GetPatch() const {
        return Patch;
    }

    bool operator < (const TVersionInfo& other) const {
        return std::make_tuple(Major, Minor, Patch) < std::make_tuple(other.Major, other.Minor, other.Patch);
    }

    bool operator == (const TVersionInfo& other) const {
        return std::make_tuple(Major, Minor, Patch) == std::make_tuple(other.Major, other.Minor, other.Patch);
    }

    bool Match(const TVersionInfo& other) const;
    TString ToString() const;
};

class TVersionedKey {
private:
    TString Name;
    TVersionInfo Version;

public:
    TVersionedKey() = default;
    TVersionedKey(const TString& key);
    TVersionedKey(const TString& name, const TVersionInfo& version);

    bool operator < (const TVersionedKey& other) const {
        return std::tie(Name, Version) < std::tie(other.Name, other.Version);
    }

    bool operator == (const TVersionedKey& other) const {
        return std::tie(Name, Version) == std::tie(other.Name, other.Version);
    }

    const TString& GetName() const {
        return Name;
    }

    const TVersionInfo& GetVersion() const {
        return Version;
    }

    bool Match(const TVersionedKey& other) const;
    TString ToString() const;

private:
    void Normalize();
};
