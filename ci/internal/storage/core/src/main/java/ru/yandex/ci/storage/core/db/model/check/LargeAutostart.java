package ru.yandex.ci.storage.core.db.model.check;

import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class LargeAutostart {
    @Nullable
    String path;

    @Nonnull
    String target;

    @Nonnull
    List<String> toolchains;
}
