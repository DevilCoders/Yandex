package ru.yandex.ci.storage.core.db.model.check_task;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.Value;
import lombok.With;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Persisted
@Value
@With
public class PartitionsFinishStatistics {
    public static final PartitionsFinishStatistics EMPTY = new PartitionsFinishStatistics(Map.of());

    @Nullable
    Map<Integer, Set<Common.ChunkType>> partitions;

    public PartitionsFinishStatistics onChunkFinished(int partition, Set<Common.ChunkType> chunkTypes) {
        var updated = partitions == null ? new HashMap<Integer, Set<Common.ChunkType>>() : new HashMap<>(partitions);

        var statistics = new HashSet<>(updated.getOrDefault(partition, Set.of()));
        statistics.addAll(chunkTypes);
        updated.put(partition, statistics);

        return new PartitionsFinishStatistics(updated);
    }

    public boolean isChunkCompleted(int numberOfPartitions, Common.ChunkType chunkType) {
        if (partitions == null || partitions.size() < numberOfPartitions) {
            return false;
        }

        return partitions.values().stream().allMatch(x -> x.contains(chunkType));
    }
}
