package ru.yandex.ci.storage.core.db.clickhouse.utils;

import java.io.IOException;
import java.io.OutputStream;
import java.util.List;
import java.util.TimeZone;

import javax.annotation.Nullable;

import ru.yandex.clickhouse.settings.ClickHouseProperties;
import ru.yandex.clickhouse.util.ClickHouseRowBinaryStream;

public class ExtendedRowBinaryStream extends ClickHouseRowBinaryStream {
    public ExtendedRowBinaryStream(OutputStream outputStream, TimeZone timeZone, ClickHouseProperties properties) {
        super(outputStream, timeZone, properties);
    }

    public void writeNullableFloat64(@Nullable Double value) throws IOException {
        if (value == null) {
            markNextNullable(true);
        } else {
            markNextNullable(false);
            writeFloat64(value);
        }
    }

    public void writeNullableString(@Nullable String value) throws IOException {
        if (value == null) {
            markNextNullable(true);
        } else {
            markNextNullable(false);
            writeString(value);
        }
    }

    public void writeBool(boolean value) throws IOException {
        writeUInt8(value ? 1 : 0);
    }

    public void writeNullableInt64(@Nullable Long value) throws IOException {
        if (value == null) {
            markNextNullable(true);
        } else {
            markNextNullable(false);
            writeInt64(value);
        }
    }

    public void writeStringArray(List<String> value) throws IOException {
        writeStringArray(value.toArray(new String[0]));
    }

    public void writeFloat64Array(List<Double> value) throws IOException {
        writeFloat64Array(value.stream().mapToDouble(x -> x).toArray());
    }

    public void writeUInt8Array(List<Integer> value) throws IOException {
        writeUInt8Array(value.stream().mapToInt(x -> x).toArray());
    }

    public void writeNullableUInt8(@Nullable Integer value) throws IOException {
        if (value == null) {
            markNextNullable(true);
        } else {
            markNextNullable(false);
            writeUInt8(value);
        }
    }
}
