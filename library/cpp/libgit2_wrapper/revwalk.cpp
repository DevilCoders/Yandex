#include "revwalk.h"

namespace NLibgit2 {
    TRevWalk::TRevWalk(TRepository& repo) {
        git_revwalk* walk;
        GitThrowIfError(git_revwalk_new(&walk, repo.Get()));
        Walk_.Reset(walk);
    }
}
