package ru.yandex.ci.storage.shard.cache;

import java.util.Collection;
import java.util.Optional;
import java.util.Set;

import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.GroupingEntityCache;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;

public interface TestDiffsCache
        extends GroupingEntityCache<ChunkAggregateEntity.Id, TestDiffByHashEntity.Id, TestDiffByHashEntity> {

    Collection<TestDiffByHashEntity> getByParentId(TestDiffByHashEntity.Id id);

    TestDiffByHashEntity getOrCreate(TestDiffByHashEntity.Id id, TestResult taskResult);

    Optional<TestDiffByHashEntity> findInPreviousIterations(TestDiffByHashEntity.Id diffId, int iterationNumber);

    interface Modifiable extends TestDiffsCache,
            GroupingEntityCache.Modifiable<ChunkAggregateEntity.Id, TestDiffByHashEntity.Id, TestDiffByHashEntity> {
        void invalidate(Set<ChunkEntity.Id> chunks);
    }

    interface WithModificationSupport extends
            TestDiffsCache, ModificationSupport<TestDiffByHashEntity.Id, TestDiffByHashEntity> {
    }

    interface WithCommitSupport extends Modifiable,
            EntityCache.Modifiable.CacheWithCommitSupport<TestDiffByHashEntity.Id, TestDiffByHashEntity> {
    }
}
