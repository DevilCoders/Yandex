package ru.yandex.ci.observer.api.stress_test;

import java.time.Duration;
import java.util.Collection;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.IsolationLevel;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check.CheckTable;
import ru.yandex.ci.observer.core.db.model.stress_test.StressTestUsedCommitEntity;
import ru.yandex.ci.util.ObjectStore;

@Slf4j
@RequiredArgsConstructor
public class StressTestService {

    private static final int MAX_CHECKS = 100_000;

    @Nonnull
    private final CiObserverDb db;
    @Nonnull
    private final ArcService arcService;

    public List<CheckTable.RevisionsView> getNotUsedRevisions(
            @Nonnull ArcRevision startTrunkRevision,
            @Nonnull Duration duration,
            long revisionsPerHour,
            @Nonnull String namespace
    ) {
        var commit = arcService.getCommit(startTrunkRevision);
        Preconditions.checkArgument(commit.isTrunk(), "commit %s should belong to trunk", startTrunkRevision);

        var targetRevisionCount = duration.toHours() * revisionsPerHour;
        var targetRevisionByCommitId = new LinkedHashMap<String, CheckTable.RevisionsView>();

        var offset = ObjectStore.of(0L);
        do {
            var revisions = db.scan().withMaxSize(MAX_CHECKS).run(() -> db.checks().findAll(
                    commit.getSvnRevision(), Math.min(MAX_CHECKS, targetRevisionCount * 2), offset.get()
            ));
            if (revisions.isEmpty()) {
                throw createNotEnoughCommitsException(startTrunkRevision, duration, revisionsPerHour,
                        targetRevisionCount, targetRevisionByCommitId.keySet());
            }

            offset.set(offset.get() + revisions.size());

            var freeRevisions = db.readOnly()
                    .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                    .run(() -> {
                        long min = Long.MAX_VALUE;
                        long max = Long.MIN_VALUE;
                        for (var revision : revisions) {
                            min = Math.min(min, revision.getLeft().getRevisionNumber());
                            max = Math.max(max, revision.getLeft().getRevisionNumber());
                        }

                        var usedCommitIds = db.stressTestUsedCommitTable().findUsedRightRevisions(namespace, min,
                                max);

                        return revisions.stream()
                                .filter(it -> !usedCommitIds.contains(it.getRight().getRevision()))
                                .toList();
                    });

            /* Add to targetRevisions outside of transaction,
               cause transaction can be retried when OptimisticLockException */
            for (var freeRevision : freeRevisions) {
                targetRevisionByCommitId.put(freeRevision.getRight().getRevision(), freeRevision);
            }
        } while (targetRevisionByCommitId.size() < targetRevisionCount);

        return targetRevisionByCommitId.values()
                .stream()
                .sorted(Comparator.comparing(it -> it.getLeft().getRevisionNumber()))
                .limit(targetRevisionCount)
                .toList();
    }

    public void markAsUsed(ArcRevision rightRevision, ArcRevision leftRevision, String namespace,
                           String flowLaunchId) {
        var rightCommit = arcService.getCommit(rightRevision);
        var leftCommit = arcService.getCommit(leftRevision);
        db.currentOrTx(() -> db.stressTestUsedCommitTable().save(StressTestUsedCommitEntity.builder()
                .id(StressTestUsedCommitEntity.Id.of(
                        rightCommit.getCommitId(),
                        namespace,
                        leftCommit.getSvnRevision()
                ))
                .flowLaunchId(flowLaunchId)
                .build()
        ));
        log.info("right {}, left {}, namespace {}, flowLaunchId {} marked as used",
                rightRevision, leftRevision, namespace, flowLaunchId);
    }

    public List<StressTestUsedCommitEntity> findUsedRightRevisions(List<String> rightRevisions, String namespace) {
        if (rightRevisions.isEmpty()) {
            return List.of();
        }
        return db.currentOrReadOnly(() ->
                db.stressTestUsedCommitTable().findUsedRightRevisions(rightRevisions, namespace)
        );
    }

    private static IllegalArgumentException createNotEnoughCommitsException(
            @Nonnull ArcRevision startRevision,
            @Nonnull Duration duration,
            long revisionsPerHour,
            long targetRevisionCount,
            Collection<?> foundRevisions
    ) {
        var message = ("Not enough commits found. " +
                "Required revisions count %d, found %d. " +
                "Requested startRevision %s, duration %s, revisionsPerHour %d"
        ).formatted(targetRevisionCount, foundRevisions.size(), startRevision, duration, revisionsPerHour);
        return new IllegalArgumentException(message);
    }

}
