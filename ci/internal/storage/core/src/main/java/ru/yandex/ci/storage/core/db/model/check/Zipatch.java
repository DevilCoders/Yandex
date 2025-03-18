package ru.yandex.ci.storage.core.db.model.check;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class Zipatch {

    @Nonnull
    String url;
    long baseRevision;
}
