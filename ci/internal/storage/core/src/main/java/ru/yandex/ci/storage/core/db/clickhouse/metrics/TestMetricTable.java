package ru.yandex.ci.storage.core.db.clickhouse.metrics;

import java.io.IOException;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Timestamp;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.storage.api.StorageFrontHistoryApi;
import ru.yandex.ci.storage.core.clickhouse.ClickhouseTable;
import ru.yandex.ci.storage.core.db.clickhouse.utils.ExtendedRowBinaryStream;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.clickhouse.BalancedClickhouseDataSource;

public class TestMetricTable extends ClickhouseTable<TestMetricEntity> {
    private static final String TABLE_NAME = "metrics";
    private static final String LOCAL_TABLE_NAME = "metrics_local";
    private final String databaseName;

    private static final List<String> ALL_COLUMNS = List.of(
            "branch",
            "path",
            "test_name",
            "subtest_name",
            "toolchain",
            "metric_name",
            "date",
            "revision",
            "result_type",
            "value",
            "test_status",
            "check_id",
            "suite_id",
            "test_id"
    );
    private static final List<String> ALL_COLUMN_PARAMS = ALL_COLUMNS.stream().map(s -> "?").toList();

    public TestMetricTable(String databaseName, BalancedClickhouseDataSource clickhouseDataSource) {
        super(clickhouseDataSource);
        this.databaseName = databaseName;
    }

    @Override
    protected String getLocalTableName() {
        return LOCAL_TABLE_NAME;
    }

    @Override
    protected String getInsertSql() {
        return "INSERT INTO %s.%s (%s) VALUES (%s)".formatted(
                databaseName, LOCAL_TABLE_NAME, String.join(",", ALL_COLUMN_PARAMS), String.join(",", ALL_COLUMNS));
    }

    @Override
    protected void write(TestMetricEntity entity, ExtendedRowBinaryStream stream) throws IOException {
        stream.writeString(entity.getBranch());
        stream.writeString(entity.getPath());
        stream.writeString(entity.getTestName());
        stream.writeString(entity.getSubtestName() == null ? "" : entity.getSubtestName());
        stream.writeString(entity.getToolchain());
        stream.writeString(entity.getMetricName());
        stream.writeDateTime(convert(entity.getTimestamp()));
        stream.writeUInt64(entity.getRevision());
        stream.writeString(entity.getResultType().name());
        stream.writeFloat64(entity.getValue());
        stream.writeString(entity.getTestStatus().name());
        stream.writeUInt64(entity.getCheckId());
        stream.writeUInt64(entity.getSuiteId());
        stream.writeUInt64(entity.getTestId());
    }

    public Set<String> getTestMetrics(TestStatusEntity test) {
        var query = """
                SELECT DISTINCT(metric_name)
                FROM %s
                WHERE branch = ? AND path = ? AND test_name = ? AND subtest_name = ?
                """.formatted(TABLE_NAME);

        var result = this.tx(connection -> {
                    var statement = connection.prepareStatement(query);
                    var index = 1;
                    statement.setString(index++, test.getId().getBranch());
                    statement.setString(index++, test.getPath());
                    statement.setString(index++, test.getName());
                    statement.setString(index, test.getSubtestName());
                    return this.readResult(statement.executeQuery(), this::readTestMetricName);
                }
        );

        return new HashSet<>(result);
    }

    private String readTestMetricName(ResultSet resultSet) throws SQLException {
        return resultSet.getString(1);
    }

    public List<TestMetricEntity> getMetricHistory(
            TestStatusEntity test,
            String metricName,
            StorageFrontHistoryApi.TestMetricHistoryFilters filters
    ) {
        var from = ProtoConverter.convert(filters.getFrom());
        var to = ProtoConverter.convert(filters.getTo());

        var query = """
                SELECT %s
                FROM %s
                WHERE branch = ? AND path = ? AND test_name = ? AND subtest_name = ?
                        AND toolchain = ? AND metric_name = ? AND date > ? AND date < ?
                """.formatted(ALL_COLUMNS, TABLE_NAME);

        return this.tx(connection -> {
                    var statement = connection.prepareStatement(query);
                    var index = 1;
                    statement.setString(index++, test.getId().getBranch());
                    statement.setString(index++, test.getPath());
                    statement.setString(index++, test.getName());
                    statement.setString(index++, test.getSubtestName());
                    statement.setString(index++, test.getId().getToolchain());
                    statement.setString(index++, metricName);
                    statement.setTimestamp(index++, convert(from));
                    statement.setTimestamp(index, convert(to));
                    return this.readResult(statement.executeQuery(), this::readTestMetric);
                }
        );
    }

    private Timestamp convert(Instant from) {
        return Timestamp.valueOf(LocalDateTime.ofInstant(from, ZoneId.systemDefault()));
    }

    private TestMetricEntity readTestMetric(ResultSet resultSet) {
        // tbd
        return TestMetricEntity.builder().build();
    }
}
