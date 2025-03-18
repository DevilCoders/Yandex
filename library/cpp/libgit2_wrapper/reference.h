#pragma once

#include "holder.h"
#include "exception.h"
#include "object_id.h"

#include <contrib/libs/libgit2/include/git2.h>

#include <util/generic/iterator.h>

namespace NLibgit2 {
    class TRepository;

    class TRef {
    public:
        explicit TRef(git_reference* ref): Ref_(ref) {}

        TRef Resolve() {
            git_reference* ref;
            GitThrowIfError(git_reference_resolve(&ref, Ref_.Get()));
            return TRef(ref);
        }

        TOid Target() {
            return *git_reference_target(Ref_.Get());
        }

        git_reference* Get() {
            return Ref_.Get();
        }

    private:
        NPrivate::THolder<git_reference, git_reference_free> Ref_;
    };

    class TRefs : public TInputRangeAdaptor<TRefs> {
    public:
        explicit TRefs(TRepository& repo);

        NPrivate::THolder<git_reference, git_reference_free> Next() {
            git_reference* ref;
            int error = git_reference_next(&ref, Iter_.Get());
            if (error == GIT_ITEROVER) {
                return nullptr;
            }
            GitThrowIfError(error);
            return NPrivate::THolder<git_reference, git_reference_free>(ref);
        }

    private:
        NPrivate::THolder<git_reference_iterator, git_reference_iterator_free> Iter_;
    };
} // namespace NLibgit2
