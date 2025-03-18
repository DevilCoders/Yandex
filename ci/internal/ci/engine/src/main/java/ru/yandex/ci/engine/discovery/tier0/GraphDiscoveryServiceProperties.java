package ru.yandex.ci.engine.discovery.tier0;

import java.time.Duration;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class GraphDiscoveryServiceProperties {

    boolean enabled;

    @Nonnull
    String sandboxTaskType;

    @Nonnull
    String sandboxTaskOwner;

    boolean useDistbuildTestingCluster;

    @Nonnull
    // https://st.yandex-team.ru/CI-1690#6044ba734de7c5566938af68
    String secretWithYaToolToken;

    @Nonnull
    String distBuildPool;

    @Nonnull
    Duration delayBetweenSandboxTaskRestarts;

}
