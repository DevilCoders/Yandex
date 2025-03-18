package ru.yandex.ci.storage.core.db.model.test_revision;

import java.time.Instant;

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
import ru.yandex.ci.storage.core.ydb.HintRegistry;

@Value
@AllArgsConstructor
@Builder(toBuilder = true)
@Table(name = "TestImportantRevisions")
public class TestImportantRevisionEntity implements Entity<TestImportantRevisionEntity> {
    /*
    Additional sql:
    ALTER TABLE `TestImportantRevisions` SET (AUTO_PARTITIONING_MAX_PARTITIONS_COUNT=512);
    ALTER TABLE `TestImportantRevisions` SET (AUTO_PARTITIONING_BY_LOAD = ENABLED);
     */

    @SuppressWarnings("UnusedVariable")
    private static YdbTableHint ydbTableHint;

    static {
        HintRegistry.getInstance().hint(
                TestImportantRevisionEntity.class,
                16,
                partitions -> ydbTableHint = HintRegistry.uniformAndSplitMerge(
                        partitions,
                        new PartitioningSettings()
                                .setPartitioningByLoad(true)
                                .setPartitioningBySize(true)
                                .setMaxPartitionsCount(512)
                )
        );
    }

    Id id;

    Common.TestStatus previousStatus;
    Common.TestStatus status;
    String uid;

    @Column(dbType = DbType.TIMESTAMP)
    Instant revisionCreated;

    boolean changed;

    @Override
    public Id getId() {
        return id;
    }

    @Value
    @AllArgsConstructor
    public static class Id implements Entity.Id<TestImportantRevisionEntity> {
        @Column(dbType = DbType.UINT64)
        long testId;

        @Column(dbType = DbType.UINT64)
        long suiteId;

        String branch;

        long revision;

        String toolchain;

        public Id(TestRevisionEntity.Id id) {
            this.testId = id.getStatusId().getTestId();
            this.suiteId = id.getStatusId().getSuiteId();
            this.branch = id.getStatusId().getBranch();
            this.revision = id.getRevision();
            this.toolchain = id.getStatusId().getToolchain();
        }
    }


    @Value
    public static class RevisionView implements yandex.cloud.repository.db.Table.View {
        RevisionViewId id;
    }

    @Value
    public static class RevisionViewId implements Entity.Id<TestImportantRevisionEntity> {
        @Column(dbType = DbType.UINT64)
        long testId;

        @Column(dbType = DbType.UINT64)
        long suiteId;

        String branch;

        long revision;
    }
}
