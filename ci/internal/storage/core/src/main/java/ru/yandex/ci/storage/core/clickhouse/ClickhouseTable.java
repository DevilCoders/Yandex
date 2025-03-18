package ru.yandex.ci.storage.core.clickhouse;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import java.util.TimeZone;

import lombok.AllArgsConstructor;

import ru.yandex.ci.storage.core.db.clickhouse.utils.ExtendedRowBinaryStream;
import ru.yandex.clickhouse.BalancedClickhouseDataSource;
import ru.yandex.clickhouse.ClickHouseConnection;
import ru.yandex.clickhouse.domain.ClickHouseFormat;
import ru.yandex.clickhouse.settings.ClickHouseProperties;

@AllArgsConstructor
public abstract class ClickhouseTable<T> {
    private final BalancedClickhouseDataSource dataSource;

    public void save(List<T> entities) {
        var byteArrayOutputStream = new ByteArrayOutputStream();
        var stream = new ExtendedRowBinaryStream(
                byteArrayOutputStream,
                TimeZone.getDefault(),
                new ClickHouseProperties()
        );

        entities.forEach(entity -> writeChecked(entity, stream));

        try (ClickHouseConnection clickHouseConnection = dataSource.getConnection()) {
            clickHouseConnection.createStatement().write()
                    .sql(getInsertSql())
                    .table(getLocalTableName())
                    .format(ClickHouseFormat.RowBinary)
                    .data(new ByteArrayInputStream(byteArrayOutputStream.toByteArray()))
                    .send();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    public <R> R tx(TxCallback<R> callback) {
        try (var clickHouseConnection = dataSource.getConnection()) {
            return callback.apply(clickHouseConnection);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    protected <R> List<R> readResult(ResultSet resultSet, ResultReader<R> reader) throws SQLException {
        var result = new ArrayList<R>();
        while (resultSet.next()) {
            result.add(reader.read(resultSet));
        }
        return result;
    }

    protected abstract String getLocalTableName();

    protected abstract String getInsertSql();

    protected abstract void write(T entity, ExtendedRowBinaryStream stream) throws IOException;

    private void writeChecked(T entity, ExtendedRowBinaryStream stream) {
        try {
            write(entity, stream);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public interface ResultReader<T> {
        T read(ResultSet resultSet) throws SQLException;
    }

    public interface TxCallback<T> {
        T apply(ClickHouseConnection resultSet) throws SQLException;
    }
}
