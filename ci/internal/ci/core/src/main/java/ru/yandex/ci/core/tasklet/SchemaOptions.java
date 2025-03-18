package ru.yandex.ci.core.tasklet;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

/**
 * Опции схемы тасклета, задаваемые вне самого тасклета.
 * Например, в конфиге задачи.
 */
@Persisted
@Value
@Builder
public class SchemaOptions {
    private static final SchemaOptions DEFAULT = SchemaOptions.builder().build();

    boolean singleInput;
    boolean singleOutput;

    public static SchemaOptions defaultOptions() {
        return DEFAULT;
    }
}
