#include "reference.h"

#include "repository.h"

namespace NLibgit2 {
    TRefs::TRefs(TRepository &repo) {
        git_reference_iterator *iter;
        GitThrowIfError(git_reference_iterator_new(&iter, repo.Get()));
        Iter_.Reset(iter);
    }
}
