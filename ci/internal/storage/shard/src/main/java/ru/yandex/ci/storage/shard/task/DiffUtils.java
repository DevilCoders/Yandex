package ru.yandex.ci.storage.shard.task;

import java.time.Instant;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.shard.cache.ShardCache;
import ru.yandex.ci.util.HostnameUtils;

@Slf4j
public class DiffUtils {
    private DiffUtils() {

    }

    public static void copyBuildOrConfigureOrSuiteDiffsToMetaIteration(
            ShardCache.Modifiable cache,
            TestDiffByHashEntity.Id diffId,
            int iterationNumber
    ) {
        var allToolchainsDiffId = diffId.toAllToolchainsDiffId();

        cloneDiff(cache, iterationNumber, diffId);
        cloneDiff(cache, iterationNumber, allToolchainsDiffId);
    }

    public static void copyTestDiffsToMetaIteration(
            ShardCache.Modifiable cache,
            TestDiffByHashEntity.Id diffId,
            int iterationNumber
    ) {
        var allToolchainsDiffId = diffId.toAllToolchainsDiffId();
        var suiteDiffId = diffId.toSuiteDiffId();
        var suiteAllToolchainsDiffId = suiteDiffId.toAllToolchainsDiffId();

        cloneDiff(cache, iterationNumber, diffId);
        cloneDiff(cache, iterationNumber, allToolchainsDiffId);
        cloneDiff(cache, iterationNumber, suiteDiffId);
        cloneDiff(cache, iterationNumber, suiteAllToolchainsDiffId);
    }

    private static void cloneDiff(
            ShardCache.Modifiable cache, int iterationNumber, TestDiffByHashEntity.Id diffId
    ) {
        var diff = cache.testDiffs().get(diffId);
        if (diff.isEmpty()) {
            cache.testDiffs()
                    .findInPreviousIterations(diffId, iterationNumber)
                    .ifPresent(d -> writeDiffWithId(cache, diffId, d));
        } else {
            log.debug("No diff to clone {}", diffId);
        }
    }

    private static void writeDiffWithId(
            ShardCache.Modifiable cache,
            TestDiffByHashEntity.Id diffId,
            TestDiffByHashEntity diff
    ) {
        log.info("Cloning diff from {} to {}", diff.getId(), diffId);
        cache.testDiffs().put(
                diff.toBuilder()
                        .id(diffId)
                        .created(Instant.now())
                        .isLast(true)
                        .processedBy(HostnameUtils.getShortHostname())
                        .clonedFromIteration(diff.getId().getAggregateId().getIterationId().getNumber())
                        .build()
        );

        cache.testDiffs().put(diff.toBuilder().isLast(false).build());
    }
}
