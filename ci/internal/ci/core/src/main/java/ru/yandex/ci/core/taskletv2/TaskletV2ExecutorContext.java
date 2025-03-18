package ru.yandex.ci.core.taskletv2;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.core.tasklet.SchemaOptions;
import ru.yandex.ci.ydb.Persisted;


/**
 * Данные, необходимые для запуска тасклета.
 * Содержит только информацию, получаемую непосредственно из конфигурационных файлов.
 */
@Persisted
@Value(staticConstructor = "of")
public class TaskletV2ExecutorContext {
    @Nonnull
    TaskletV2Metadata.Description taskletDescription;

    @Nonnull
    TaskletV2Metadata.Id taskletKey;

    @Nonnull
    SchemaOptions schemaOptions;
}
