#pragma once

#include <util/generic/fwd.h>
#include <util/generic/maybe.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>

struct TSemVer {
    ui16 Major;
    ui16 Minor;
    ui16 Patch;
    ui16 Build;

    static TMaybe<TSemVer> FromString(TStringBuf version);

    constexpr TSemVer(ui16 major, ui16 minor = 0, ui16 patch = 0, ui16 build = 0) noexcept
        : Major(major)
        , Minor(minor)
        , Patch(patch)
        , Build(build)
    {
    }

    constexpr bool operator!=(TSemVer other) const noexcept {
        return !(*this == other);
    }
    constexpr bool operator==(TSemVer other) const noexcept {
        return Major == other.Major && Minor == other.Minor && Patch == other.Patch && Build == other.Build;
    }
    constexpr bool operator<(TSemVer other) const noexcept {
        if (Major != other.Major)
            return Major < other.Major;
        if (Minor != other.Minor)
            return Minor < other.Minor;
        if (Patch != other.Patch)
            return Patch < other.Patch;
        return Build < other.Build;
    }
    constexpr bool operator<=(TSemVer other) const noexcept {
        return !(other < *this);
    }
    constexpr bool operator>(TSemVer other) const noexcept {
        return other < *this;
    }
    constexpr bool operator>=(TSemVer other) const noexcept {
        return other <= *this;
    }

    TString ToString() const;
};

struct TFullSemVer {
    class TTag {
    private:
        using TValue = std::variant<TString, ui16>;
        TValue Variant;

    public:
        TTag(TStringBuf value)
            : Variant(TString(value))
        {
        }

        TTag(ui16 value)
            : Variant(value)
        {
        }

        bool operator<(const TTag& other) const noexcept;
        bool operator<=(const TTag& other) const noexcept {
            return !(other < *this);
        }
        bool operator>(const TTag& other) const noexcept {
            return !(*this <= other);
        }
        bool operator>=(const TTag& other) const noexcept {
            return !(*this < other);
        }
        bool operator==(const TTag& other) const noexcept {
            return Variant == other.Variant;
        }

        TString ToString() const;
    };
    using TTags = TVector<TTag>;

    TSemVer Version;
    TTags PreRelease;

    TFullSemVer(TSemVer version, const TTags& preRelease);

    constexpr bool operator==(const TFullSemVer& other) const noexcept {
        return Version == other.Version && PreRelease == other.PreRelease;
    }
    constexpr bool operator!=(const TFullSemVer& other) const noexcept {
        return !(*this == other);
    }
    bool operator>(const TFullSemVer& other) const noexcept {
        return !(*this <= other);
    }
    bool operator>=(const TFullSemVer& other) const noexcept {
        return !(*this < other);
    }
    bool operator<(const TFullSemVer& other) const noexcept;
    bool operator<=(const TFullSemVer& other) const noexcept {
        return !(other < *this);
    }

    TString ToString() const;

    static TMaybe<TFullSemVer> FromString(TStringBuf str);
};

Y_DECLARE_OUT_SPEC(inline, TSemVer, out, t) {
    out << t.ToString();
}

Y_DECLARE_OUT_SPEC(inline, TFullSemVer, out, t) {
    out << t.ToString();
}

template <>
void Out<TFullSemVer::TTag>(IOutputStream& out, const TFullSemVer::TTag& tags);
template <>
void Out<TFullSemVer::TTags>(IOutputStream& out, const TFullSemVer::TTags& tags);
