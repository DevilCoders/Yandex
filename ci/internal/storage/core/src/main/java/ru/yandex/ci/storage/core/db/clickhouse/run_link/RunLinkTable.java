package ru.yandex.ci.storage.core.db.clickhouse.run_link;

import java.io.IOException;
import java.sql.Date;

import ru.yandex.ci.storage.core.clickhouse.ClickhouseTable;
import ru.yandex.ci.storage.core.db.clickhouse.utils.ExtendedRowBinaryStream;
import ru.yandex.clickhouse.BalancedClickhouseDataSource;

public class RunLinkTable extends ClickhouseTable<RunLinkEntity> {
    private static final String TABLE_NAME = "run_links";
    private final String databaseName;

    public RunLinkTable(String databaseName, BalancedClickhouseDataSource clickhouseDataSource) {
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
                (date, run_id, test_id, names, links)
                VALUES (?, ?, ?, ?, ?)"""
                .formatted(databaseName, TABLE_NAME);
    }

    @Override
    protected void write(RunLinkEntity entity, ExtendedRowBinaryStream stream) throws IOException {
        stream.writeDate(Date.valueOf(entity.getDate()));
        stream.writeInt64(entity.getRunId());
        stream.writeInt64(entity.getTestId());
        stream.writeStringArray(entity.getNames());
        stream.writeStringArray(entity.getLinks());
    }
}

