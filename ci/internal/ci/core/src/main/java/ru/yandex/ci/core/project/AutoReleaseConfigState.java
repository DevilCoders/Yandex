package ru.yandex.ci.core.project;


import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.core.config.a.model.auto.AutoReleaseConfig;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.lang.NonNullApi;

@Persisted
@Value
@Nonnull
@NonNullApi
public class AutoReleaseConfigState {
    public static final AutoReleaseConfigState EMPTY = new AutoReleaseConfigState(false);

    boolean enabled;

    public static AutoReleaseConfigState of(AutoReleaseConfig config) {
        return new AutoReleaseConfigState(
                config.isEnabled()
        );
    }

}
