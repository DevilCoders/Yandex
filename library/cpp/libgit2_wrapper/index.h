#pragma once

#include "exception.h"
#include "holder.h"
#include "object_id.h"

#include <contrib/libs/libgit2/include/git2.h>

namespace NLibgit2 {

    class TIndex {
    public:
        explicit TIndex(git_index* index): Index_(index) {}

        void AddEverything();

        TOid WriteTree();

        void Write() {
            GitThrowIfError(git_index_write(Index_.Get()));
        }

        git_index* Get() {
            return Index_.Get();
        }

    private:
        NPrivate::THolder<git_index, git_index_free> Index_;
    };

} // namespace NLibgit2
