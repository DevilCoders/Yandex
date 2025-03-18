#pragma once

#include "repository.h"

#include <contrib/libs/libgit2/include/git2.h>

namespace NLibgit2 {
    class TTreeBuilder {
    public:
        explicit TTreeBuilder(TRepository& repo) {
            git_treebuilder* builder;
            GitThrowIfError(git_treebuilder_new(&builder, repo.Get(), nullptr));
            Builder_.Reset(builder);
        }

        void Clear() {
            git_treebuilder_clear(Builder_.Get());
        }

        ui64 EntryCount() {
            return git_treebuilder_entrycount(Builder_.Get());
        }

        void Insert(const TString& name, const git_oid& id, git_filemode_t mode) {
            GitThrowIfError(git_treebuilder_insert(nullptr, Builder_.Get(), name.c_str(), &id, mode));
        }

        void Remove(const TString& name) {
            GitThrowIfError(git_treebuilder_remove(Builder_.Get(), name.c_str()));
        }

        git_oid Write() {
            git_oid res;
            GitThrowIfError(git_treebuilder_write(&res, Builder_.Get()));
            return res;
        }

    private:
        NPrivate::THolder<git_treebuilder, git_treebuilder_free> Builder_;
    };

    class TTreeEntryRef {
    public:
        explicit TTreeEntryRef(const git_tree_entry* entry): TreeEntry_(entry) {}

        [[nodiscard]] const git_tree_entry* Get() const {
            return TreeEntry_;
        }

        [[nodiscard]] TGitFileMode FileMode() {
            return git_tree_entry_filemode(TreeEntry_);
        }

        [[nodiscard]] TOid Id() {
            return *git_tree_entry_id(TreeEntry_);
        }

        [[nodiscard]] TString Name() {
            return git_tree_entry_name(TreeEntry_);
        }

    private:
        const git_tree_entry* TreeEntry_;
    };

    class TTreeIterator {
    public:
        TTreeIterator(git_tree* tree, size_t index): Tree_(tree), Index_(index) {}

        bool operator!=(TTreeIterator right) {
            return Tree_ != right.Tree_ || Index_ != right.Index_;
        }

        TTreeEntryRef operator*() {
            return TTreeEntryRef(git_tree_entry_byindex(Tree_, Index_));
        }

        void operator++ () {
            ++Index_;
        }

    private:
        git_tree* Tree_;
        size_t Index_;
    };

    class TTree {
    public:
        explicit TTree(TRepository& repo, const TOid& id);

        explicit TTree(git_tree* tree): Tree_(tree) {}

        TTreeIterator begin();
        TTreeIterator end();

        [[nodiscard]] git_tree* Get() {
            return Tree_.Get();
        }

        [[nodiscard]] const git_tree* Get() const {
            return Tree_.Get();
        }

    private:
        NPrivate::THolder<git_tree, git_tree_free> Tree_;
    };

    class TTreeEntry {
    public:
        explicit TTreeEntry(TTree&& tree, const TFsPath& path) {
            git_tree_entry* entry;
            GitThrowIfError(git_tree_entry_bypath(&entry, tree.Get(), path.c_str()));
            TreeEntry_.Reset(entry);
        }

        explicit TTreeEntry(TTree& tree, const TFsPath& path) {
            git_tree_entry* entry;
            GitThrowIfError(git_tree_entry_bypath(&entry, tree.Get(), path.c_str()));
            TreeEntry_.Reset(entry);
        }

        [[nodiscard]] TOid Id() const {
            return *git_tree_entry_id(TreeEntry_.Get());
        }

        [[nodiscard]] git_tree_entry* Get() {
            return TreeEntry_.Get();
        }

    private:
        NPrivate::THolder<git_tree_entry, git_tree_entry_free> TreeEntry_;
    };
} // namespace NLibgit2
