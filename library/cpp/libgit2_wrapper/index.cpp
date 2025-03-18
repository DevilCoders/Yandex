#include "index.h"

namespace NLibgit2 {

    TOid TIndex::WriteTree() {
        git_oid oid;
        GitThrowIfError(git_index_write_tree(&oid, Index_.Get()));
        return oid;
    }

    void TIndex::AddEverything() {
        GitThrowIfError(
                git_index_add_all(
                        Index_.Get(),
                        /* pathspec = */ nullptr,
                        // FIXME: make FORCE / DEFAULT option configurable from the interface
                        GIT_INDEX_ADD_FORCE,
                        /* callback = */ nullptr,
                        /* payload = */ nullptr
                )
        );
    }

} // namespace NLibgit2
