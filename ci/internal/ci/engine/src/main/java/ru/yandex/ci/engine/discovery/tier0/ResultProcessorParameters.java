package ru.yandex.ci.engine.discovery.tier0;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class ResultProcessorParameters {
    int defaultFileExistsCacheSize;
    int defaultConfigBundleCacheSize;
    int defaultBatchSize;

    @Nonnull
    String sandboxResultResourceType;
}
