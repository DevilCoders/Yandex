package ru.yandex.ci.storage.shard.cache;

import java.util.Map;
import java.util.Set;

import ru.yandex.ci.storage.core.cache.StorageCustomCache;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

public interface MuteStatusCache extends StorageCustomCache {
    void syncMuteActionsIfNeeded();

    Map<TestStatusEntity.Id, Boolean> get(Set<TestStatusEntity.Id> ids);

    boolean get(TestStatusEntity.Id id);
}
