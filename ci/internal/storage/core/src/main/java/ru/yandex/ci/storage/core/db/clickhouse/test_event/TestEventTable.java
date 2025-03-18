package ru.yandex.ci.storage.core.db.clickhouse.test_event;

import java.io.IOException;
import java.sql.Timestamp;

import ru.yandex.ci.storage.core.clickhouse.ClickhouseTable;
import ru.yandex.ci.storage.core.clickhouse.sp.Result;
import ru.yandex.ci.storage.core.db.clickhouse.utils.ExtendedRowBinaryStream;
import ru.yandex.clickhouse.BalancedClickhouseDataSource;

public class TestEventTable extends ClickhouseTable<TestEventEntity> {
    private static final String TABLE_NAME = "events";
    private final String databaseName;

    public TestEventTable(String databaseName, BalancedClickhouseDataSource clickhouseDataSource) {
        super(clickhouseDataSource);
        this.databaseName = databaseName;
    }

    @Override
    protected String getLocalTableName() {
        return TABLE_NAME;
    }

    @Override
    protected String getInsertSql() {
        return """
                INSERT INTO %s.%s
                (date_time, revision, branch_id, author, message, strong_modes, strong_stages, main_job_names,
                main_job_partitions, disabled_toolchains, fast_targets, advised_pool)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"""
                .formatted(databaseName, TABLE_NAME);
    }

    @Override
    protected void write(TestEventEntity entity, ExtendedRowBinaryStream stream) throws IOException {
        stream.writeDateTime(Timestamp.from(entity.getDate()));
        stream.writeUInt32(entity.getRevision());
        stream.writeUInt32(entity.getBranchId());
        stream.writeString(entity.getAuthor());
        stream.writeString(entity.getMessage());
        stream.writeUInt8Array(
                entity.getStrongModes().stream().mapToInt(TestEventEntity.StrongMode::value).toArray()
        );
        stream.writeUInt8Array(
                entity.getStrongStages().stream().mapToInt(Result.TestType::value).toArray()
        );
        stream.writeStringArray(entity.getMainJobNames());
        stream.writeUInt8Array(entity.getMainJobPartitions());
        stream.writeStringArray(entity.getDisabledToolchains());
        stream.writeStringArray(entity.getFastTargets());
        stream.writeString(entity.getAdvisedPool());
    }
}
