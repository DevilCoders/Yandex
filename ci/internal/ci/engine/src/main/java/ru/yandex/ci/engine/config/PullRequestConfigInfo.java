package ru.yandex.ci.engine.config;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.lang.NonNullApi;

@NonNullApi
@Value(staticConstructor = "of")
public class PullRequestConfigInfo {
    @Nonnull
    ConfigBundle config;
    @Nullable
    ConfigBundle upstreamConfig;
}
