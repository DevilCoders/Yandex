package ru.yandex.ci.storage.core.sharding;

import java.util.List;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.primitives.UnsignedLongs;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.utils.CiHashCode;

@SuppressWarnings("UnstableApiUsage")
@RequiredArgsConstructor
public class ChunkDistributor {

    @Nonnull
    private final CiStorageDb db;

    @Nonnull
    private final ShardingSettings shardingSettings;

    private final int numberOfPartitions;

    public void initializeOrCheckState() {
        var chunks = db.currentOrReadOnly(() -> db.chunks().readAll());

        if (chunks.isEmpty()) {
            db.currentOrTx(
                    () -> db.chunks().createInitialDistribution(numberOfPartitions, shardingSettings)
            );
        } else {
            var chunksByType = chunks.stream().collect(Collectors.groupingBy(x -> x.getId().getChunkType()));
            for (var type : Common.ChunkType.values()) {
                if (type == Common.ChunkType.UNRECOGNIZED) {
                    continue;
                }
                if (chunksByType.getOrDefault(type, List.of()).size() < shardingSettings.getChunksCount(type)) {
                    throw new RuntimeException("Number of chunks for %s is less then in chunks table".formatted(type));
                }
            }
        }

    }

    @Nullable
    public ChunkEntity.Id getId(
            Long hid, Common.ChunkType chunkType, long chunkShift, ShardingSettings shardingSettings
    ) {
        var chunksCount = shardingSettings.getChunksCount(chunkType);
        var chunkNumber = getChunkNumber(hid, chunksCount, chunkShift);
        var samplingPercent = shardingSettings.getChunkSamplingPercent();

        if (samplingPercent > 0 && chunkNumber * 100 / chunksCount > samplingPercent) {
            return null;
        }

        return ChunkEntity.Id.of(chunkType, chunkNumber);
    }

    @VisibleForTesting
    static int getChunkNumber(Long hid, int chunksCount, long chunkShift) {
        if (chunkShift == 0) {
            return (int) UnsignedLongs.remainder(hid, chunksCount) + 1;
        }

        var shiftedHid = 31L * hid + chunkShift;
        return (int) UnsignedLongs.remainder(shiftedHid, chunksCount) + 1;
    }

    public static int hashCode(String value) {
        return CiHashCode.hashCode(value);
    }
}
