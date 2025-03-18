package ru.yandex.ci.ayamler;

import java.time.Duration;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class AYamlerServiceProperties {

    @Nonnull
    Duration aYamlCacheExpireAfterAccess;
    long aYamlCacheMaximumSize;

    @Nonnull
    Duration aYamlOidCacheExpireAfterAccess;
    long aYamlOidCacheMaximumSize;

    @Nonnull
    Duration strongModeCacheExpireAfterAccess;
    long strongModeCacheMaximumSize;

    int cacheConcurrencyLevel;

}
