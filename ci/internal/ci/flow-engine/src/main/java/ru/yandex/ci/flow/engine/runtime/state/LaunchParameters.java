package ru.yandex.ci.flow.engine.runtime.state;

import java.util.Map;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.runtime.state.model.DelegatedOutputResources;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;

@Value
@Builder
public class LaunchParameters {

    @Nonnull
    LaunchId launchId;
    @Nonnull
    String projectId;
    @Nonnull
    LaunchFlowInfo flowInfo;
    @Nonnull
    LaunchVcsInfo vcsInfo;
    @Nonnull
    LaunchInfo launchInfo;
    @Nonnull
    Flow flow;
    @Singular
    Map<String, DelegatedOutputResources> jobResources;
    @Nullable
    String triggeredBy;
    @Nullable
    String title;

}
