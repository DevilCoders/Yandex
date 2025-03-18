package ru.yandex.ci.storage.post_processor.history;

import java.time.Clock;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.NavigableMap;
import java.util.Optional;
import java.util.concurrent.ConcurrentSkipListMap;
import java.util.stream.Collectors;

import ru.yandex.ci.storage.core.db.model.test_revision.TestRevisionEntity;
import ru.yandex.ci.storage.core.utils.MapUtils;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;

public class HistoryBucket {
    private final int number;
    private final NavigableMap<Long, TestRevisionEntity> revisions;
    private final PostProcessorStatistics statistics;
    private final Clock clock;
    private Instant lastRead;

    public HistoryBucket(
            int number, List<TestRevisionEntity> revisions, PostProcessorStatistics statistics, Clock clock
    ) {
        this.number = number;
        this.revisions = revisions.stream().collect(
                Collectors.toMap(
                        x -> x.getId().getRevision(), x -> x, MapUtils::forbidMergeIfNotEquals,
                        ConcurrentSkipListMap::new
                )
        );
        this.statistics = statistics;
        this.clock = clock;
        this.lastRead = clock.instant();
        this.statistics.onBucketCreated();
        this.statistics.onRevisionsAdded(this.revisions.size());
    }

    public int getNumber() {
        return number;
    }

    public int getNumberOfRevisions() {
        return this.revisions.size();
    }

    public Optional<TestRevisionEntity> getRevision(long revision) {
        this.lastRead = clock.instant();
        return Optional.ofNullable(revisions.get(revision));
    }

    public Instant getLastRead() {
        return lastRead;
    }

    public void putRevision(TestRevisionEntity revision) {
        if (!this.revisions.containsKey(revision.getId().getRevision())) {
            this.statistics.onRevisionsAdded(1);
        }
        this.revisions.put(revision.getId().getRevision(), revision);
    }

    public Optional<TestRevisionEntity> getPrevious(long revision) {
        return Optional.ofNullable(revisions.lowerEntry(revision)).map(Map.Entry::getValue);
    }

    public Optional<TestRevisionEntity> getNext(long revision) {
        return Optional.ofNullable(revisions.higherEntry(revision)).map(Map.Entry::getValue);
    }

    public Optional<TestRevisionEntity> getLast() {
        if (revisions.isEmpty()) {
            return Optional.empty();
        }

        return Optional.of(revisions.lastEntry().getValue());
    }

    public Optional<TestRevisionEntity> getFirst() {
        if (revisions.isEmpty()) {
            return Optional.empty();
        }

        return Optional.of(revisions.firstEntry().getValue());
    }

    public void onRemoved() {
        this.statistics.onBucketRemoved();
        this.statistics.onRevisionsRemoved(revisions.size());
    }
}
