package ru.yandex.ci.storage.core.db.model.check_iteration;

import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.stream.Stream;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings("ReferenceEquality")
@Persisted
@Value
@With
public class IterationStatistics {
    public static final IterationStatistics EMPTY = new IterationStatistics(
            TechnicalStatistics.EMPTY, Metrics.EMPTY, Map.of()
    );

    @Column(flatten = false)
    TechnicalStatistics technical;

    @Column(flatten = false)
    Metrics metrics;

    @Column(flatten = false)
    Map<String, ToolchainStatistics> toolchains;

    public ToolchainStatistics getToolchain(String name) {
        var toolchain = toolchains.get(name);
        return toolchain == null ? ToolchainStatistics.EMPTY : toolchain;
    }

    public ToolchainStatistics getAllToolchain() {
        var toolchain = toolchains.get(TestEntity.ALL_TOOLCHAINS);
        return toolchain == null ? ToolchainStatistics.EMPTY : toolchain;
    }

    public Metrics getMetrics() {
        return Objects.requireNonNullElse(metrics, Metrics.EMPTY);
    }

    public IterationStatisticsMutable toMutable() {
        var map = new HashMap<String, ToolchainStatisticsMutable>(toolchains.size());
        toolchains.forEach((key, value) -> map.put(key, value.toMutable()));
        return new IterationStatisticsMutable(technical, metrics, map);
    }

    public Common.CheckStatus toCompletedStatus() {
        return getToolchain(TestEntity.ALL_TOOLCHAINS).toCompletedStatus();
    }

    public boolean hasUnrecoverableErrors() {
        return getToolchain(TestEntity.ALL_TOOLCHAINS).hasUnrecoverableErrors();
    }

    @Data
    @AllArgsConstructor
    public static class IterationStatisticsMutable {
        TechnicalStatistics technicalStatistics;
        Metrics metrics;
        Map<String, ToolchainStatisticsMutable> toolchains;

        public ToolchainStatisticsMutable getToolchain(String name) {
            var toolchain = toolchains.get(name);
            if (toolchain != null) {
                return toolchain;
            }

            toolchain = ToolchainStatistics.EMPTY.toMutable();
            toolchains.put(name, toolchain);
            return toolchain;
        }

        public IterationStatistics toImmutable() {
            var map = new HashMap<String, ToolchainStatistics>();
            toolchains.forEach((key, value) -> map.put(key, value.toImmutable()));
            return new IterationStatistics(
                    technicalStatistics, metrics, map
            );
        }
    }

    @Persisted
    @Value
    public static class ToolchainStatistics {
        public static final ToolchainStatistics EMPTY = new ToolchainStatistics(
                MainStatistics.EMPTY, ExtendedStatistics.EMPTY
        );

        @Column(flatten = false)
        MainStatistics main;

        @Column(flatten = false)
        ExtendedStatistics extended;

        public ToolchainStatisticsMutable toMutable() {
            return new ToolchainStatisticsMutable(
                    main.toMutable(), extended.toMutable()
            );
        }

        public Common.CheckStatus toCompletedStatus() {
            return main.getTotalOrEmpty().toCompletedStatus();
        }

        public boolean hasUnrecoverableErrors() {
            if (main.getTotalOrEmpty().getFailedInStrongMode() > 0) {
                return true;
            }

            return Stream.of(
                            main.getConfigureOrEmpty().toCompletedStatus(),
                            main.getBuildOrEmpty().toCompletedStatus(),
                            main.getStyleOrEmpty().toCompletedStatus()
                    )
                    .anyMatch(v -> v == Common.CheckStatus.COMPLETED_FAILED);
        }
    }

    @Value
    public static class ToolchainStatisticsMutable {
        MainStatistics.MainStatisticsMutable mainStatistics;
        ExtendedStatistics.Mutable extendedStatistics;

        public ToolchainStatistics toImmutable() {
            return new ToolchainStatistics(
                    mainStatistics.toImmutable(),
                    extendedStatistics.toImmutable()
            );
        }

        public static ToolchainStatisticsMutable newEmpty() {
            return ToolchainStatistics.EMPTY.toMutable();
        }
    }
}
