#pragma once

#include "exception.h"
#include "holder.h"

#include <contrib/libs/libgit2/include/git2.h>

#include <util/generic/string.h>

#include <chrono>

namespace NLibgit2 {
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

    /*
     * The signature represents a subject to autorization.
     * Due to weird data model, commits signed with this signature will also have
     * commit time set to the time stored internally.
     *
     * All ctors will use current time as a reasonable default.
     */
    class TSignature {
    public:
        TSignature() = default;

        explicit TSignature(const TString& name, const TString& email) {
            git_signature* sign;
            GitThrowIfError(git_signature_now(&sign, name.c_str(), email.c_str()));
            Sign_.Reset(sign);
        }

        explicit TSignature(const TString& name, const TString& email, git_time_t time, int offset) {
            git_signature* sign;
            GitThrowIfError(git_signature_new(&sign, name.c_str(), email.c_str(), time, offset));
            Sign_.Reset(sign);
        }

        static TSignature FromString(const TString& buffer);

        [[nodiscard]] git_signature* Get() {
            return Sign_.Get();
        }

        [[nodiscard]] const git_signature* Get() const {
            return Sign_.Get();
        }

        TString Name() const;
        TString Email() const;
        TimePoint Time() const;

        // WARN: fractional part of commit time will be lost during libgit2 adoption
        TSignature& SetTime(TimePoint timePoint);

    private:
        explicit TSignature(git_signature*);
        NPrivate::THolder<git_signature, git_signature_free> Sign_;
    };
} // namespace NLibgit2
