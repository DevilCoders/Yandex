#include "object_db.h"

#include "repository.h"

namespace NLibgit2 {
    TObjectDB::TObjectDB(TRepository &repo) {
        git_odb* db;
        GitThrowIfError(git_repository_odb(&db, repo.Get()));
        DB_.Reset(db);
    }
}
