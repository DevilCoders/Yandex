#include "tree.h"

namespace NLibgit2 {
    TTree::TTree(TRepository& repo, const TOid& id) {
        git_tree* tree;
        GitThrowIfError(git_tree_lookup(&tree, repo.Get(), id.Get()));
        Tree_.Reset(tree);
    }

    TTreeIterator TTree::begin() {
        return { Tree_.Get(), 0 };
    }

    TTreeIterator TTree::end() {
        return { Tree_.Get(), git_tree_entrycount(Tree_.Get()) };
    }
} // namespace NLibgit2
