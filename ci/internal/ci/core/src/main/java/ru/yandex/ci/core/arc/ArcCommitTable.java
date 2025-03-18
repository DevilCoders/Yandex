package ru.yandex.ci.core.arc;

import java.util.Collection;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class ArcCommitTable extends KikimrTableCi<ArcCommit> {

    public ArcCommitTable(QueryExecutor executor) {
        super(ArcCommit.class, executor);
    }

    public List<ArcCommit> findByRevisions(Collection<ArcRevision> revisions) {
        return find(revisions.stream()
                .map(ArcRevision::toCommitId)
                .collect(Collectors.toSet()));
    }

    public ArcCommit get(String commitId) {
        return get(ArcCommit.Id.of(commitId));
    }

    public Optional<ArcCommit> findOptional(String commitId) {
        return find(ArcCommit.Id.of(commitId));
    }

    public List<ArcCommit.Id> findChildCommitIds(String commitId) {
        return byParentCommitId()
                .find(List.of(
                        YqlPredicate.where("id.parentCommitId").eq(commitId)
                ))
                .stream()
                .map(it -> it.getId().getCommitId())
                .collect(Collectors.toList());
    }

    @Override
    public void deleteAll() {
        byParentCommitId().deleteAll();
        super.deleteAll();
    }

    private KikimrTable<ArcCommit.ByParentCommitId> byParentCommitId() {
        return new KikimrTable<>(ArcCommit.ByParentCommitId.class, executor);
    }
}
