package ru.yandex.ci.engine.launch.auto;

import java.time.Duration;
import java.util.Set;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.core.discovery.DiscoveryType;

@Value
@Builder
public class BinarySearchSettings {
    @Nonnull
    Duration minIntervalDuration;

    int minIntervalSize;

    @Nonnull
    Duration closeLaunchesOlderThan;

    int maxActiveLaunches;

    @Nonnull
    Set<DiscoveryType> discoveryTypes;

    boolean treatFailedLaunchAsComplete;
}
