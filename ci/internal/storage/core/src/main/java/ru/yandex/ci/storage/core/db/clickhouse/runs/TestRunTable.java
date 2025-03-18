package ru.yandex.ci.storage.core.db.clickhouse.runs;

import java.io.IOException;
import java.sql.Date;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.time.LocalDate;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

import ru.yandex.ci.storage.core.clickhouse.ClickhouseTable;
import ru.yandex.ci.storage.core.clickhouse.sp.Result;
import ru.yandex.ci.storage.core.db.clickhouse.utils.ExtendedRowBinaryStream;
import ru.yandex.clickhouse.BalancedClickhouseDataSource;

public class TestRunTable extends ClickhouseTable<TestRunEntity> {
    private static final String TABLE_NAME = "runs_v3";
    private final String databaseName;

    public TestRunTable(String databaseName, BalancedClickhouseDataSource clickhouseDataSource) {
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
                (date, id, task_id, test_id, status, error_type, owner_groups, owner_logins,
                uid, snippet, duration, test_size, tags, requirements, metric_keys, metrics, results)
                VALUES (?, ?, ?, ?,?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"""
                .formatted(databaseName, TABLE_NAME);
    }

    @Override
    protected void write(TestRunEntity entity, ExtendedRowBinaryStream stream) throws IOException {
        stream.writeDate(Date.valueOf(entity.getDate()));
        stream.writeUInt64(entity.getId());
        stream.writeUInt64(entity.getTaskId());
        stream.writeUInt64(entity.getTestId());
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
        stream.writeStringArray(entity.getMetricKeys());
        stream.writeFloat64Array(entity.getMetricValues());
        stream.writeNullableString(entity.getResults());
    }

    public List<TestRunEntity> getRuns(long oldId, LocalDate since, LocalDate until) {
        var query = """
                SELECT date, id, task_id, test_id, status, error_type, owner_groups, owner_logins,
                uid, snippet, duration, test_size, tags, requirements, metric_keys, metrics, results
                FROM %s.%s
                WHERE date > ? and date < ? and test_id = ?
                """.formatted(this.databaseName, TABLE_NAME);

        return this.tx(connection -> {
                    var statement = connection.prepareStatement(query);
                    statement.setQueryTimeout(60);
                    statement.setDate(1, Date.valueOf(since));
                    statement.setDate(2, Date.valueOf(until));
                    statement.setLong(3, oldId);

                    return this.readResult(statement.executeQuery(), this::readRun);
                }
        );
    }

    private TestRunEntity readRun(ResultSet rs) throws SQLException {
        return new TestRunEntity(
                rs.getDate("date").toLocalDate(),
                rs.getLong("id"),
                rs.getLong("task_id"),
                rs.getLong("test_id"),
                Result.Status.valueOf(rs.getString("status")),
                rs.getString("error_type") == null ? null : Result.ErrorType.valueOf(rs.getString("error_type")),
                getStringArray(rs, "owner_groups"),
                getStringArray(rs, "owner_logins"),
                rs.getString("uid"),
                rs.getString("snippet"),
                rs.getDouble("duration"),
                rs.getString("test_size") == null ? null : Result.TestSize.valueOf(rs.getString("test_size")),
                getStringArray(rs, "tags"),
                rs.getString("requirements"),
                getStringArray(rs, "metric_keys"),
                getDoubleArray(rs, "metrics"),
                rs.getString("results")
        );
    }

    private List<Double> getDoubleArray(ResultSet rs, String column) throws SQLException {
        return Arrays.stream((double[]) rs.getArray(column).getArray()).boxed().toList();
    }

    private List<String> getStringArray(ResultSet rs, String column) throws SQLException {
        return Arrays.stream((String[]) rs.getArray(column).getArray()).collect(Collectors.toList());
    }
}
