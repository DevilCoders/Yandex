package ru.yandex.ci.storage.core.db.model.check;

import java.util.List;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class NativeBuild {
    @Nonnull
    String path;

    @Nonnull
    String toolchain;

    @Nonnull
    List<String> targets;
}
