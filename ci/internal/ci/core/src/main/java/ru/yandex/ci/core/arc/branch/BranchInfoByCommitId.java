package ru.yandex.ci.core.arc.branch;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@Value
@Table(name = "main/BranchInfoByCommitId")
public class BranchInfoByCommitId implements Entity<BranchInfoByCommitId> {
    public static final String COMMIT_ID = "id.commitId";

    Id id;

    @Override
    public Id getId() {
        return id;
    }

    public static BranchInfoByCommitId of(BranchInfo item) {
        return new BranchInfoByCommitId(
                Id.of(item.getBaseRevision().getCommitId(), item.getArcBranch().asString())
        );
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<BranchInfoByCommitId> {
        @Column(name = "idx_commitId", dbType = DbType.UTF8)
        String commitId;

        @Column(name = "idx_branchName", dbType = DbType.UTF8)
        String branch;
    }
}
