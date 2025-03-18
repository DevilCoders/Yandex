package ru.yandex.ci.storage.shard.task;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.Common.TestStatus;
import ru.yandex.ci.storage.core.check.TaskResultComparer;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.test.TestTag;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffStatistics;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.shard.CalculationLogger;
import ru.yandex.ci.storage.shard.cache.ShardCache;

/**
 * Docs at https://docs.yandex-team.ru/ci/dev/storage#Подсчет-статистики
 */
@Slf4j
public class AggregateProcessor {
    private static final Set<TestStatus> NOT_FLAKY_TRANSITION_FROM = Set.of(
            TestStatus.TS_UNKNOWN,
            TestStatus.TS_NONE,
            TestStatus.TS_SKIPPED,
            TestStatus.TS_DISCOVERED,
            TestStatus.TS_BROKEN_DEPS,
            TestStatus.TS_SUITE_PROBLEMS
    );

    private static final Set<TestStatus> NOT_FLAKY_TRANSITION_TO = Set.of(
            TestStatus.TS_SKIPPED,
            TestStatus.TS_DISCOVERED,
            TestStatus.TS_BROKEN_DEPS,
            TestStatus.TS_SUITE_PROBLEMS
    );

    private final TaskDiffProcessor taskDiffProcessor;

    private static final Set<TestStatus> OK_TEST_STATUSES = Set.of(
            TestStatus.TS_NONE, TestStatus.TS_OK, TestStatus.TS_DISCOVERED, TestStatus.TS_SKIPPED
    );

    public AggregateProcessor(TaskDiffProcessor taskDiffProcessor) {
        this.taskDiffProcessor = taskDiffProcessor;
    }

    public void process(
            ShardCache.Modifiable cache,
            ChunkAggregateEntity.ChunkAggregateEntityMutable aggregate,
            TestResult result
    ) {
        process(cache, aggregate, null, result);
    }

    public void process(
            ShardCache.Modifiable cache,
            ChunkAggregateEntity.ChunkAggregateEntityMutable aggregate,
            @Nullable ChunkAggregateEntity.ChunkAggregateEntityMutable metaAggregate,
            TestResult result
    ) {
        if (metaAggregate != null) {
            process(cache, metaAggregate, result, result.getStatus());
        }
        process(cache, aggregate, result, result.getStatus());
    }

    private void process(
            ShardCache.Modifiable cache,
            ChunkAggregateEntity.ChunkAggregateEntityMutable aggregate,
            TestResult result,
            TestStatus status
    ) {
        var iterationNumber = result.getDiffId().getIterationNumber();
        var diffId = TestDiffByHashEntity.Id.of(aggregate.getId(), result.getId().getFullTestId());

        if (status.equals(TestStatus.TS_UNKNOWN)) {
            log.error("Unknown test status for {}", diffId);
            return;
        }

        var isMetaIteration = aggregate.getId().getIterationId().isMetaIteration();

        if (ResultTypeUtils.isTest(result.getResultType())) {
            if (isMetaIteration) {
                DiffUtils.copyTestDiffsToMetaIteration(cache, diffId, iterationNumber);
            }
            processTestResult(cache, aggregate, result, diffId, status);
        } else {
            if (isMetaIteration) {
                DiffUtils.copyBuildOrConfigureOrSuiteDiffsToMetaIteration(cache, diffId, iterationNumber);
            }
            processBuildOrConfigureOrSuiteResult(cache, aggregate, result, diffId, status);
        }
    }

    private void processTestResult(
            ShardCache.Modifiable cache,
            ChunkAggregateEntity.ChunkAggregateEntityMutable aggregate,
            TestResult result,
            TestDiffByHashEntity.Id diffId,
            TestStatus status
    ) {
        var allToolchainsDiff = cache.testDiffs().getOrCreate(diffId.toAllToolchainsDiffId(), result);
        var suiteDiff = cache.testDiffs().getOrCreate(diffId.toSuiteDiffId(), result);
        var suiteAllToolchainsDiff = cache.testDiffs().getOrCreate(suiteDiff.getId().toAllToolchainsDiffId(), result);

        final var originalDiff = cache.testDiffs().getOrCreate(diffId, result);

        status = modifyStatusBySuite(
                originalDiff.getId(), status, result.isLeft() ? suiteDiff.getLeft() : suiteDiff.getRight()
        );

        if (result.getAutocheckChunkId() != null) {
            var chunkDiff = cache.testDiffs().getOrCreate(diffId.toChunkId(result.getAutocheckChunkId()), result);
            status = modifyStatusBySuite(
                    originalDiff.getId(), status, result.isLeft() ? chunkDiff.getLeft() : chunkDiff.getRight()
            );
        }

        var updatedAllToolchainsDiff = allToolchainsDiff.toBuilder();

        var updatedDiff = updateStatus(
                cache,
                result,
                status,
                allToolchainsDiff,
                originalDiff,
                updatedAllToolchainsDiff
        );

        if (updatedDiff.equals(originalDiff)) {
            log.debug("Diff not changed {}", updatedDiff.getId());
            return;
        }

        var allToolchainsStatistics = allToolchainsDiff.getStatistics().toMutable();
        var suiteStatistics = suiteDiff.getStatistics().toMutable();
        var suiteAllToolchainsStatistics = suiteAllToolchainsDiff.getStatistics().toMutable();

        taskDiffProcessor.rollbackTestDiff(
                originalDiff,
                allToolchainsStatistics,
                suiteStatistics,
                suiteAllToolchainsStatistics,
                aggregate
        );

        taskDiffProcessor.applyTestDiff(
                updatedDiff,
                allToolchainsStatistics,
                suiteStatistics,
                suiteAllToolchainsStatistics,
                aggregate
        );

        cache.testDiffs().put(
                List.of(
                        updatedDiff,
                        updatedAllToolchainsDiff.statistics(allToolchainsStatistics.toImmutable()).build(),
                        suiteDiff.toBuilder().statistics(suiteStatistics.toImmutable()).build(),
                        suiteAllToolchainsDiff.toBuilder()
                                .statistics(suiteAllToolchainsStatistics.toImmutable())
                                .build()
                )
        );
    }

    private void processBuildOrConfigureOrSuiteResult(
            ShardCache.Modifiable cache,
            ChunkAggregateEntity.ChunkAggregateEntityMutable aggregate,
            TestResult result,
            TestDiffByHashEntity.Id diffId,
            TestStatus status
    ) {
        var allToolchainsDiff = cache.testDiffs().getOrCreate(diffId.toAllToolchainsDiffId(), result);
        var updatedAllToolchainsDiff = allToolchainsDiff.toBuilder();
        final var originalDiff = cache.testDiffs().getOrCreate(diffId, result);

        var updatedDiff = updateStatus(
                cache, result, status, allToolchainsDiff, originalDiff, updatedAllToolchainsDiff
        );

        if (updatedDiff.equals(originalDiff)) {
            log.debug("Diff not changed {}", updatedDiff.getId());
            return;
        }

        var allToolchainsStatistics = allToolchainsDiff.getStatistics().toMutable();

        taskDiffProcessor.rollbackBuildOrConfigureOrSuiteDiff(
                originalDiff,
                allToolchainsStatistics,
                aggregate
        );

        taskDiffProcessor.applyBuildOrConfigureOrSuiteDiff(
                updatedDiff,
                allToolchainsStatistics,
                aggregate
        );

        cache.testDiffs().put(
                List.of(
                        updatedDiff,
                        updatedAllToolchainsDiff.statistics(allToolchainsStatistics.toImmutable()).build()
                )
        );
    }

    private TestStatus modifyStatusOnFlaky(
            TestResult result, TestStatus status, TestDiffByHashEntity originalDiff
    ) {
        if (isFlakyResult(result, originalDiff)) {
            log.info(
                    "Status changed, marking as flaky diff {}, old left {}, old right {}, new {} status {}, type {}",
                    originalDiff.getId(),
                    originalDiff.getLeft(),
                    originalDiff.getRight(),
                    result.isRight() ? "right" : "left",
                    result.getStatus(),
                    originalDiff.getDiffType()
            );
            status = TestStatus.TS_FLAKY;
        }

        return status;
    }

    private TestStatus modifyStatusBySuite(TestDiffByHashEntity.Id id, TestStatus status, TestStatus suiteStatus) {
        if (status != TestStatus.TS_NONE) {
            return status;
        }

        var updatedStatus = switch (suiteStatus) {
            case TS_UNKNOWN, UNRECOGNIZED, TS_XFAILED, TS_OK, TS_DISCOVERED -> status;
            case TS_FLAKY, TS_INTERNAL, TS_TIMEOUT, TS_MULTIPLE_PROBLEMS,
                    TS_SUITE_PROBLEMS, TS_FAILED, TS_XPASSED -> TestStatus.TS_SUITE_PROBLEMS;
            case TS_SKIPPED, TS_BROKEN_DEPS, TS_NONE, TS_NOT_LAUNCHED -> suiteStatus;
        };

        log.debug("Modified status for {} from {} to {}, on suite status {}", id, status, updatedStatus, suiteStatus);

        return updatedStatus;
    }

    private TestDiffByHashEntity updateStatus(
            ShardCache.Modifiable cache,
            TestResult result,
            TestStatus status,
            TestDiffByHashEntity originalAllToolchainsDiff,
            TestDiffByHashEntity originalDiff,
            TestDiffByHashEntity.Builder updatedAllToolchainsDiff
    ) {
        var isMuted = cache.muteStatus().get(TestStatusEntity.Id.idInTrunk(originalDiff.getId().getTestId()));

        if (result.getId().getIterationId().getNumber() > 1 && originalDiff.isUnknown()) {
            var previousDiff = cache.testDiffs()
                    .findInPreviousIterations(originalDiff.getId(), result.getId().getIterationId().getNumber());
            if (previousDiff.isPresent()) {
                if (previousDiff.get().isUnknown()) {
                    log.error("Previous diff is unknown {} for {}", previousDiff.get().getId(), originalDiff.getId());
                }

                originalDiff = originalDiff.toBuilder()
                        .left(getStatusOrNone(previousDiff.get().getLeft()))
                        .right(getStatusOrNone(previousDiff.get().getRight()))
                        .build();
            }
        }

        status = modifyStatusOnFlaky(result, status, originalDiff);

        var updatedDiff = originalDiff.toBuilder().isMuted(isMuted);

        fillCommonFields(result, originalDiff, updatedDiff);
        fillCommonFields(result, originalAllToolchainsDiff, updatedAllToolchainsDiff);

        if (result.isRight()) {
            var diffType = TaskResultComparer.compare(
                    originalDiff.getLeft(), status, updatedDiff.getIsExternal(result.isOwner())
            );

            updatedDiff
                    .right(status)
                    .diffType(diffType)
                    .statistics(
                            new TestDiffStatistics(
                                    taskDiffProcessor.calculateStageDiffStatistics(
                                            diffType,
                                            ResultTypeUtils.isBuild(result.getResultType()),
                                            updatedDiff.getIsMuted(),
                                            updatedDiff.getIsStrongMode()
                                    ),
                                    originalDiff.getStatistics().getChildren()
                            )
                    );

            var allToolchainsStatus = getAllToolchainsStatus(originalAllToolchainsDiff.getRight(), status);
            updatedAllToolchainsDiff
                    .right(allToolchainsStatus)
                    .diffType(
                            TaskResultComparer.compare(
                                    originalAllToolchainsDiff.getLeft(), allToolchainsStatus,
                                    updatedAllToolchainsDiff.getIsExternal(result.isOwner())
                            )
                    );
        } else {
            var diffType = TaskResultComparer.compare(
                    status, originalDiff.getRight(), updatedDiff.getIsExternal(result.isOwner())
            );
            updatedDiff
                    .left(status)
                    .diffType(diffType)
                    .statistics(
                            new TestDiffStatistics(
                                    taskDiffProcessor.calculateStageDiffStatistics(
                                            diffType,
                                            ResultTypeUtils.isBuild(result.getResultType()),
                                            updatedDiff.getIsMuted(),
                                            updatedDiff.getIsStrongMode()
                                    ),
                                    originalDiff.getStatistics().getChildren()
                            )
                    );

            var allToolchainsStatus = getAllToolchainsStatus(originalAllToolchainsDiff.getLeft(), status);
            updatedAllToolchainsDiff
                    .left(allToolchainsStatus)
                    .diffType(
                            TaskResultComparer.compare(
                                    allToolchainsStatus,
                                    originalAllToolchainsDiff.getRight(),
                                    updatedAllToolchainsDiff.getIsExternal(result.isOwner())
                            )
                    );
        }

        var updatedDiffResult = updatedDiff.build();
        CalculationLogger.logStateUpdate(originalDiff, updatedDiffResult);
        return updatedDiffResult;
    }

    private TestStatus getStatusOrNone(TestStatus status) {
        return status == TestStatus.TS_UNKNOWN ? TestStatus.TS_NONE : status;

    }

    private boolean isFlakyResult(TestResult result, TestDiffByHashEntity diff) {
        if (diff.isExternal() || TestTag.isExternal(result.getTags())) {
            return false;
        }

        var oldStatus = result.isRight() ? diff.getRight() : diff.getLeft();
        if (NOT_FLAKY_TRANSITION_FROM.contains(oldStatus) || NOT_FLAKY_TRANSITION_TO.contains(result.getStatus())) {
            return false;
        }

        return !oldStatus.equals(result.getStatus());
    }

    private void fillCommonFields(
            TestResult result,
            TestDiffByHashEntity originalDiff,
            TestDiffByHashEntity.Builder diff
    ) {
        var tags = new HashSet<>(originalDiff.getTags());
        tags.addAll(result.getTags());
        diff
                .name(result.getName())
                .subtestName(result.getSubtestName())
                .tags(tags);

        if (result.isRight()) {
            diff
                    .requirements(result.getRequirements())
                    .oldTestId(result.getOldTestId())
                    .oldSuiteId(result.getOldSuiteId())
                    .isOwner(result.isOwner())
                    .owners(result.getOwners())
                    .isLaunchable(result.getIsLaunchable());
        }

        if (result.isRight()) {
            diff.rightIterationNumber(result.getId().getIterationId().getNumber());
        } else {
            diff.leftIterationNumber(result.getId().getIterationId().getNumber());
        }

        if (!originalDiff.getResultType().equals(result.getResultType()) && result.isRight()) {
            diff.resultType(result.getResultType());
        }
    }

    private TestStatus getAllToolchainsStatus(TestStatus previous, TestStatus next) {
        if (previous.equals(TestStatus.TS_UNKNOWN) || previous.equals(TestStatus.TS_NONE) ||
                previous.equals(TestStatus.TS_SKIPPED) || previous.equals(TestStatus.TS_DISCOVERED)) {
            return next;
        }

        if (next.equals(previous)) {
            return next;
        }

        if (next == TestStatus.TS_FLAKY || previous == TestStatus.TS_FLAKY) {
            return TestStatus.TS_FLAKY;
        }

        if (OK_TEST_STATUSES.contains(next)) {
            return previous;
        }

        if (OK_TEST_STATUSES.contains(previous)) {
            if (next.equals(TestStatus.TS_SKIPPED) || next.equals(TestStatus.TS_DISCOVERED) ||
                    next.equals(TestStatus.TS_NONE)) {
                return previous;
            }

            return next;
        }

        return TestStatus.TS_MULTIPLE_PROBLEMS;
    }
}
