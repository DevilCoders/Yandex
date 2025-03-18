package ru.yandex.ci.storage.core.db.model.check;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class LargeTestsConfig {
    @Nonnull
    String path;

    @Nonnull
    StorageRevision revision;
}
