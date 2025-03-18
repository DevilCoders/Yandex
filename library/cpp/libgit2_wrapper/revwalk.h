#pragma once

#include "exception.h"
#include "holder.h"
#include "repository.h"

#include <contrib/libs/libgit2/include/git2.h>

#include <util/generic/iterator.h>

namespace NLibgit2 {
    class TRevWalk : public TInputRangeAdaptor<TRevWalk> {
    public:
        explicit TRevWalk(TRepository& repo);

        const git_oid* Next() {
            int error = git_revwalk_next(&Oid_, Walk_.Get());
            if (error == GIT_ITEROVER) {
                return nullptr;
            }
            GitThrowIfError(error);
            return &Oid_;
        }

        git_revwalk* Get() {
            return Walk_.Get();
        }

    private:
        NPrivate::THolder<git_revwalk, git_revwalk_free> Walk_;
        git_oid Oid_;
    };
} // namespace NLibgit2
