#pragma once

#include "object_id.h"
#include "repository.h"
#include "tree.h"

#include <contrib/libs/libgit2/include/git2.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>

namespace NLibgit2 {
    class TCommit {
    public:
        explicit TCommit(TRepository& repo, const TOid& id) {
            git_commit* commit;
            GitThrowIfError(git_commit_lookup(&commit, repo.Get(), id.Get()));
            Commit_.Reset(commit);
        }

        TCommit(TCommit&& other) noexcept {
            std::swap(Commit_, other.Commit_);
        }

        TCommit& operator=(TCommit&& other) noexcept {
            std::swap(Commit_, other.Commit_);
            return *this;
        }

        [[nodiscard]] const git_oid& Id() const {
            Y_ENSURE(Commit_);
            return *git_commit_id(Commit_.Get());
        }

        [[nodiscard]] const git_oid& TreeId() const {
            Y_ENSURE(Commit_);
            return *git_commit_tree_id(Commit_.Get());
        }

        [[nodiscard]] TTree Tree() const {
            Y_ENSURE(Commit_);
            git_tree *tree;
            GitThrowIfError(git_commit_tree(&tree, Commit_.Get()));
            return TTree(tree);
        }

        git_commit* Get() {
            return Commit_.Get();
        }

        [[nodiscard]] const git_commit* Get() const {
            return Commit_.Get();
        }

        [[nodiscard]] TStringBuf Message() const {
            return git_commit_message(Commit_.Get());
        }

        const git_oid& ParentId(unsigned int n) {
            return *git_commit_parent_id(Commit_.Get(), n);
        }

        size_t ParentCount() {
            return git_commit_parentcount(Commit_.Get());
        }

        TInstant Time() {
            return TInstant::Seconds(git_commit_time(Commit_.Get()));
        }

    private:
        NPrivate::THolder<git_commit, git_commit_free> Commit_;
    };
} // namespace NLibgit2
