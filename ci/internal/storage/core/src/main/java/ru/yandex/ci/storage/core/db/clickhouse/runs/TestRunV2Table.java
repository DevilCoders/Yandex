package ru.yandex.ci.storage.core.db.clickhouse.runs;

import java.io.IOException;
import java.sql.Date;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.time.LocalDate;
import java.util.List;

import ru.yandex.ci.storage.core.clickhouse.ClickhouseTable;
import ru.yandex.ci.storage.core.db.clickhouse.utils.ExtendedRowBinaryStream;
import ru.yandex.clickhouse.BalancedClickhouseDataSource;

public class TestRunV2Table extends ClickhouseTable<TestRunV2Entity> {
    private static final String TABLE_NAME = "runs_by_test_id_v2";
    private final String databaseName;

    public TestRunV2Table(String databaseName, BalancedClickhouseDataSource clickhouseDataSource) {
        super(clickhouseDataSource);
        this.databaseName = databaseName;
    }

    @Override
    protected String getLocalTableName() {
        return TABLE_NAME;
    }

    @Override
    protected String getInsertSql() {
        throw new UnsupportedOperationException();
    }

    @Override
    protected void write(TestRunV2Entity entity, ExtendedRowBinaryStream stream) throws IOException {
        throw new UnsupportedOperationException();
    }

    public List<TestRunV2Entity> getRuns(long oldId, LocalDate since, LocalDate until) {
        var query = """
                SELECT date, test_id, revision, branch_id, run_id
                FROM %s.%s
                WHERE date > ? and date < ? and test_id = ? and run_id is not null and branch_id = 2
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

    private TestRunV2Entity readRun(ResultSet rs) throws SQLException {
        return new TestRunV2Entity(
                rs.getDate("date").toLocalDate(),
                rs.getLong("test_id"),
                rs.getInt("revision"),
                rs.getInt("branch_id"),
                rs.getLong("run_id")
        );
    }
}
