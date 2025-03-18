#pragma once

#include <contrib/libs/libgit2/include/git2.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/string_hash.h>

#include <iostream>

namespace NLibgit2 {
    class TOid {
    public:
        TOid()
        {
            memset(&Oid_, 0, sizeof(Oid_));
        }

        TOid(const git_oid& oid)
                : Oid_(oid)
        { }

        explicit TOid(TStringBuf buf) {
            git_oid oid;
            git_oid_fromstr(&oid, buf.data());
            Oid_ = oid;
        }

        git_oid* Get() {
            return &Oid_;
        }

        [[nodiscard]] const git_oid* Get() const {
            return &Oid_;
        }

        [[nodiscard]] operator git_oid& () {
            return Oid_;
        }

        [[nodiscard]] operator const git_oid& () const {
            return Oid_;
        }

        [[nodiscard]] TString ToString() const;

        [[nodiscard]] bool IsNull() const {
            return *this == TOid();
        }

        int operator<=>(const TOid& other) const {
            return git_oid_cmp(&Oid_, &other.Oid_);
        }

        bool operator==(const TOid& other) const {
            return git_oid_equal(&Oid_, &other.Oid_);
        }

        friend std::ostream& operator<<(std::ostream& os, TOid oid) {
            os << oid.ToString();
            return os;
        }

        [[nodiscard]] size_t Hash() const {
            return static_cast<size_t>(CityHash64(reinterpret_cast<const char*>(Oid_.id),
                                                  sizeof(Oid_.id)));
        }

    private:
        git_oid Oid_;
    };
} // namespace NLibgit2
