package ru.yandex.ci.storage.post_processor.history;

import java.time.Clock;
import java.time.Instant;
import java.util.List;
import java.util.NavigableMap;
import java.util.Optional;
import java.util.concurrent.ConcurrentSkipListMap;
import java.util.function.Function;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test_revision.BucketNumber;
import ru.yandex.ci.storage.core.db.model.test_revision.FragmentationSettings;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.utils.MapUtils;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;

@Slf4j
public class HistoryRegion {
    private final CiStorageDb db;
    private final TestStatusEntity.Id testId;
    private final int number;
    private final NavigableMap<Integer, Optional<HistoryBucket>> buckets;
    private final Instant lastRead;
    private final PostProcessorStatistics statistics;
    private final Clock clock;

    private final FragmentationSettings fragmentationSettings;

    public HistoryRegion(
            CiStorageDb db,
            TestStatusEntity.Id testId,
            int number,
            List<Integer> buckets,
            PostProcessorStatistics statistics,
            Clock clock,
            FragmentationSettings fragmentationSettings
    ) {
        this.db = db;
        this.testId = testId;
        this.number = number;
        this.buckets = buckets.stream().collect(
                Collectors.toMap(
                        Function.identity(), x -> Optional.empty(), MapUtils::forbidMergeIfNotEquals,
                        ConcurrentSkipListMap::new
                )
        );

        this.clock = clock;
        this.fragmentationSettings = fragmentationSettings;
        this.lastRead = clock.instant();
        this.statistics = statistics;
        statistics.onRegionCreated();
    }

    public Optional<HistoryBucket> get(int bucketNumber) {
        var bucketOptional = buckets.get(bucketNumber);
        //noinspection OptionalAssignedToNull
        if (bucketOptional == null) {
            this.statistics.onBucketEmptyLoad();
            return Optional.empty();
        }

        if (bucketOptional.isEmpty()) {
            var bucket = db.currentOrReadOnly(() -> loadBucket(bucketNumber, this.fragmentationSettings));
            buckets.put(bucketNumber, Optional.of(bucket));
            return Optional.of(bucket);
        }

        this.statistics.onBucketFromCacheLoad();
        return bucketOptional;
    }

    public HistoryBucket getOrElseThrow(int bucketNumber) {
        return get(bucketNumber).orElseThrow(() -> {
            throw new RuntimeException("Bucket not found: " + bucketNumber);
        });
    }

    private HistoryBucket loadBucket(int bucketNumber, FragmentationSettings settings) {
        var bucket = new BucketNumber(number, bucketNumber);
        var nextBucket = settings.nextBucket(bucket);
        log.info("Loading bucket {} for {}", bucket, testId);

        var revisions = this.db.testRevision().getRevisions(
                testId,
                settings.getBucketStartRevision(bucket),
                settings.getBucketStartRevision(nextBucket)
        );
        log.info("Loaded {} revisions from bucket {} for {}", revisions.size(), bucket, testId);
        var historyBucket = new HistoryBucket(bucketNumber, revisions, statistics, clock);
        this.statistics.onBucketFromDbLoad();
        return historyBucket;
    }

    public int getNumberOfBuckets() {
        return this.buckets.size();
    }

    public Instant getLastRead() {
        return lastRead;
    }

    public HistoryBucket createBucket(int bucketNumber) {
        log.info("Creating bucket {} in region {} for {}", bucketNumber, number, testId);
        var bucket = new HistoryBucket(bucketNumber, List.of(), statistics, clock);
        this.buckets.put(bucketNumber, Optional.of(bucket));
        return bucket;
    }

    public Optional<HistoryBucket> getPrevious(int number) {
        var previous = buckets.lowerEntry(number);
        if (previous == null) {
            return Optional.empty();
        }

        return get(previous.getKey());
    }

    public Optional<HistoryBucket> getNext(int number) {
        var next = buckets.higherEntry(number);
        if (next == null) {
            return Optional.empty();
        }

        return get(next.getKey());
    }

    public Optional<HistoryBucket> getFirstBucket() {
        if (buckets.isEmpty()) {
            return Optional.empty();
        }

        return buckets.firstEntry().getValue();
    }

    public Optional<HistoryBucket> getLastBucket() {
        if (buckets.isEmpty()) {
            return Optional.empty();
        }

        return buckets.lastEntry().getValue();
    }

    public void onRemoved() {
        statistics.onRegionRemoved();
        buckets.values().stream().filter(Optional::isPresent).map(Optional::get).forEach(HistoryBucket::onRemoved);
    }
}
