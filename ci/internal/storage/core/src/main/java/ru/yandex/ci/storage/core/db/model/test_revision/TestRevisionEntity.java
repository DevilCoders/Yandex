package ru.yandex.ci.storage.core.db.model.test_revision;

import java.time.Instant;

import javax.annotation.Nullable;

import com.yandex.ydb.table.settings.PartitioningSettings;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.client.YdbTableHint;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.ydb.HintRegistry;

@Value
@AllArgsConstructor
@Builder(toBuilder = true)
@Table(name = "TestRevisions")
public class TestRevisionEntity implements Entity<TestRevisionEntity> {

    @SuppressWarnings("UnusedVariable")
    private static YdbTableHint ydbTableHint;

    static {
        HintRegistry.getInstance().hint(
                TestRevisionEntity.class,
                512,
                partitions -> ydbTableHint = HintRegistry.uniformAndSplitMerge(
                        partitions,
                        new PartitioningSettings()
                                .setPartitioningByLoad(true)
                                .setPartitioningBySize(true)
                                .setMinPartitionsCount(Math.min(partitions, 512))
                                .setMaxPartitionsCount(8192)
                )
        );
    }

    Id id;

    Common.TestStatus previousStatus;
    Common.TestStatus status;

    @Nullable
    String uid;

    long previousRevision;
    long nextRevision;

    @Column(dbType = DbType.TIMESTAMP)
    Instant revisionCreated;

    boolean changed;

    @Override
    public Id getId() {
        return id;
    }

    public String getUid() {
        return uid == null ? "" : uid;
    }

    public TestImportantRevisionEntity toImportant() {
        return new TestImportantRevisionEntity(
                new TestImportantRevisionEntity.Id(this.id),
                this.previousStatus,
                this.status,
                this.uid,
                revisionCreated,
                this.changed
        );
    }

    @Value
    @AllArgsConstructor
    @lombok.Builder(toBuilder = true)
    public static class Id implements Entity.Id<TestRevisionEntity> {
        TestStatusEntity.Id statusId;
        long revision;
    }

    @Value
    public static class RevisionView implements yandex.cloud.repository.db.Table.View {
        RevisionViewId id;
    }

    @Value
    public static class RevisionViewId implements Entity.Id<TestRevisionEntity> {
        StatusIdView statusId;
        long revision;
    }

    @Value
    public static class StatusIdView {
        @Column(dbType = DbType.UINT64)
        long testId;

        @Column(dbType = DbType.UINT64)
        long suiteId;

        String branch;
    }
}
