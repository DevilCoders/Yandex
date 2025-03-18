package ru.yandex.ci.storage.post_processor.cache;

import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.cache.TestStatisticsCache;
import ru.yandex.ci.storage.core.cache.TestsCache;
import ru.yandex.ci.storage.post_processor.history.TestHistoryCache;

public interface PostProcessorCache extends StorageCoreCache<PostProcessorCache.Modifiable> {
    TestHistoryCache testHistory();

    TestsCache tests();

    TestStatisticsCache testStatistics();

    interface Modifiable extends StorageCoreCache.Modifiable {
        TestsCache.Modifiable tests();

        TestStatisticsCache.Modifiable testStatistics();
    }
}
