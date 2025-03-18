package ru.yandex.ci.storage.shard.cache;

import ru.yandex.ci.storage.core.cache.CheckTextSearchCache;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;

public interface ShardCache extends StorageCoreCache<ShardCache.Modifiable> {
    ChunkAggregatesCache chunkAggregates();

    TestDiffsCache testDiffs();

    StrongModeCache strongModeCache();

    MuteStatusCache muteStatus();

    CheckTextSearchCache checkTextSearch();

    interface Modifiable extends StorageCoreCache.Modifiable {
        ChunkAggregatesCache.Modifiable chunkAggregates();

        TestDiffsCache.Modifiable testDiffs();

        MuteStatusCache muteStatus();

        CheckTextSearchCache checkTextSearch();
    }
}
