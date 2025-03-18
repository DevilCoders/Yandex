package ru.yandex.ci.storage.core.db.model.check_task_statistics;

import java.time.Instant;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder(toBuilder = true)
public class CheckTaskStatistics {
    public static final CheckTaskStatistics EMPTY = new CheckTaskStatistics(
            null, null, Map.of(), AffectedChunks.EMPTY, Set.of(), Set.of()
    );

    @Nullable
    Instant firstWrite;

    @Nullable
    Instant lastWrite;

    // values can be inaccurate
    Map<String, Integer> numberOfMessagesByType;

    AffectedChunks affectedChunks;

    @Nullable // old values
    Set<Common.ChunkType> finishedChunkTypes;

    @Nullable
    Set<String> affectedToolchains;

    public Set<Common.ChunkType> getFinishedChunkTypes() {
        return finishedChunkTypes == null ? Set.of() : finishedChunkTypes;
    }

    public Set<String> getAffectedToolchains() {
        return affectedToolchains == null ? Set.of() : affectedToolchains;
    }

    @Persisted
    @Value
    public static class AffectedChunks {
        private static final AffectedChunks EMPTY = new AffectedChunks(Map.of());

        Map<Common.ChunkType, Set<Integer>> chunks;

        public AffectedChunksMutable toMutable() {
            return new AffectedChunksMutable(new HashMap<>(chunks));
        }
    }

    @Data
    @AllArgsConstructor
    public static class AffectedChunksMutable {
        Map<Common.ChunkType, Set<Integer>> chunks;

        public AffectedChunks toImmutable() {
            return new AffectedChunks(Map.copyOf(chunks));
        }
    }
}
