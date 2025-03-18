package ru.yandex.ci.storage.core.db.model.settings;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
public class PostProcessorSettings {
    public static final PostProcessorSettings EMPTY =
            PostProcessorSettings.builder().skip(SkipSettings.DEFAULT).build();

    boolean stopAllRead;
    SkipSettings skip;

    @Persisted
    @Value
    @lombok.Builder
    public static class SkipSettings {
        public static final SkipSettings DEFAULT =
                SkipSettings.builder()
                        .missing(true)
                        .parseError(true)
                        .validationError(true)
                        .build();

        boolean unknownMessage;
        boolean missing;
        boolean parseError;
        boolean validationError;
        boolean generalError;
    }
}

