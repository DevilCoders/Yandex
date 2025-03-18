package ru.yandex.ci.core.launch;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class LaunchFlowDescription {

    @Nonnull
    String title;

    @Nullable
    String description;

    @Nonnull
    Common.FlowType flowType;

    @Nullable
    LaunchId rollbackUsingLaunch;
}
