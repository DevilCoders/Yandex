package ru.yandex.ci.engine.launch.auto;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.EntityNotFoundException;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.lang.NonNullApi;

@Slf4j
@NonNullApi
@RequiredArgsConstructor
public class DiscoveryProgressChecker {

    private static final int BATCH_SIZE = 1000;

    private static final String DIR_DISCOVERY_LAST_PROCESSED =
            "DiscoveryProgressChecker.dirDiscoveryLastProcessed";
    private static final String GRAPH_DISCOVERY_LAST_PROCESSED =
            "DiscoveryProgressChecker.graphDiscoveryLastProcessed";
    private static final String STORAGE_DISCOVERY_LAST_PROCESSED =
            "DiscoveryProgressChecker.storageDiscoveryLastProcessed";
    private static final String PCI_DSS_DISCOVERY_LAST_PROCESSED =
            "DiscoveryProgressChecker.pciDssDiscoveryLastProcessed";

    @Nonnull
    private final Duration lastProcessedCommitInitializationDelay;
    private final boolean failIfUninitialized;
    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final Clock clock;

    private final int maxProcessedCommitsPerRun;

    public void check() {
        check(ArcBranch.trunk(), DiscoveryType.DIR);
        check(ArcBranch.trunk(), DiscoveryType.GRAPH);
        check(ArcBranch.trunk(), DiscoveryType.STORAGE);
        check(ArcBranch.trunk(), DiscoveryType.PCI_DSS);
    }

    private void check(ArcBranch branch, DiscoveryType discoveryType) {
        var lastProcessed = db.currentOrTx(() ->
                tryGetOrInitLastProcessedCommit(branch, discoveryType)
        );
        if (lastProcessed == null) {
            log.info("lastProcessed {} is null, exiting", discoveryType);
            return;
        }

        int processed = 0;
        var newLastProcessed = lastProcessed;
        while (newLastProcessed != null && newLastProcessed.isDiscoveryFinished(discoveryType)
                && processed < maxProcessedCommitsPerRun) {
            String commitId = newLastProcessed.getId().getCommitId();

            newLastProcessed = db.currentOrTx(() -> {
                Set<CommitDiscoveryProgress.Id> ids = db.arcCommit()
                        .findChildCommitIds(commitId)
                        .stream()
                        .map(ArcCommit.Id::getCommitId)
                        .map(CommitDiscoveryProgress.Id::of)
                        .collect(Collectors.toSet());

                var childCommit = db.commitDiscoveryProgress().find(ids)
                        .stream()
                        .filter(it -> it.getArcRevision().getBranch().equals(branch))
                        .findFirst()
                        .orElse(null);

                if (childCommit != null) {
                    updateLastProcessedCommitInTx(childCommit.getArcRevision(), discoveryType);
                    childCommit = markThatDiscoveryFinishedForParents(childCommit, discoveryType);
                }
                return childCommit;
            });

            processed++;
        }

        log.info("discoveryType {}, newLastProcessed {}", discoveryType, newLastProcessed);
    }

    private CommitDiscoveryProgress markThatDiscoveryFinishedForParents(
            CommitDiscoveryProgress progress,
            DiscoveryType discoveryType
    ) {
        var updatedProgress = progress.withDiscoveryFinishedForParents(discoveryType);
        return db.commitDiscoveryProgress().save(updatedProgress);
    }

    public void updateLastProcessedCommitInTx(OrderedArcRevision arcRevision, DiscoveryType discoveryType) {
        var ns = getNamespace(discoveryType);
        db.keyValue().setValue(ns, arcRevision.getBranch().asString(), arcRevision.getCommitId());
    }

    public Optional<String> getLastProcessedCommitInTx(ArcBranch branch, DiscoveryType discoveryType) {
        var ns = getNamespace(discoveryType);
        return db.keyValue().findString(ns, branch.asString());
    }

    private String getNamespace(DiscoveryType discoveryType) {
        return switch (discoveryType) {
            case DIR -> DIR_DISCOVERY_LAST_PROCESSED;
            case GRAPH -> GRAPH_DISCOVERY_LAST_PROCESSED;
            case STORAGE -> STORAGE_DISCOVERY_LAST_PROCESSED;
            case PCI_DSS -> PCI_DSS_DISCOVERY_LAST_PROCESSED;
        };
    }

    @Nullable
    private CommitDiscoveryProgress tryGetOrInitLastProcessedCommit(ArcBranch branch, DiscoveryType discoveryType) {
        Optional<String> lastProcessedCommit = getLastProcessedCommitInTx(branch, discoveryType);
        log.info("lastProcessedCommit ({}, {}) is {}", branch, discoveryType, lastProcessedCommit);
        return lastProcessedCommit
                .flatMap(it -> db.commitDiscoveryProgress().find(it))
                .orElseGet(() -> {
                    log.info("lastProcessedCommit ({}, {}) is null, trying initialize", branch, discoveryType);
                    return initializeLastProcessedCommit(branch, discoveryType);
                });
    }

    @Nullable
    private CommitDiscoveryProgress initializeLastProcessedCommit(ArcBranch branch, DiscoveryType discoveryType) {
        // The problem is - this is totally useless on a real environment (we have 841K+ commits already)
        // However we need this for tests
        if (failIfUninitialized) {
            throw new RuntimeException("Discovery type " + discoveryType + " it not initialized." +
                    " Please initialize KeyValue with initial value using ns " + getNamespace(discoveryType));
        }
        return db.commitDiscoveryProgress()
                .streamAll(BATCH_SIZE)
                .filter(it -> it.getArcRevision().getBranch().equals(branch))
                .min(Comparator.comparingLong(it -> it.getArcRevision().getNumber()))
                .filter(it -> {
                    Instant commitIsEnoughOldCriteria = clock.instant()
                            .minusMillis(lastProcessedCommitInitializationDelay.toMillis());
                    ArcCommit arcCommit = db.arcCommit().findOptional(it.getId().getCommitId())
                            .orElseThrow(() -> new EntityNotFoundException(
                                    "ArcCommit not found: %s".formatted(it.getId().getCommitId())
                            ));
                    return arcCommit.getCreateTime().isBefore(commitIsEnoughOldCriteria);
                })
                .map(lastProcessedCommit -> {
                    updateLastProcessedCommitInTx(lastProcessedCommit.getArcRevision(), discoveryType);
                    lastProcessedCommit = markThatDiscoveryFinishedForParents(lastProcessedCommit, discoveryType);
                    log.info("lastProcessedCommit ({}, {}) was initialized with {}", branch, discoveryType,
                            lastProcessedCommit);
                    return lastProcessedCommit;
                })
                .orElse(null);
    }

    public List<String> findNotProcessedPciDssCommits() {
        return db.commitDiscoveryProgress().findPciDssStateNotProcessed()
                .stream()
                .map(cdp -> cdp.getArcRevision().getCommitId())
                .toList();
    }

}
