package ru.yandex.ci.storage.core.db.clickhouse.change_run;

import java.io.IOException;
import java.sql.Date;

import ru.yandex.ci.storage.core.clickhouse.ClickhouseTable;
import ru.yandex.ci.storage.core.db.clickhouse.utils.ExtendedRowBinaryStream;
import ru.yandex.clickhouse.BalancedClickhouseDataSource;

public class ChangeRunTable extends ClickhouseTable<ChangeRunEntity> {
    private static final String TABLE_NAME = "runs_by_test_id_v2";
    private final String databaseName;

    public ChangeRunTable(String databaseName, BalancedClickhouseDataSource clickhouseDataSource) {
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
                (date, test_id, revision, branch_id, run_id, status, error_type,
                affected, counter, important, muted)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"""
                .formatted(databaseName, TABLE_NAME);
    }

    @Override
    protected void write(ChangeRunEntity entity, ExtendedRowBinaryStream stream) throws IOException {
        stream.writeDate(Date.valueOf(entity.getDate()));
        stream.writeUInt64(entity.getTestId());
        stream.writeUInt32(entity.getRevision());
        stream.writeUInt32(entity.getBranchId());
        stream.writeNullableInt64(entity.getRunId());
        stream.writeNullableUInt8((entity.getStatus() == null) ? null : entity.getStatus().value());
        stream.writeNullableUInt8((entity.getErrorType() == null) ? null : entity.getErrorType().value());
        stream.writeBool(entity.isAffected());
        stream.writeInt64(entity.getCounter());
        stream.writeBool(entity.isImportant());
        stream.writeBool(entity.isMuted());
    }
}
