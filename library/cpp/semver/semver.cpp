#include "semver.h"

#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/printf.h>

namespace {

TFullSemVer::TTag TagFromString(TStringBuf s) {
    ui16 number = 0;
    if (TryFromString(s, number)) {
        return number;
    }
    return s;
}

} // namespace

TString TSemVer::ToString() const {
    TString base = Sprintf("%d.%d.%d", Major, Minor, Patch);
    return Build == 0 ? base : TStringBuilder() << base << '.' << Build;
}

// static
TMaybe<TSemVer> TSemVer::FromString(TStringBuf version) {
    TStringBuf v(version);
    ui16 major = 0;
    ui16 minor = 0;
    ui16 patch = 0;
    ui16 build = 0;
    TStringBuf part;
    if ((part = v.NextTok('.')) && !TryFromString(part, major))
        return TMaybe<TSemVer>();
    if ((part = v.NextTok('.')) && !TryFromString(part, minor))
        return TMaybe<TSemVer>();
    if ((part = v.NextTok('.')) && !TryFromString(part, patch))
        return TMaybe<TSemVer>();
    if ((part = v.NextTok('.')) && !TryFromString(part, build))
        return TMaybe<TSemVer>();
    return TMaybe<TSemVer>(TSemVer(major, minor, patch, build));
}

TFullSemVer::TFullSemVer(TSemVer version, const TTags& preRelease)
    : Version(version)
    , PreRelease(preRelease)
{
}

// static
TMaybe<TFullSemVer> TFullSemVer::FromString(TStringBuf str) {
    TMaybe<TSemVer> version = TSemVer::FromString(str.NextTok('-'));
    if (!version.Defined()) {
        return Nothing();
    }

    TStringBuf tokens = str.NextTok('+');
    TTags preRelease;
    while (tokens) {
        preRelease.push_back(TagFromString(tokens.NextTok('.')));
    }
    return TFullSemVer(version.GetRef(), preRelease);
}

TString TFullSemVer::ToString() const {
    TStringBuilder b;
    b << Version;
    if (PreRelease) {
        b << '-' << PreRelease;
    }
    return b;
}

bool TFullSemVer::operator<(const TFullSemVer& other) const noexcept {
    if (Version != other.Version) {
        return Version < other.Version;
    }

    // When major, minor, and patch are equal, a pre-release version has lower precedence than a normal version.
    if (PreRelease.empty()) {
        return false;
    }

    if (other.PreRelease.empty()) {
        return true;
    }

    return std::lexicographical_compare(PreRelease.begin(), PreRelease.end(),
                                        other.PreRelease.begin(), other.PreRelease.end());
}

TString TFullSemVer::TTag::ToString() const {
    TStringBuilder str;
    struct TVisitor {
        TStringBuilder& Out;

        void operator()(const TString& val) {
            Out << val;
        }
        void operator()(ui16 val) {
            Out << val;
        }
    };
    std::visit(TVisitor{str}, Variant);
    return str;
}

bool TFullSemVer::TTag::operator<(const TFullSemVer::TTag& other) const noexcept {
    struct TLhsVisitor {
        const TFullSemVer::TTag::TValue& Rhs;

        bool operator()(ui16 lhs) const noexcept {
            if (std::holds_alternative<TString>(Rhs)) {
                return true;
            }
            return lhs < std::get<ui16>(Rhs);
        }
        bool operator()(const TString& lhs) const noexcept {
            if (std::holds_alternative<ui16>(Rhs)) {
                return false;
            }
            return lhs < std::get<TString>(Rhs);
        }
    };

    return std::visit(TLhsVisitor{other.Variant}, Variant);
}

template <>
void Out<TFullSemVer::TTag>(IOutputStream& out, const TFullSemVer::TTag& tag) {
    out << tag.ToString();
}

template <>
void Out<TFullSemVer::TTags>(IOutputStream& out, const TFullSemVer::TTags& tags) {
    out << JoinSeq(".", tags);
}
