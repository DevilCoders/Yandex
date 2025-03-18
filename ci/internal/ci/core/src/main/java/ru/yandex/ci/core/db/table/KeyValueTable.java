package ru.yandex.ci.core.db.table;

import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;
import java.util.OptionalInt;
import java.util.OptionalLong;
import java.util.function.Supplier;

import javax.annotation.Nullable;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.db.model.KeyValue;
import ru.yandex.ci.util.gson.LocalDateAdapter;
import ru.yandex.ci.util.gson.LocalDateTimeAdapter;

public class KeyValueTable extends KikimrTableCi<KeyValue> {
    private static final String DEFAULT_NAMESPACE = "default";
    private static final Gson GSON = new GsonBuilder()
            .registerTypeAdapter(LocalDateTime.class, new LocalDateTimeAdapter())
            .registerTypeAdapter(LocalDate.class, new LocalDateAdapter())
            .create();

    public KeyValueTable(QueryExecutor executor) {
        super(KeyValue.class, executor);
    }

    public <T> Optional<T> findObject(String key, Class<T> clazz) {
        return findObject(DEFAULT_NAMESPACE, key, clazz);
    }

    public <T> Optional<T> findObject(String namespace, String key, Class<T> clazz) {
        return findString(namespace, key).map(string -> GSON.fromJson(string, clazz));
    }

    public void setValue(String namespace, String key, Object value) {
        setValue(namespace, key, GSON.toJson(value));
    }

    public Optional<String> findValue(String namespace, String key) {
        return find(KeyValue.Id.of(namespace, key))
                .map(KeyValue::getValue);
    }

    public List<KeyValue> findValues(String namespace) {
        return find(List.of(
                YqlPredicate.where("id.namespace").eq(namespace)
        ));
    }

    public String getValue(String namespace, String key) {
        return get(KeyValue.Id.of(namespace, key)).getValue();
    }

    @Nullable
    public String getString(String key) {
        return getString(DEFAULT_NAMESPACE, key);
    }

    @Nullable
    public String getString(String namespace, String key) {
        return getValue(namespace, key);
    }

    public String getString(String namespace, String key, String defaultValue) {
        return findValue(namespace, key).orElse(defaultValue);
    }

    public Optional<String> findString(String namespace, String key) {
        return findValue(namespace, key);
    }

    public int getInt(String key) {
        return getInt(DEFAULT_NAMESPACE, key);
    }

    public int getInt(String namespace, String key) {
        return Integer.parseInt(getValue(namespace, key));
    }

    public OptionalInt findInt(String namespace, String key) {
        return findValue(namespace, key).stream()
                .mapToInt(Integer::parseInt)
                .findFirst();
    }

    public int getInt(String namespace, String key, int defaultValue) {
        return findValue(namespace, key)
                .map(Integer::parseInt)
                .orElse(defaultValue);
    }

    public long getLong(String key) {
        return getLong(DEFAULT_NAMESPACE, key);
    }

    public long getLong(String namespace, String key) {
        return Long.parseLong(getValue(namespace, key));
    }

    public OptionalLong findLong(String namespace, String key) {
        return findValue(namespace, key).stream()
                .mapToLong(Long::parseLong)
                .findFirst();
    }

    public long getLong(String namespace, String key, long defaultValue) {
        return findValue(namespace, key)
                .map(Long::parseLong)
                .orElse(defaultValue);
    }

    public boolean getBoolean(String key) {
        return getBoolean(DEFAULT_NAMESPACE, key);
    }

    public boolean getBoolean(String namespace, String key) {
        return Boolean.parseBoolean(getValue(namespace, key));
    }

    public Optional<Boolean> findBoolean(String namespace, String key) {
        return findValue(namespace, key).map(Boolean::parseBoolean);
    }

    public boolean getBoolean(String namespace, String key, boolean defaultValue) {
        return getBoolean(namespace, key, () -> defaultValue);
    }

    public boolean getBoolean(String namespace, String key, Supplier<Boolean> defaultValueSupplier) {
        return findBoolean(namespace, key).orElseGet(defaultValueSupplier);
    }

    public Instant getInstant(String key) {
        return getInstant(DEFAULT_NAMESPACE, key);
    }

    public Instant getInstant(String namespace, String key) {
        return Instant.ofEpochMilli(getLong(namespace, key));
    }

    public Instant getInstant(String namespace, String key, Instant defaultValue) {
        return Instant.ofEpochMilli(getLong(namespace, key, defaultValue.toEpochMilli()));
    }

    public void setValue(String key, String value) {
        setValue(DEFAULT_NAMESPACE, key, value);
    }

    public void setValue(String namespace, String key, String value) {
        save(new KeyValue(
                KeyValue.Id.of(namespace, key), value
        ));
    }

    public void setValue(String key, long value) {
        setValue(DEFAULT_NAMESPACE, key, value);
    }

    public void setValue(String namespace, String key, long value) {
        setValue(namespace, key, Long.toString(value));
    }

    public void setValue(String key, boolean value) {
        setValue(DEFAULT_NAMESPACE, key, value);
    }

    public void setValue(String namespace, String key, boolean value) {
        setValue(namespace, key, Boolean.toString(value));
    }

    public void setValue(String key, int value) {
        setValue(DEFAULT_NAMESPACE, key, value);
    }

    public void setValue(String namespace, String key, int value) {
        setValue(namespace, key, Integer.toString(value));
    }

    public void setValue(String key, Instant value) {
        setValue(DEFAULT_NAMESPACE, key, value.toEpochMilli());
    }

    public void setValue(String key, Object value) {
        setValue(DEFAULT_NAMESPACE, key, value);
    }

    public void setValue(String namespace, String key, Instant value) {
        setValue(namespace, key, value.toEpochMilli());
    }
}
