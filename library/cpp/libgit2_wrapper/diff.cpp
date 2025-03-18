#include "diff.h"

#include "commit.h"
#include "repository.h"

namespace NLibgit2 {

    TDiff::TDiff(TRepository& repo, const TStringBuf& fromRev, const TStringBuf& toRev) {
        TCommit fromCommit(repo, TOid(fromRev));
        TCommit toCommit(repo, TOid(toRev));
        git_diff* diff;
        GitThrowIfError(git_diff_tree_to_tree(&diff, repo.Get(), fromCommit.Tree().Get(), toCommit.Tree().Get(), nullptr));
        Diff_.Reset(diff);
    }

    TDiff::TDiff(git_diff* diff)
            : Diff_(diff)
    {
    }

    bool TDiff::empty() const {
        size_t deltaCount = git_diff_num_deltas(Diff_.Get());
        return (deltaCount == 0);
    }

    int TDiff::TraverseDiff(const git_diff_delta* delta, float progress, void* payload) {
        auto *cb = reinterpret_cast<TDeltaCallback *>(payload);
        TDiffDelta Delta = {
                .NewFile = {
                        .Mode = TGitFileMode(delta->new_file.mode),
                        .Path = delta->new_file.path,
                },
                .OldFile = {
                        .Mode = TGitFileMode(delta->old_file.mode),
                        .Path = delta->old_file.path,
                },
                .Status = delta->status
        };
        (*cb)(Delta, progress);
        return 0;
    }

}
