package ru.yandex.ci.storage.post_processor;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_launch.TestLaunchEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.FragmentationSettings;
import ru.yandex.ci.storage.core.db.model.test_revision.TestRevisionEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.post_processor.history.TestHistoryCacheImpl;
import ru.yandex.ci.storage.post_processor.processing.HistoryProcessor;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;

public class HistoryTest extends PostProcessorTestBase {
    private final FragmentationSettings fragmentationSettings = FragmentationSettings.create(1000, 1000);

    @Test
    public void loadsHistory() {
        var testId = new TestStatusEntity.Id(
                "trunk", new TestEntity.Id(1, "toolchain", 1)
        );

        var r1 = TestRevisionEntity.builder().id(new TestRevisionEntity.Id(testId, 8017110L)).build();
        var r2 = TestRevisionEntity.builder().id(new TestRevisionEntity.Id(testId, 9087111L)).build();
        var r3 = TestRevisionEntity.builder().id(new TestRevisionEntity.Id(testId, 9091110L)).build();

        this.db.currentOrTx(() -> this.db.testRevision().save(List.of(r1, r2, r3)));

        var cache = new TestHistoryCacheImpl(db, mock(PostProcessorStatistics.class), 1, clock, fragmentationSettings);
        var history = cache.get(testId);

        assertThat(history).isNotNull();
        assertThat(history.getNumberOfRegions()).isEqualTo(2);

        var region = history.getRegion(9).orElseThrow();
        assertThat(region.getNumberOfBuckets()).isEqualTo(2);

        var bucket = region.get(91).orElseThrow();

        assertThat(bucket.getNumberOfRevisions()).isEqualTo(1);
        assertThat(bucket.getRevision(9091110L)).isNotNull();

        cache = new TestHistoryCacheImpl(
                db, mock(PostProcessorStatistics.class), 1, clock,
                FragmentationSettings.create(32, 1000)
        );
        history = cache.get(testId);

        assertThat(history).isNotNull();
        assertThat(history.getNumberOfRegions()).isEqualTo(3);

        region = history.getRegion(250).orElseThrow();
        assertThat(region.getNumberOfBuckets()).isEqualTo(1);
        bucket = region.get(534).orElseThrow();
        assertThat(bucket.getNumberOfRevisions()).isEqualTo(1);
        assertThat(bucket.getRevision(8017110L)).isNotNull();

        region = history.getRegion(283).orElseThrow();
        assertThat(region.getNumberOfBuckets()).isEqualTo(1);
        bucket = region.get(972).orElseThrow();
        assertThat(bucket.getNumberOfRevisions()).isEqualTo(1);
        assertThat(bucket.getRevision(9087111L)).isNotNull();

        region = history.getRegion(284).orElseThrow();
        assertThat(region.getNumberOfBuckets()).isEqualTo(1);
        bucket = region.get(97).orElseThrow();
        assertThat(bucket.getNumberOfRevisions()).isEqualTo(1);
        assertThat(bucket.getRevision(9091110L)).isNotNull();
    }

    @Test
    public void buildsSearchTree() {
        var testId = new TestStatusEntity.Id(
                "trunk", new TestEntity.Id(1, "toolchain", 1)
        );

        var launches = createTestLaunches(testId);

        launches.set(
                3,
                launches.get(3).toBuilder()
                        .status(Common.TestStatus.TS_FAILED)
                        .build()
        );

        var revisions = launches.stream().map(x -> x.getId().getRevisionNumber()).toList();

        var historyCache = new TestHistoryCacheImpl(db, statistics, 1, clock, fragmentationSettings);
        var historyService = new HistoryProcessor(
                db, statistics, historyCache, timeTraceService, fragmentationSettings
        );
        historyService.process(launches);

        var history = historyCache.get(testId);

        for (var i = 1; i < revisions.size() - 1; i++) {
            var current = history.getRevision(revisions.get(i));
            assertThat(current).isPresent();
            assertThat(current.get().getPreviousRevision()).isEqualTo(revisions.get(i - 1));
            assertThat(current.get().getNextRevision()).isEqualTo(revisions.get(i + 1));
            assertThat(current.get().getStatus()).isEqualTo(launches.get(i).getStatus());
            assertThat(current.get().getPreviousStatus()).isEqualTo(launches.get(i - 1).getStatus());

            var left = history.getPrevious(new TestRevisionEntity.Id(testId, current.get().getId().getRevision()));
            var right = history.getNext(new TestRevisionEntity.Id(testId, current.get().getId().getRevision()));
            assertThat(left).isPresent();
            assertThat(right).isPresent();
            assertThat(left.get().getId().getRevision()).isEqualTo(revisions.get(i - 1));
            assertThat(right.get().getId().getRevision()).isEqualTo(revisions.get(i + 1));
        }

        var first = history.getRevision(revisions.get(0));
        assertThat(first).isPresent();
        assertThat(first.get().getPreviousRevision()).isEqualTo(0);
        assertThat(first.get().getNextRevision()).isEqualTo(revisions.get(1));

        var left = history.getPrevious(new TestRevisionEntity.Id(testId, revisions.get(0)));
        var right = history.getNext(new TestRevisionEntity.Id(testId, revisions.get(0)));
        assertThat(left).isEmpty();
        assertThat(right).isPresent();
        assertThat(right.get().getId().getRevision()).isEqualTo(revisions.get(1));

        var last = history.getRevision(revisions.get(5));
        assertThat(last).isPresent();
        assertThat(last.get().getPreviousRevision()).isEqualTo(revisions.get(4));
        assertThat(last.get().getNextRevision()).isEqualTo(0);

        left = history.getPrevious(new TestRevisionEntity.Id(testId, last.get().getId().getRevision()));
        right = history.getNext(new TestRevisionEntity.Id(testId, last.get().getId().getRevision()));
        assertThat(left).isPresent();
        assertThat(right).isEmpty();
        assertThat(left.get().getId().getRevision()).isEqualTo(revisions.get(4));
    }

    @Test
    public void marksAsChanged() {
        var testId = new TestStatusEntity.Id(
                "trunk", new TestEntity.Id(1, "toolchain", 1)
        );

        var launches = createTestLaunches(testId);

        launches.set(
                3,
                launches.get(3).toBuilder()
                        .status(Common.TestStatus.TS_FAILED)
                        .build()
        );

        var historyCache = new TestHistoryCacheImpl(db, statistics, 1, clock, fragmentationSettings);
        var historyService = new HistoryProcessor(db, statistics, historyCache, timeTraceService,
                fragmentationSettings);

        historyService.process(launches);
        // one more time
        historyService.process(launches);

        var history = historyCache.get(testId);

        assertThat(history).isNotNull();
        assertThat(history.getNumberOfRegions()).isEqualTo(3);

        var region = history.getRegion(2).orElseThrow();
        assertThat(region.getNumberOfBuckets()).isEqualTo(2);

        var bucket = region.get(2).orElseThrow();

        assertThat(bucket.getNumberOfRevisions()).isEqualTo(1);

        var revision = bucket.getRevision(launches.get(3).getId().getRevisionNumber());
        assertThat(revision).isPresent();

        assertThat(revision.get().isChanged()).isTrue();

        region = history.getRegion(3).orElseThrow();
        bucket = region.get(1).orElseThrow();

        revision = bucket.getRevision(launches.get(4).getId().getRevisionNumber());
        assertThat(revision).isPresent();
        assertThat(revision.get().isChanged()).isTrue();

        bucket = region.get(2).orElseThrow();
        revision = bucket.getRevision(launches.get(5).getId().getRevisionNumber());
        assertThat(revision).isPresent();
        assertThat(revision.get().isChanged()).isFalse();

        var importantRevisions = this.db.currentOrReadOnly(() -> this.db.testImportantRevision().findAll());
        assertThat(importantRevisions).hasSize(2);
        assertThat(importantRevisions.get(0).isChanged()).isTrue();
        assertThat(importantRevisions.get(1).isChanged()).isTrue();


        var insertRevision = launches.get(3).getId().getRevisionNumber() + 1;
        historyService.process(
                List.of(
                        launches.get(3).toBuilder()
                                .id(
                                        new TestLaunchEntity.Id(
                                                testId, insertRevision, launches.get(3).getId().getLaunch()
                                        )
                                )
                                .status(Common.TestStatus.TS_OK)
                                .build()
                )
        );

        revision = history.getRevision(launches.get(4).getId().getRevisionNumber());
        assertThat(revision).isPresent();
        assertThat(revision.get().isChanged()).isFalse();

        var importantId = revision.get().toImportant().getId();
        var important = this.db.currentOrReadOnly(() -> this.db.testImportantRevision().find(importantId));
        assertThat(important.orElseThrow().isChanged()).isFalse();
    }

    private ArrayList<TestLaunchEntity> createTestLaunches(TestStatusEntity.Id testId) {
        var launches = new ArrayList<TestLaunchEntity>();

        for (var region = 1L; region <= 3L; region++) {
            for (var bucket = 1L; bucket <= 2L; bucket++) {
                var revision = fragmentationSettings.getRevisionsInRegions() * region +
                        fragmentationSettings.getRevisionsInBucket() * bucket + 1;
                launches.add(
                        new TestLaunchEntity(
                                new TestLaunchEntity.Id(
                                        testId,
                                        revision,
                                        new TestLaunchEntity.LaunchRef(sampleIterationId, "taskId", 1, 0)
                                ),
                                Common.TestStatus.TS_OK,
                                "uid",
                                "",
                                Instant.now(),
                                Instant.now()
                        )
                );
            }
        }
        return launches;
    }
}
