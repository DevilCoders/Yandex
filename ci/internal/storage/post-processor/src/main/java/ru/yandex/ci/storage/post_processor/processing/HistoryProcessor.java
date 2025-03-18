package ru.yandex.ci.storage.post_processor.processing;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test_launch.TestLaunchEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.FragmentationSettings;
import ru.yandex.ci.storage.core.db.model.test_revision.TestImportantRevisionEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.TestRevisionEntity;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;
import ru.yandex.ci.storage.post_processor.history.TestHistoryCache;
import ru.yandex.ci.util.Retryable;

@Slf4j
@AllArgsConstructor
public class HistoryProcessor {
    private static final int BULK_LIMIT = 8192;

    private final CiStorageDb db;
    private final PostProcessorStatistics statistics;
    private final TestHistoryCache cache;
    private final TimeTraceService timeTraceService;
    private final FragmentationSettings fragmentationSettings;

    public void process(List<TestLaunchEntity> records) {
        var trace = timeTraceService.createTrace("history_processor");

        log.info("Warming cache for {} records", records.size());

        var updatedRecords = new ArrayList<TestLaunchEntity>();
        this.db.currentOrReadOnly(
                () -> records.forEach(
                        history -> {
                            var revision = cache.get(history.getId().getStatusId())
                                    .getRevision(history.getId().getRevisionNumber());
                            if (revision.isEmpty() || revision.get().getStatus() != history.getStatus()) {
                                updatedRecords.add(history);
                            }
                        }
                )
        );

        statistics.onReceivedRevisions(
                records.stream().map(x -> x.getId().getRevisionNumber()).collect(Collectors.toSet())
        );

        trace.step("cache_warm");

        log.info("Updated records: {} / {}", updatedRecords.size(), records.size());
        this.statistics.onHistoryUnchangedProcessed(records.size() - updatedRecords.size());

        log.info("Bulk inserting {} test history", records.size());

        this.db.currentOrTx(
                () -> this.db.testLaunches().bulkUpsertWithRetries(
                        records, BULK_LIMIT, (e) -> this.statistics.onBulkInsertError()
                )
        );

        trace.step("runs_insert");

        Retryable.retryUntilInterruptedOrSucceeded(
                () -> processChanged(updatedRecords, trace), (e) -> this.statistics.onHistoryError(), false
        );
    }

    private void processChanged(List<TestLaunchEntity> records, TimeTraceService.Trace trace) {
        log.info("Processing {} changed records", records.size());

        var modified = new HashMap<TestRevisionEntity.Id, TestRevisionEntity>();
        var importantModified = new HashMap<TestRevisionEntity.Id, TestImportantRevisionEntity>();

        // We can not allow any exceptions to be throws because we are modifying cache without saving to db
        records.forEach(record -> Retryable.retryUntilInterruptedOrSucceeded(
                () -> processChanged(record, modified, importantModified),
                e -> statistics.onBulkInsertError()
        ));

        trace.step("changed_processed");

        saveModified(modified);
        saveImportantModified(importantModified);

        trace.step("revisions_db_insert");

        this.statistics.onHistoryChangedProcessed(records.size());

        log.info("Processing completed");
    }

    private void saveImportantModified(Map<TestRevisionEntity.Id, TestImportantRevisionEntity> values) {
        if (values.isEmpty()) {
            return;
        }

        log.info("Saving {} important modified revisions", values.size());
        this.db.currentOrTx(
                () -> this.db.testImportantRevision().bulkUpsertWithRetries(
                        new ArrayList<>(values.values()), 10000, e -> statistics.onBulkInsertError()
                )
        );
    }

    private void saveModified(HashMap<TestRevisionEntity.Id, TestRevisionEntity> modified) {
        if (modified.isEmpty()) {
            return;
        }

        log.info("Saving {} modified revisions", modified.size());
        this.db.currentOrTx(
                () -> this.db.testRevision().bulkUpsertWithRetries(
                        new ArrayList<>(modified.values()), 10000, e -> statistics.onBulkInsertError()
                )
        );
    }

    private void processChanged(
            TestLaunchEntity record,
            Map<TestRevisionEntity.Id, TestRevisionEntity> modified,
            Map<TestRevisionEntity.Id, TestImportantRevisionEntity> importantRevisions
    ) {
        var revisionId = new TestRevisionEntity.Id(record.getId().getStatusId(), record.getId().getRevisionNumber());
        var bucketNumber = fragmentationSettings.getBucket(record.getId().getRevisionNumber());
        var history = cache.get(revisionId.getStatusId());
        var current = history.getRevision(revisionId.getRevision());
        var previous = history.getPrevious(revisionId);
        var next = history.getNext(revisionId);

        Common.TestStatus status;
        if (current.isEmpty() || record.getStatus() == current.get().getStatus()) {
            status = record.getStatus();
            if (current.isPresent()) {
                log.info("Same status for: {}", record.getId().getStatusId());
            }
        } else {
            log.info(
                    "Marking as flaky {}, previous: {}, new: {}",
                    record.getId().getStatusId(), record.getStatus(), current.get().getStatus()
            );
            status = Common.TestStatus.TS_FLAKY;
        }
        var revision = TestRevisionEntity.builder()
                .id(revisionId)
                .status(status)
                .previousRevision(previous.map(x -> x.getId().getRevision()).orElse(0L))
                .nextRevision(next.map(x -> x.getId().getRevision()).orElse(0L))
                .changed(previous.isPresent() && previous.get().getStatus() != status)
                .previousStatus(previous.isPresent() ? previous.get().getStatus() : Common.TestStatus.TS_NONE)
                .revisionCreated(record.getRevisionCreated())
                .uid(record.getUid())
                .build();

        modified.put(revision.getId(), revision);
        if ((current.isEmpty() && revision.isChanged()) ||
                (current.isPresent() && current.get().isChanged() != revision.isChanged())) {
            importantRevisions.put(revision.getId(), revision.toImportant());
        }

        var regionOptional = history.getRegion(bucketNumber.getRegion());
        if (regionOptional.isEmpty()) {
            regionOptional = Optional.of(history.createRegion(bucketNumber.getRegion()));
        }
        var region = regionOptional.get();
        var bucketOptional = region.get(bucketNumber.getBucket());
        if (bucketOptional.isEmpty()) {
            region.createBucket(bucketNumber.getBucket());
        }

        history.put(revision);

        if (previous.isPresent() && previous.get().getNextRevision() != revisionId.getRevision()) {
            previous = Optional.of(previous.get().toBuilder()
                    .nextRevision(revisionId.getRevision())
                    .build());

            modified.put(previous.get().getId(), previous.get());
            history.put(previous.get());
        }

        if (next.isPresent() && next.get().getPreviousRevision() != revisionId.getRevision()) {
            var oldIsChanged = next.get().isChanged();
            next = Optional.of(
                    next.get().toBuilder()
                            .previousRevision(revisionId.getRevision())
                            .changed(next.get().getStatus() != status)
                            .previousStatus(status)
                            .build()
            );
            modified.put(next.get().getId(), next.get());
            history.put(next.get());

            if (oldIsChanged != next.get().isChanged()) {
                importantRevisions.put(next.get().getId(), next.get().toImportant());
            }
        }
    }
}
