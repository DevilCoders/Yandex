package ru.yandex.ci.storage.core.db.clickhouse.old_test;

import java.io.IOException;
import java.sql.Date;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.time.LocalDate;
import java.time.ZoneOffset;
import java.util.Collection;
import java.util.List;

import ru.yandex.ci.storage.core.clickhouse.ClickhouseTable;
import ru.yandex.ci.storage.core.clickhouse.sp.Result;
import ru.yandex.ci.storage.core.db.clickhouse.utils.ExtendedRowBinaryStream;
import ru.yandex.clickhouse.BalancedClickhouseDataSource;

public class OldTestTable extends ClickhouseTable<OldTestEntity> {
    private static final String TABLE_NAME = "tests";
    private final String databaseName;

    public OldTestTable(String databaseName, BalancedClickhouseDataSource clickhouseDataSource) {
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
                (date, id, str_id, toolchain, type, path, name, subtest_name, is_suite, suite_id)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"""
                .formatted(databaseName, TABLE_NAME);
    }

    @Override
    protected void write(OldTestEntity entity, ExtendedRowBinaryStream stream) throws IOException {
        stream.writeDate(Date.valueOf(entity.getDate()));
        stream.writeInt64(entity.getId());
        stream.writeString(entity.getStrId());
        stream.writeString(entity.getToolchain());
        stream.writeUInt8(entity.getType().value());
        stream.writeNullableString(entity.getPath());
        stream.writeNullableString(entity.getName());
        stream.writeNullableString(entity.getSubtestName());
        stream.writeBool(entity.isSuite());
        stream.writeNullableString(entity.getSuiteId());
    }

    public List<OldTestEntity> getTests(Collection<String> ids) throws SQLException {
        var query = """
                SELECT id,
                            any(str_id) AS str_id2,
                            any(toolchain) AS toolchain2,
                            any(type) AS type2,
                            any(path) AS path2,
                            any(name) AS name2,
                            any(subtest_name) AS subtest_name2,
                            any(is_suite) AS is_suite2,
                            any(suite_id) AS suite_id2
                FROM %s.%s
                WHERE str_id IN(?)
                GROUP BY id
                """.formatted(this.databaseName, TABLE_NAME);

        return this.tx(connection -> {
                    var statement = connection.prepareStatement(query);
                    statement.setArray(1, connection.createArrayOf("STRING", ids.toArray()));
                    return this.readResult(statement.executeQuery(), this::readTest);
                }
        );
    }

    private OldTestEntity readTest(ResultSet rs) throws SQLException {
        return new OldTestEntity(
                LocalDate.now(ZoneOffset.UTC),
                rs.getLong("id"),
                rs.getString("str_id2"),
                rs.getString("toolchain2"),
                Result.TestType.valueOf(rs.getString("type2")),
                rs.getString("path2"),
                rs.getString("name2"),
                rs.getString("subtest_name2"),
                rs.getBoolean("is_suite2"),
                rs.getString("suite_id2")
        );
    }
}
