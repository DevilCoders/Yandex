package ru.yandex.ci.storage.post_processor.history;

import java.time.Clock;
import java.util.Map;
import java.util.concurrent.ExecutionException;
import java.util.stream.Collectors;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.FragmentationSettings;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;

@Slf4j
public class TestHistoryCacheImpl implements TestHistoryCache {

    private final CiStorageDb db;
    private final Cache<TestStatusEntity.Id, TestHistory> cache;
    private final PostProcessorStatistics statistics;
    private final int numberOfPartitions;
    private final Clock clock;

    private final FragmentationSettings fragmentationSettings;

    public TestHistoryCacheImpl(
            CiStorageDb db,
            PostProcessorStatistics statistics,
            int numberOfPartitions,
            Clock clock,
            FragmentationSettings fragmentationSettings
    ) {
        this.db = db;
        this.statistics = statistics;
        this.numberOfPartitions = numberOfPartitions;
        this.clock = clock;
        this.fragmentationSettings = fragmentationSettings;
        this.cache = CacheBuilder.newBuilder()
                .recordStats()
                .build();

        statistics.monitor(cache, "history");
    }

    @Override
    public TestHistory get(TestStatusEntity.Id id) {
        try {
            return cache.get(id, () -> load(id));
        } catch (ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    private TestHistory load(TestStatusEntity.Id id) {
        var history = new TestHistory(db, id, this.statistics, fragmentationSettings, clock);
        history.initialize();

        return history;
    }

    @Override
    public void invalidateAll() {
        this.cache.invalidateAll();
    }

    @Override
    public void invalidatePartition(int partition) {
        log.info("Invalidating partition {}", partition);
        var tests = cache.asMap().entrySet().stream()
                .filter(
                        x -> TestEntity.Id.getPostProcessorPartition(
                                x.getKey().getTestId(), this.numberOfPartitions
                        ) == partition
                ).toList();

        log.info("Invalidating {} tests from partition {}", tests, partition);
        this.cache.invalidateAll(tests.stream().map(Map.Entry::getKey).collect(Collectors.toSet()));
        tests.stream().map(Map.Entry::getValue).forEach(TestHistory::onRemoved);
    }
}
