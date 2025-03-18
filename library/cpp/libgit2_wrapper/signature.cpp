#include "signature.h"

#include <contrib/libs/libgit2/include/git2/signature.h>
#include <time.h>

namespace NLibgit2 {

    TSignature::TSignature(git_signature* toAdopt)
        : Sign_(toAdopt)
    {
    }

    TSignature TSignature::FromString(const TString& buffer) {
        git_signature* out;
        GitThrowIfError(git_signature_from_buffer(&out, buffer.data()));
		out->when.time = time(nullptr);
		out->when.offset = 0;
        return TSignature(out);
    }

    TString TSignature::Name() const {
        return Sign_->name;
    }

    TString TSignature::Email() const {
        return Sign_->email;
    }

    TimePoint TSignature::Time() const {
        return TimePoint::clock::from_time_t(Sign_->when.time);
    }

    TSignature& TSignature::SetTime(TimePoint timePoint) {
        Sign_->when.time = TimePoint::clock::to_time_t(timePoint);
        return *this;
    }
}
