package ru.yandex.ci.storage.reader.cache;

import ru.yandex.ci.storage.core.cache.CheckTaskStatisticsCache;
import ru.yandex.ci.storage.core.cache.SkippedChecksCache;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;

public interface ReaderCache extends StorageCoreCache<ReaderCache.Modifiable> {

    ChunkAggregatesGroupedCache chunkAggregatesGroupedByIteration();

    CheckTaskStatisticsCache taskStatistics();

    SkippedChecksCache skippedChecks();

    interface Modifiable extends StorageCoreCache.Modifiable {
        ChunkAggregatesGroupedCache.Modifiable chunkAggregatesGroupedByIteration();

        CheckTaskStatisticsCache.Modifiable taskStatistics();

        SkippedChecksCache.Modifiable skippedChecks();
    }
}
