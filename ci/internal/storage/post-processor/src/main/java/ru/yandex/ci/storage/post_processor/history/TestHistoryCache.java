package ru.yandex.ci.storage.post_processor.history;

import ru.yandex.ci.storage.core.cache.StorageCustomCache;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

public interface TestHistoryCache extends StorageCustomCache {
    TestHistory get(TestStatusEntity.Id id);

    void invalidatePartition(int partition);
}
