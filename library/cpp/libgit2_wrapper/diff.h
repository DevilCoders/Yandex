#pragma once

#include "callbacks.h"
#include "exception.h"
#include "holder.h"

#include <contrib/libs/libgit2/include/git2.h>

#include <util/folder/path.h>

namespace NLibgit2 {

    class TRepository;

    using TGitFileMode = git_filemode_t;

    struct TDiffFile {
        TGitFileMode Mode;
        TFsPath Path;
    };

    using TDiffStatus = git_delta_t;

    struct TDiffDelta {
        TDiffFile NewFile;
        TDiffFile OldFile;
        TDiffStatus Status;
    };

    class TDiff {
    public:
        explicit TDiff(const TString &content) {
            git_diff *diff;
            GitThrowIfError(git_diff_from_buffer(&diff, content.c_str(), content.size()));
            Diff_.Reset(diff);
        };

        explicit TDiff(git_diff *diff);

        TDiff(TRepository &repo, const TStringBuf &fromRev, const TStringBuf &toRev);

        [[nodiscard]] bool empty() const;

        void ForEachDelta(TDeltaCallback cb) {
            GitThrowIfError(git_diff_foreach(Diff_.Get(), TraverseDiff, nullptr, nullptr, nullptr, &cb));
        }

        [[nodiscard]] git_diff *Get() {
            return Diff_.Get();
        }

    private:
        NPrivate::THolder<git_diff, git_diff_free> Diff_;

        static int TraverseDiff(const git_diff_delta *delta, float progress, void *payload);
    };

}
