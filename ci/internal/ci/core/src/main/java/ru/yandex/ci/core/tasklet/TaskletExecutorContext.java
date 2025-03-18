package ru.yandex.ci.core.tasklet;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;


/**
 * Данные, необходимые для запуска тасклета.
 * Содержит только информацию, получаемую непосредственно из конфигурационных файлов.
 */
@Persisted
@Value(staticConstructor = "of")
public class TaskletExecutorContext {
    @Nonnull
    TaskletMetadata.Id taskletKey;

    @Nonnull
    SchemaOptions schemaOptions;
}
