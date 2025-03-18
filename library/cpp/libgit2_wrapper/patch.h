#pragma once

#include "holder.h"
#include "exception.h"

#include <contrib/libs/libgit2/include/git2.h>

namespace NLibgit2 {

    class TPatch {
    public:
        explicit TPatch(const TString &to, const TString &from) {
            git_patch *patch;
            GitThrowIfError(
                    git_patch_from_buffers(&patch, to.c_str(), to.Size(), nullptr, from.c_str(), from.size(), nullptr,
                                           nullptr));
            Patch_.Reset(patch);
        };

        [[nodiscard]] git_patch *Get() {
            return Patch_.Get();
        }

        [[nodiscard]] const git_patch *Get() const {
            return Patch_.Get();
        }

    private:
        NPrivate::THolder<git_patch, git_patch_free> Patch_;
    };

} // namespace NLibgit2
