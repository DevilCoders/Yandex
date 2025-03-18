package ru.yandex.ci.storage.core.archive;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.common.collect.Lists;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.util.Retryable;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
@AllArgsConstructor
public class CheckArchiveService {
    private static final int BATCH_SIZE = 10000;

    private final Clock clock;
    private final CiStorageDb db;

    private final BazingaTaskManager taskManager;

    private final Duration archiveOlderThan;
    private final int maxArchiveInProgress;

    public void planChecksToArchive() {
        var checks = db.currentOrReadOnly(
                () -> db.checks().getInArchiveState(CheckOuterClass.ArchiveState.AS_ARCHIVING, clock.instant(), 1000)
        );

        log.info(
                "{} checks in progress: {}",
                checks.size(), checks.stream().map(CheckEntity::getId).collect(Collectors.toSet())
        );

        if (checks.size() >= maxArchiveInProgress) {
            return;
        }

        var cut = clock.instant().minus(archiveOlderThan);
        var freeSlots = maxArchiveInProgress - checks.size();
        var checksToArchive = new ArrayList<CheckEntity>();
        checksToArchive.addAll(db.currentOrReadOnly(
                () -> db.checks().getInArchiveState(null, cut, freeSlots)
        ));
        checksToArchive.addAll(db.currentOrReadOnly(
                () -> db.checks().getInArchiveState(
                        CheckOuterClass.ArchiveState.AS_NONE, cut, freeSlots - checksToArchive.size()
                )
        ));

        log.info(
                "Got {} checks to archive: {}",
                checksToArchive.size(), checksToArchive.stream().map(CheckEntity::getId).collect(Collectors.toSet())
        );

        this.db.currentOrTx(() -> {
            this.db.checks().save(
                    checksToArchive.stream().map(
                            c -> c.toBuilder().archiveState(CheckOuterClass.ArchiveState.AS_ARCHIVING).build()
                    ).toList()
            );

            checksToArchive.forEach(
                    check -> taskManager.schedule(new ArchiveTask(new ArchiveTask.Params(check.getId().getId())))
            );
        });
    }

    public void archive(CheckEntity.Id id) {
        var check = db.currentOrReadOnly(() -> db.checks().get(id));
        if (check.getArchiveState() == CheckOuterClass.ArchiveState.AS_ARCHIVED) {
            log.info("Check {} already archived", check.getId());
            return;
        }

        updateIterations(id);

        dropChunks(check.getId());

        db.currentOrReadOnly(() -> dropFromTablesInTx(check));

        markCheckAsArchived(id);

        log.info("Archive completed for {}", check.getId());
    }

    private void dropFromTablesInTx(CheckEntity check) {
        dropInTx(() -> db.testDiffs().fetchIdsByCheck(check.getId(), BATCH_SIZE), db::testDiffs);
        dropInTx(() -> db.testResults().fetchIdsByCheck(check.getId(), BATCH_SIZE), db::testResults);
        dropInTx(() -> db.testDiffsBySuite().fetchIdsByCheck(check.getId(), BATCH_SIZE), db::testDiffsBySuite);
        dropInTx(() -> db.checkTasks().fetchIdsByCheck(check.getId(), BATCH_SIZE), db::checkTasks);
        dropInTx(() -> db.checkTaskStatistics().fetchIdsByCheck(check.getId(), BATCH_SIZE), db::checkTaskStatistics);
        dropInTx(() -> db.largeTasks().fetchIdsByCheck(check.getId(), BATCH_SIZE), db::largeTasks);
        dropInTx(() -> db.suiteRestarts().fetchIdsByCheck(check.getId(), BATCH_SIZE), db::suiteRestarts);
        dropInTx(() -> db.checkTextSearch().fetchIdsByCheck(check.getId(), BATCH_SIZE), db::checkTextSearch);
    }

    private void markCheckAsArchived(CheckEntity.Id id) {
        db.currentOrTx(() -> db.checks().save(
                        db.checks().get(id).toBuilder()
                                .archived(Instant.now())
                                .archiveState(CheckOuterClass.ArchiveState.AS_ARCHIVED)
                                .build()
                )
        );
    }

    private void updateIterations(CheckEntity.Id id) {
        db.currentOrTx(() -> {
            var iterations = db.checkIterations().findByCheck(id);
            log.info("{} iterations", iterations.size());
            for (var iteration : iterations) {
                db.checkIterations().save(
                        iteration.toBuilder()
                                .useSuiteDiffIndex(false)
                                .build()
                );
            }
        });
    }

    private void dropChunks(CheckEntity.Id checkId) {
        var chunkAggregateIds = db.currentOrReadOnly(() -> db.chunkAggregates().findIdsByCheckId(checkId));
        log.info("{} chunk aggregates", chunkAggregateIds.size());

        db.currentOrReadOnly(() -> {
            for (var chunkId : chunkAggregateIds) {
                log.info("Dropping aggregate {}", chunkId);
                var idList = db.testDiffsByHash().getChunkIds(chunkId);

                int count = 0;
                for (var list : Lists.partition(idList, BATCH_SIZE)) {
                    var set = Set.copyOf(list);
                    Retryable.retry(
                            () -> db.tx(() -> db.testDiffsByHash().delete(set)),
                            (t) -> log.warn("Failed to delete {} chunks", set.size()),
                            true, 2, 32, 8
                    );
                    count += set.size();
                    log.info("Dropped {}", count);
                }
                log.info("Complete, dropped {} diffs by hash", idList.size());
            }
        });

        db.currentOrTx(() -> db.chunkAggregates().delete(Set.copyOf(chunkAggregateIds)));
        log.info("Complete, dropped {} aggregates", chunkAggregateIds.size());
    }

    private <T extends Entity<T>, ID extends Entity.Id<T>> void dropInTx(
            Supplier<Stream<? extends ID>> loader,
            Supplier<KikimrTableCi<T>> table
    ) {
        var tableName = table.get().getTableName();
        log.info("Dropping from `{}`", tableName);

        var count = 0;
        while (true) {
            var ids = new HashSet<ID>();
            Retryable.retry(
                    () -> ids.addAll(loader.get().collect(Collectors.toSet())),
                    (t) -> log.info("Failed to fetch ids: {}", t.getMessage()),
                    true, 2, 32, 8
            );

            if (ids.isEmpty()) {
                break;
            }
            count += ids.size();

            Retryable.retry(
                    () -> db.tx(() -> table.get().delete(ids)),
                    (t) -> log.warn("Failed to delete {} ids", ids.size()),
                    true, 2, 32, 8
            );
            log.info("Dropped {}", count);
        }

        log.info("Complete, dropped {} from `{}`", count, tableName);
    }
}
