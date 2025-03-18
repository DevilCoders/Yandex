package ru.yandex.ci.storage.core.db.model.settings;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
public class ShardSettings {
    public static final ShardSettings EMPTY = ShardSettings.builder().skip(SkipSettings.DEFAULT).build();

    boolean stopAllRead;
    SkipSettings skip;

    @Persisted
    @Value
    @lombok.Builder
    public static class SkipSettings {
        public static final SkipSettings DEFAULT =
                SkipSettings.builder()
                        .aYamlerClientErrors(true)
                        .missing(true)
                        .parseError(true)
                        .validationError(true)
                        .build();

        boolean unknownMessage;
        boolean missing;
        boolean parseError;
        boolean validationError;
        boolean generalError;
        boolean aYamlerClientErrors;
    }
}

