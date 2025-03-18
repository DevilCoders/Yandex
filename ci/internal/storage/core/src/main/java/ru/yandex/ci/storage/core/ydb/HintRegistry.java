package ru.yandex.ci.storage.core.ydb;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import com.yandex.ydb.table.settings.AutoPartitioningPolicy;
import com.yandex.ydb.table.settings.PartitioningPolicy;
import com.yandex.ydb.table.settings.PartitioningSettings;
import com.yandex.ydb.table.values.PrimitiveValue;
import com.yandex.ydb.table.values.TupleValue;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.client.YdbTableHint;

import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

@Slf4j
public class HintRegistry {

    private static class Holder {
        private static final HintRegistry INSTANCE = new HintRegistry();
    }

    private final Map<Class<? extends Entity<?>>, YdbTableHintBuilder> hints = new ConcurrentHashMap<>();

    public <T extends Entity<T>> void hint(Class<T> entity, int numberOfPartitions, YdbTableHintBuilder hintBuilder) {
        log.info("Registering hint for {}", entity);
        if (hints.put(entity, hintBuilder) != null) {
            throw new IllegalArgumentException("Attempt to registry hint for " + entity + " more than once");
        }
        hintBuilder.apply(numberOfPartitions);
    }

    public Collection<YdbTableHintBuilder> getHints() {
        return hints.values();
    }

    //

    public static YdbTableHint uniform(int numberOfPartitions, PartitioningSettings partitioningSettings) {
        return new YdbTableHint(
                new PartitioningPolicy()
                        .setUniformPartitions(numberOfPartitions),
                partitioningSettings,
                YdbTableHint.TablePreset.LOG_LZ4
        );
    }

    public static YdbTableHint uniformAndSplitMerge(
            int numberOfPartitions, PartitioningSettings partitioningSettings
    ) {
        return uniformAndSplitMerge(numberOfPartitions, YdbTableHint.TablePreset.LOG_LZ4, partitioningSettings);
    }

    public static YdbTableHint uniformAndSplitMerge(
            int numberOfPartitions, YdbTableHint.TablePreset preset, PartitioningSettings partitioningSettings
    ) {
        return new YdbTableHint(
                new PartitioningPolicy()
                        .setUniformPartitions(numberOfPartitions)
                        .setAutoPartitioning(AutoPartitioningPolicy.AUTO_SPLIT_MERGE),
                partitioningSettings,
                preset
        );
    }

    public static YdbTableHint byCheckEntityPoints(int numberOfPartitions, PartitioningSettings partitioningSettings) {
        if (numberOfPartitions == 1) {
            return uniformAndSplitMerge(numberOfPartitions, partitioningSettings);
        }

        var points = new ArrayList<TupleValue>(CheckEntity.NUMBER_OF_ID_PARTITIONS);
        for (var i = 1; i <= CheckEntity.NUMBER_OF_ID_PARTITIONS; i++) {
            points.add(TupleValue.of(PrimitiveValue.int64(i * CheckEntity.ID_START).makeOptional()));
        }

        return new YdbTableHint(
                new PartitioningPolicy()
                        .setAutoPartitioning(AutoPartitioningPolicy.AUTO_SPLIT_MERGE)
                        .setExplicitPartitioningPoints(points),
                partitioningSettings,
                YdbTableHint.TablePreset.LOG_LZ4
        );
    }

    public static HintRegistry getInstance() {
        return Holder.INSTANCE;
    }

    public interface YdbTableHintBuilder {
        void apply(int numberOfPartitions);
    }

}
