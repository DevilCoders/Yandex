package ru.yandex.ci.storage.shard.cache;

import java.util.Map;
import java.util.Set;
import java.util.concurrent.CompletableFuture;

import ru.yandex.ci.client.ayamler.StrongModeRequest;
import ru.yandex.ci.storage.core.cache.StorageCustomCache;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultYamlInfo;

public interface StrongModeCache extends StorageCustomCache {
    CompletableFuture<Map<StrongModeRequest, TestResultYamlInfo>> getStrongMode(
            Set<StrongModeRequest> strongModeRequests
    );
}
