package ru.yandex.ci.core.arc;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.common.ydb.KikimrProjectionCI;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true)
@Table(name = "main/Commit")
public class ArcCommit implements Entity<ArcCommit>, CommitId, KikimrProjectionCI {

    @Column(name = "commitId")
    ArcCommit.Id id;

    @With
    @Column(dbType = DbType.UTF8)
    String author;

    @With
    @Column(dbType = DbType.UTF8)
    String message;

    @Column(dbType = DbType.TIMESTAMP)
    Instant createTime;

    @With
    @Column(dbType = DbType.JSON, flatten = false)
    List<String> parents;

    @Column
    long svnRevision;

    @Column
    long pullRequestId;

    @Override
    public List<Entity<?>> createProjections() {
        return new ArrayList<>(ByParentCommitId.of(this));
    }

    @Override
    public ArcCommit.Id getId() {
        return id;
    }

    @Override
    public String getCommitId() {
        return id.getCommitId();
    }

    public ArcRevision getRevision() {
        return ArcRevision.of(id.getCommitId());
    }

    /**
     * Возвращает true, если коммит досягаем с головы trunk-а, иными словами - он в транке.
     * Важно: для некоторых процессов необходимо различать ситуацию, когда коммит в транке, но на нем отведена ветка.
     * Процесс может быть запущен на этой ревизии как в транке, так и в ветке. Для таких случаев ветку, с высоты которой
     * мы смотрим на данный коммит надо брать откуда-то извне.
     */
    public boolean isTrunk() {
        return svnRevision > 0;
    }

    public OrderedArcRevision toOrderedTrunkArcRevision() {
        Preconditions.checkState(
                isTrunk(),
                "Can't make ordered not trunk commit: %s", this
        );
        return getRevision().toOrdered(ArcBranch.trunk(), svnRevision, pullRequestId);
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<ArcCommit> {
        @Column(dbType = DbType.UTF8)
        String commitId;
    }

    @VisibleForTesting
    public ArcCommit withParent(ArcCommit parent) {
        return withParents(List.of(parent.getCommitId()));
    }

    @Value(staticConstructor = "of")
    @Table(name = "main/Commit_ByParentCommitId")
    public static class ByParentCommitId implements Entity<ByParentCommitId> {

        ByParentCommitId.Id id;

        @Override
        public ByParentCommitId.Id getId() {
            return id;
        }

        public static List<ByParentCommitId> of(ArcCommit arcCommit) {
            return arcCommit.getParents()
                    .stream()
                    .map(parentCommitId -> ByParentCommitId.of(
                            ByParentCommitId.Id.of(parentCommitId, arcCommit.getId())
                    ))
                    .collect(Collectors.toList());
        }

        @Value(staticConstructor = "of")
        public static class Id implements Entity.Id<ByParentCommitId> {
            @Column(name = "idx_parentCommitId", dbType = DbType.UTF8)
            String parentCommitId;
            ArcCommit.Id commitId;
        }
    }
}
