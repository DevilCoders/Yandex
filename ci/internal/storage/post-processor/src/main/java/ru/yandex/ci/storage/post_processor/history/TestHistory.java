package ru.yandex.ci.storage.post_processor.history;

import java.time.Clock;
import java.util.List;
import java.util.NavigableMap;
import java.util.Optional;
import java.util.concurrent.ConcurrentSkipListMap;
import java.util.function.Function;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test_revision.FragmentationSettings;
import ru.yandex.ci.storage.core.db.model.test_revision.TestRevisionEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;

@Slf4j
public class TestHistory {
    private final CiStorageDb db;
    private final TestStatusEntity.Id testId;
    private final NavigableMap<Integer, Optional<HistoryRegion>> regions;
    private final PostProcessorStatistics statistics;

    private final FragmentationSettings fragmentationSettings;
    private final Clock clock;

    public TestHistory(
            CiStorageDb db,
            TestStatusEntity.Id testId,
            PostProcessorStatistics statistics,
            FragmentationSettings fragmentationSettings,
            Clock clock
    ) {
        this.db = db;
        this.testId = testId;
        this.regions = new ConcurrentSkipListMap<>();
        this.statistics = statistics;
        this.fragmentationSettings = fragmentationSettings;
        this.clock = clock;
    }

    public void initialize() {
        this.db.currentOrReadOnly(this::initializeInTx);
    }

    private void initializeInTx() {
        var numbers = this.db.testRevision().getRegions(testId, this.fragmentationSettings);
        regions.putAll(
                numbers.stream().collect(
                        Collectors.toMap(Function.identity(), x -> Optional.empty())
                )
        );
    }

    public int getNumberOfRegions() {
        return regions.size();
    }

    public Optional<HistoryRegion> getRegion(int number) {
        var regionOptional = regions.get(number);
        //noinspection OptionalAssignedToNull
        if (regionOptional == null) {
            return Optional.empty();
        }

        if (regionOptional.isEmpty()) {
            var region = db.currentOrReadOnly(() -> loadRegion(number));
            regions.put(number, Optional.of(region));
            return Optional.of(region);
        }

        return regionOptional;
    }

    public HistoryRegion createRegion(int number) {
        log.info("Creating region {} for {}", number, testId);

        var region = new HistoryRegion(db, testId, number, List.of(), statistics, clock, this.fragmentationSettings);
        regions.put(number, Optional.of(region));
        return region;
    }

    private HistoryRegion loadRegion(int number) {
        log.info("Loading region {} for {}", number, testId);
        this.statistics.onRegionLoad();

        var buckets = this.db.testRevision().getBuckets(testId, number, this.fragmentationSettings);
        return new HistoryRegion(this.db, this.testId, number, buckets, statistics, clock, this.fragmentationSettings);
    }

    public Optional<TestRevisionEntity> getRevision(long revisionNumber) {
        var bucketNumber = this.fragmentationSettings.getBucket(revisionNumber);
        var region = this.getRegion(bucketNumber.getRegion());
        if (region.isEmpty()) {
            return Optional.empty();
        }

        var bucket = region.get().get(bucketNumber.getBucket());

        if (bucket.isEmpty()) {
            return Optional.empty();
        }

        return bucket.get().getRevision(revisionNumber);
    }

    public void put(TestRevisionEntity revision) {
        var bucketNumber = this.fragmentationSettings.getBucket(revision.getId().getRevision());
        var region = this.getRegionOrElseThrow(bucketNumber.getRegion());
        region.getOrElseThrow(bucketNumber.getBucket()).putRevision(revision);
    }


    public HistoryRegion getRegionOrElseThrow(int number) {
        var region = this.getRegion(number);
        return region.orElseThrow(() -> {
            throw new RuntimeException("Region not found: " + number);
        });
    }

    public Optional<TestRevisionEntity> getPrevious(TestRevisionEntity.Id revision) {
        var bucketNumber = this.fragmentationSettings.getBucket(revision.getRevision());

        var currentRegionOptional = this.getRegion(bucketNumber.getRegion());
        if (currentRegionOptional.isPresent()) {
            var currentRegion = currentRegionOptional.get();
            var currentBucket = currentRegion.get(bucketNumber.getBucket());

            if (currentBucket.isPresent()) {
                var previousRevision = currentBucket.get().getPrevious(revision.getRevision());
                if (previousRevision.isPresent()) {
                    return previousRevision;
                }
            }

            var previousBucket = currentRegion.getPrevious(bucketNumber.getBucket());
            if (previousBucket.isPresent()) {
                return previousBucket.get().getLast();
            }
        }

        var lowerRegion = regions.lowerEntry(bucketNumber.getRegion());
        if (lowerRegion == null) {
            return Optional.empty();
        }

        var previousRegion = getRegionOrElseThrow(lowerRegion.getKey());
        var previousBucket = previousRegion.getLastBucket();
        if (previousBucket.isEmpty()) {
            return Optional.empty();
        }

        return previousBucket.get().getLast();
    }

    public Optional<TestRevisionEntity> getNext(TestRevisionEntity.Id revision) {
        var bucketNumber = this.fragmentationSettings.getBucket(revision.getRevision());

        var currentRegionOptional = this.getRegion(bucketNumber.getRegion());
        if (currentRegionOptional.isPresent()) {
            var currentRegion = currentRegionOptional.get();
            var currentBucket = currentRegion.get(bucketNumber.getBucket());

            if (currentBucket.isPresent()) {
                var nextRevision = currentBucket.get().getNext(revision.getRevision());
                if (nextRevision.isPresent()) {
                    return nextRevision;
                }
            }

            var nextBucket = currentRegion.getNext(bucketNumber.getBucket());
            if (nextBucket.isPresent()) {
                return nextBucket.get().getFirst();
            }
        }

        var higherRegion = regions.higherEntry(bucketNumber.getRegion());
        if (higherRegion == null) {
            return Optional.empty();
        }

        var nextRegion = getRegionOrElseThrow(higherRegion.getKey());
        var nextBucket = nextRegion.getFirstBucket();
        if (nextBucket.isEmpty()) {
            return Optional.empty();
        }

        return nextBucket.get().getFirst();
    }

    public void onRemoved() {
        regions.values().stream().filter(Optional::isPresent).map(Optional::get).forEach(HistoryRegion::onRemoved);
    }
}
