package ru.yandex.ci.storage.core.db.model.chunk_aggregate;

import java.util.HashMap;
import java.util.Map;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class ChunkAggregateStatistics {
    public static final ChunkAggregateStatistics EMPTY = new ChunkAggregateStatistics(Map.of());

    @Column(flatten = false)
    Map<String, ToolchainStatistics> toolchains;

    public ChunkAggregateStatisticsMutable toMutable() {
        var map = new HashMap<String, ToolchainStatisticsMutable>();
        toolchains.forEach((key, value) -> map.put(key, value.toMutable()));
        return new ChunkAggregateStatisticsMutable(map);
    }

    public boolean isEmpty() {
        return this.equals(EMPTY);
    }

    public ToolchainStatistics getAll() {
        return toolchains.getOrDefault(TestEntity.ALL_TOOLCHAINS, ToolchainStatistics.EMPTY);
    }

    @Data
    @AllArgsConstructor
    public static class ChunkAggregateStatisticsMutable {
        Map<String, ToolchainStatisticsMutable> toolchains;

        public ChunkAggregateStatistics toImmutable() {
            var map = new HashMap<String, ToolchainStatistics>();
            toolchains.forEach((key, value) -> map.put(key, value.toImmutable()));
            return new ChunkAggregateStatistics(map);
        }

        public ToolchainStatisticsMutable getToolchain(String name) {
            var value = toolchains.get(name);
            if (value != null) {
                return value;
            }
            value = new ToolchainStatisticsMutable(
                    MainStatistics.EMPTY.toMutable(), ExtendedStatistics.EMPTY.toMutable()
            );

            toolchains.put(name, value);
            return value;
        }
    }

    @SuppressWarnings("ReferenceEquality")
    @Persisted
    @Value
    @With
    public static class ToolchainStatistics {
        public static final ToolchainStatistics EMPTY = new ToolchainStatistics(
                MainStatistics.EMPTY, ExtendedStatistics.EMPTY
        );

        @Column(flatten = false)
        MainStatistics main;

        @Column(flatten = false)
        ExtendedStatistics extended;

        public ToolchainStatisticsMutable toMutable() {
            return new ToolchainStatisticsMutable(main.toMutable(), extended.toMutable());
        }
    }

    @Value
    public static class ToolchainStatisticsMutable {
        MainStatistics.MainStatisticsMutable main;
        ExtendedStatistics.Mutable extended;

        public ToolchainStatistics toImmutable() {
            return new ToolchainStatistics(
                    main.toImmutable(), extended.toImmutable()
            );
        }

        public boolean hasNegativeNumbers() {
            return main.hasNegativeNumbers() || extended.hasNegativeNumbers();
        }
    }
}
