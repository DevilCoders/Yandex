package ru.yandex.ci.storage.core.db.clickhouse.last_run;

import java.io.IOException;
import java.sql.Date;

import ru.yandex.ci.storage.core.clickhouse.ClickhouseTable;
import ru.yandex.ci.storage.core.db.clickhouse.utils.ExtendedRowBinaryStream;
import ru.yandex.clickhouse.BalancedClickhouseDataSource;

public class LastRunTable extends ClickhouseTable<LastRunEntity> {
    private static final String TABLE_NAME = "last_test_runs";
    private final String databaseName;

    public LastRunTable(String databaseName, BalancedClickhouseDataSource clickhouseDataSource) {
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
                (date, active, test_id, branch_id, revision, run_id, counter,
                str_id, toolchain, type, path, name, subtest_name,
                is_suite, suite_id, status, error_type, owner_groups, owner_logins,
                uid, snippet, duration, test_size, tags, requirements,
                link_names, links, metric_keys, metrics, results)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"""
                .formatted(databaseName, TABLE_NAME);
    }

    @Override
    protected void write(LastRunEntity entity, ExtendedRowBinaryStream stream) throws IOException {
        stream.writeDate(Date.valueOf(entity.getDate()));
        stream.writeBool(entity.isActive());
        stream.writeUInt64(entity.getTestId());
        stream.writeUInt32(entity.getBranchId());
        stream.writeUInt32(entity.getRevision());
        stream.writeUInt64(entity.getRunId());
        stream.writeUInt64(entity.getCounter());
        stream.writeString(entity.getStrId());
        stream.writeString(entity.getToolchain());
        stream.writeUInt8(entity.getType().value());
        stream.writeNullableString(entity.getPath());
        stream.writeNullableString(entity.getName());
        stream.writeNullableString(entity.getSubtestName());
        stream.writeBool(entity.isSuite());
        stream.writeNullableString(entity.getSuiteId());
        stream.writeUInt8(entity.getStatus().value());
        stream.writeNullableUInt8((entity.getErrorType() == null) ? null : entity.getErrorType().value());
        stream.writeStringArray(entity.getOwnerGroups());
        stream.writeStringArray(entity.getOwnerLogins());
        stream.writeString(entity.getUid());
        stream.writeNullableString(entity.getSnippet());
        stream.writeNullableFloat64(entity.getDuration());
        stream.writeNullableUInt8((entity.getTestSize() == null) ? null : entity.getTestSize().value());
        stream.writeStringArray(entity.getTags());
        stream.writeNullableString(entity.getRequirements());
        stream.writeStringArray(entity.getLinkNames());
        stream.writeStringArray(entity.getLinks());
        stream.writeStringArray(entity.getMetricKeys());
        stream.writeFloat64Array(entity.getMetricValues());
        stream.writeNullableString(entity.getResults());
    }
}
