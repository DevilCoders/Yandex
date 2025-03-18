package ru.yandex.ci.core.arc.branch;

import java.util.Collection;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.arc.CommitId;

public class BranchInfoTable extends KikimrTableCi<BranchInfo> {
    public BranchInfoTable(QueryExecutor executor) {
        super(BranchInfo.class, executor);
    }

    public List<BranchInfo> findAtCommit(CommitId commitId) {
        Set<BranchInfo.Id> ids = byCommitId().find(
                YqlPredicate.where(BranchInfoByCommitId.COMMIT_ID).eq(commitId.getCommitId())
        ).stream()
                .map(projection -> BranchInfo.Id.of(projection.getId().getBranch()))
                .collect(Collectors.toSet());

        return find(ids);
    }

    public List<BranchInfoByCommitId> findAtCommits(Collection<? extends CommitId> commits) {
        var commitIds = commits.stream()
                .map(CommitId::getCommitId)
                .collect(Collectors.toSet());

        return byCommitId().find(
                YqlPredicate.where(BranchInfoByCommitId.COMMIT_ID).in(commitIds)
        );
    }

    @Override
    public void deleteAll() {
        byCommitId().deleteAll();
        super.deleteAll();
    }

    private KikimrTable<BranchInfoByCommitId> byCommitId() {
        return new KikimrTable<>(BranchInfoByCommitId.class, executor);
    }
}
