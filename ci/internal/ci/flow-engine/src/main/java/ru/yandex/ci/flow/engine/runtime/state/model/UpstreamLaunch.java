package ru.yandex.ci.flow.engine.runtime.state.model;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class UpstreamLaunch {
    @Nonnull
    String upstreamJobId;

    int launchNumber;
}
