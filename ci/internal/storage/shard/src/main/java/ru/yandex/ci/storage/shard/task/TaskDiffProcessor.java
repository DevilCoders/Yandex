package ru.yandex.ci.storage.shard.task;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.TestDiffType;
import ru.yandex.ci.storage.core.db.constant.TestDiffTypeUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics.Mutable;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity.ChunkAggregateEntityMutable;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateStatistics;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffStatistics;
import ru.yandex.ci.storage.shard.ShardStatistics;

import static ru.yandex.ci.storage.core.Common.TestDiffType.TDT_PASSED_NEW;

@Slf4j
public class TaskDiffProcessor {
    private final ShardStatistics shardStatistics;

    public TaskDiffProcessor(ShardStatistics shardStatistics) {
        this.shardStatistics = shardStatistics;
    }

    public TestDiffStatistics.StatisticsGroup calculateStageDiffStatistics(
            Common.TestDiffType diffType, boolean isBuild, boolean isMuted, boolean isStrongMode
    ) {
        return new TestDiffStatistics.StatisticsGroup(
                calculateStageDelta(diffType, isBuild, isStrongMode, isMuted),
                calculateExtendedDelta(diffType, isStrongMode, isMuted)
        );
    }

    public void applyTestDiff(
            TestDiffByHashEntity diff,
            TestDiffStatistics.Mutable allToolchainsStatistics,
            TestDiffStatistics.Mutable suiteStatistics,
            TestDiffStatistics.Mutable suiteAllToolchainsStatistics,
            ChunkAggregateEntityMutable aggregate
    ) {
        applyTestDiff(diff, allToolchainsStatistics, suiteStatistics, suiteAllToolchainsStatistics, aggregate, 1);
    }

    public void rollbackTestDiff(
            TestDiffByHashEntity diff,
            TestDiffStatistics.Mutable allToolchainsStatistics,
            TestDiffStatistics.Mutable suiteStatistics,
            TestDiffStatistics.Mutable suiteAllToolchainsStatistics,
            ChunkAggregateEntityMutable aggregate
    ) {
        applyTestDiff(diff, allToolchainsStatistics, suiteStatistics, suiteAllToolchainsStatistics, aggregate, -1);
    }

    public void applyBuildOrConfigureOrSuiteDiff(
            TestDiffByHashEntity diff,
            TestDiffStatistics.Mutable suiteAllToolchainsStatistics,
            ChunkAggregateEntityMutable aggregate
    ) {
        applyBuildOrConfigureOrSuiteDiff(diff, suiteAllToolchainsStatistics, aggregate, 1);
    }

    public void rollbackBuildOrConfigureOrSuiteDiff(
            TestDiffByHashEntity diff,
            TestDiffStatistics.Mutable suiteAllToolchainsStatistics,
            ChunkAggregateEntityMutable aggregate
    ) {
        applyBuildOrConfigureOrSuiteDiff(diff, suiteAllToolchainsStatistics, aggregate, -1);
    }

    private void applyTestDiff(
            TestDiffByHashEntity diff,
            TestDiffStatistics.Mutable allToolchainsStatistics,
            TestDiffStatistics.Mutable suiteStatistics,
            TestDiffStatistics.Mutable suiteAllToolchainsStatistics,
            ChunkAggregateEntityMutable aggregate,
            int sign
    ) {
        var self = diff.getStatistics().getSelf().toMutable();

        var statistics = aggregate.getStatistics();

        var toolchainAggregate = statistics.getToolchain(diff.getId().getTestId().getToolchain());
        var allToolchainsAggregate = statistics.getToolchain(TestEntity.ALL_TOOLCHAINS);

        if (sign > 0) {
            allToolchainsStatistics.getSelf().add(self, sign);
            suiteAllToolchainsStatistics.getChildren().add(allToolchainsStatistics.getSelf().normalized(), sign);
            suiteStatistics.getChildren().add(self, sign);

            updateAggregate(aggregate.getId(), toolchainAggregate, diff, suiteStatistics.getChildren(), sign);
            updateAggregate(
                    aggregate.getId(), allToolchainsAggregate, diff, suiteAllToolchainsStatistics.getChildren(), sign
            );
        } else {
            updateAggregate(aggregate.getId(), toolchainAggregate, diff, suiteStatistics.getChildren(), sign);
            updateAggregate(
                    aggregate.getId(), allToolchainsAggregate, diff, suiteAllToolchainsStatistics.getChildren(), sign
            );

            suiteStatistics.getChildren().add(self, sign);
            suiteAllToolchainsStatistics.getChildren().add(allToolchainsStatistics.getSelf().normalized(), sign);
            allToolchainsStatistics.getSelf().add(self, sign);

            if (suiteStatistics.getChildren().hasNegativeNumbers()) {
                log.error(
                        "Got negative numbers after rollback for suite statistics {}, children: {}",
                        diff.getId(), suiteStatistics.getChildren()
                );
                shardStatistics.onNegativeStatisticsError();
            }

            if (suiteAllToolchainsStatistics.getChildren().hasNegativeNumbers()) {
                log.error("Got negative numbers after rollback for suite all toolchains statistics {}", diff.getId());
                shardStatistics.onNegativeStatisticsError();
            }

            if (allToolchainsStatistics.getSelf().hasNegativeNumbers()) {
                log.error("Got negative numbers after rollback for all toolchains statistics {}", diff.getId());
                shardStatistics.onNegativeStatisticsError();
            }
        }
    }

    private void applyBuildOrConfigureOrSuiteDiff(
            TestDiffByHashEntity diff,
            TestDiffStatistics.Mutable suiteAllToolchainsStatistics,
            ChunkAggregateEntityMutable aggregate,
            int sign
    ) {
        var self = diff.getStatistics().getSelf().toMutable();
        var suiteStatistics = diff.getStatistics().toMutable();

        var statistics = aggregate.getStatistics();

        var toolchainAggregate = statistics.getToolchain(diff.getId().getTestId().getToolchain());
        var allToolchainsAggregate = statistics.getToolchain(TestEntity.ALL_TOOLCHAINS);

        if (sign > 0) {
            suiteAllToolchainsStatistics.getSelf().add(self, sign);

            var suiteSelfNormalized = suiteStatistics.getSelf().normalized();
            var suiteAllToolchainsSelfNormalized = suiteAllToolchainsStatistics.getSelf().normalized();

            updateAggregate(aggregate.getId(), toolchainAggregate, diff, suiteSelfNormalized, sign);
            updateAggregate(aggregate.getId(), allToolchainsAggregate, diff, suiteAllToolchainsSelfNormalized, sign);
        } else {
            var suiteSelfNormalized = suiteStatistics.getSelf().normalized();
            var suiteAllToolchainsSelfNormalized = suiteAllToolchainsStatistics.getSelf().normalized();

            updateAggregate(aggregate.getId(), toolchainAggregate, diff, suiteSelfNormalized, sign);
            updateAggregate(aggregate.getId(), allToolchainsAggregate, diff, suiteAllToolchainsSelfNormalized, sign);

            suiteAllToolchainsStatistics.getSelf().add(self, sign);
            if (suiteAllToolchainsStatistics.getSelf().hasNegativeNumbers()) {
                log.error("Got negative numbers after rollback for all toolchains statistics {}", diff.getId());
                shardStatistics.onNegativeStatisticsError();
            }
        }
    }

    private void updateAggregate(
            ChunkAggregateEntity.Id id,
            ChunkAggregateStatistics.ToolchainStatisticsMutable aggregate,
            TestDiffByHashEntity diff,
            TestDiffStatistics.StatisticsGroupMutable delta,
            int sign
    ) {
        aggregate.getMain().getTotal().add(delta.getStage(), sign);
        aggregate.getMain().getStage(diff.getResultType()).add(delta.getStage(), sign);
        aggregate.getExtended().add(delta.getExtended(), sign);

        if (sign < 0 && aggregate.hasNegativeNumbers()) {
            log.error("Got negative numbers after rollback for aggregate statistics {}, diff {}", id, diff.getId());
            shardStatistics.onNegativeStatisticsError();
        }
    }

    @SuppressWarnings("DuplicateBranchesInSwitch")
    private ExtendedStatistics calculateExtendedDelta(
            Common.TestDiffType diffType, boolean isStrongMode, boolean isMuted
    ) {
        // possible optimization: return statics
        var statistics = ExtendedStatistics.EMPTY.toMutable();
        if (isMuted) {
            statistics.getMuted().setTotal(1);

            if (diffType.equals(TDT_PASSED_NEW)) {
                statistics.getMuted().setPassedAdded(1);
            } else if (TestDiffTypeUtils.isAddedFailure(diffType)) {
                statistics.getMuted().setFailedAdded(1);
            }
        }

        var notMuted = !isMuted || isStrongMode;

        switch (diffType) {
            case TDT_PASSED -> {
                // Pure pass/failed/skip not counted here
            }
            case TDT_PASSED_NEW -> {
                statistics.getAdded().setTotal(1);
                statistics.getAdded().setPassedAdded(1);
            }
            case TDT_PASSED_FIXED, TDT_PASSED_BY_DEPS_FIXED,
                    TDT_FAILED, TDT_FAILED_BROKEN -> {
                // Pure pass/failed/skip not counted here
            }
            case TDT_FAILED_NEW -> {
                statistics.getAdded().setTotal(1);
                if (notMuted) {
                    statistics.getAdded().setFailedAdded(1);
                }
            }
            case TDT_FAILED_BY_DEPS, TDT_FAILED_BY_DEPS_BROKEN, TDT_FAILED_BY_DEPS_NEW,
                    TDT_SKIPPED -> {
                // Pure pass/failed/skip not counted here
            }
            case TDT_SKIPPED_NEW -> {
                statistics.getAdded().setTotal(1);
            }
            case TDT_DELETED -> statistics.getDeleted().setTotal(1);

            case TDT_TIMEOUT_FIXED -> {
                statistics.getTimeout().setTotal(1);
                if (notMuted) {
                    statistics.getTimeout().setPassedAdded(1);
                }
            }
            case TDT_TIMEOUT_BROKEN -> {
                statistics.getTimeout().setTotal(1);
                if (notMuted) {
                    statistics.getTimeout().setFailedAdded(1);
                }
            }
            case TDT_TIMEOUT_FAILED -> {
                statistics.getTimeout().setTotal(1);
            }
            case TDT_TIMEOUT_NEW -> {
                statistics.getAdded().setTotal(1);
                statistics.getTimeout().setTotal(1);

                if (notMuted) {
                    statistics.getTimeout().setFailedAdded(1);
                    statistics.getAdded().setFailedAdded(1);
                }
            }
            case TDT_FLAKY_FIXED -> {
                statistics.getFlaky().setTotal(1);
                if (notMuted) {
                    statistics.getFlaky().setPassedAdded(1);
                }
            }
            case TDT_FLAKY_BROKEN -> {
                statistics.getFlaky().setTotal(1);
                if (notMuted) {
                    statistics.getFlaky().setFailedAdded(1);
                }
            }
            case TDT_FLAKY_FAILED -> {
                statistics.getFlaky().setTotal(1);
            }
            case TDT_FLAKY_NEW -> {
                statistics.getFlaky().setTotal(1);
                if (notMuted) {
                    statistics.getFlaky().setFailedAdded(1);
                }
            }
            case TDT_EXTERNAL_FIXED -> {
                statistics.getExternal().setTotal(1);
                if (notMuted) {
                    statistics.getExternal().setPassedAdded(1);
                }
            }
            case TDT_EXTERNAL_BROKEN -> {
                statistics.getExternal().setTotal(1);
                if (notMuted) {
                    statistics.getExternal().setFailedAdded(1);
                }
            }
            case TDT_EXTERNAL_FAILED -> {
                statistics.getExternal().setTotal(1);
            }
            case TDT_EXTERNAL_NEW -> {
                statistics.getAdded().setTotal(1);
                statistics.getExternal().setTotal(1);
                if (notMuted) {
                    statistics.getExternal().setFailedAdded(1);
                    statistics.getAdded().setFailedAdded(1);
                }
            }
            case TDT_INTERNAL_FIXED -> {
                statistics.getInternal().setTotal(1);
                if (notMuted) {
                    statistics.getInternal().setPassedAdded(1);
                }
            }
            case TDT_INTERNAL_BROKEN -> {
                statistics.getInternal().setTotal(1);
                if (notMuted) {
                    statistics.getInternal().setFailedAdded(1);
                }
            }
            case TDT_INTERNAL_FAILED -> {
                statistics.getInternal().setTotal(1);
            }
            case TDT_INTERNAL_NEW -> {
                statistics.getAdded().setTotal(1);
                statistics.getInternal().setTotal(1);

                if (notMuted) {
                    statistics.getInternal().setFailedAdded(1);
                    statistics.getAdded().setFailedAdded(1);
                }
            }
            case TDT_SUITE_PROBLEMS -> {
                // skipping
            }
            case TDT_UNKNOWN -> {
                // waiting for other side status.
            }
            case UNRECOGNIZED -> {
                throw new RuntimeException("Unrecognized diff type " + diffType);
            }
            default -> throw new RuntimeException("Unsupported diff type " + diffType);
        }

        return statistics.toImmutable();
    }

    private StageStatistics calculateStageDelta(
            TestDiffType diffType, boolean isBuild, boolean isStrongMode, boolean isMuted
    ) {
        // possible optimization: return statics
        var statistics = new Mutable(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        if (diffType == Common.TestDiffType.TDT_UNKNOWN) {
            return statistics.toImmutable();
        }

        if (isMuted && !isStrongMode) {
            if (diffType == TestDiffType.TDT_PASSED || diffType == TestDiffType.TDT_PASSED_FIXED) {
                statistics.changePassed(1);
            }

            return statistics.toImmutable();
        }

        switch (diffType) {
            case TDT_PASSED, TDT_PASSED_NEW -> statistics.changePassed(1);
            case TDT_PASSED_FIXED -> {
                statistics.changePassed(1);
                if (!isMuted) {
                    statistics.changePassedAdded(1);
                }
            }
            case TDT_PASSED_BY_DEPS_FIXED -> {
                statistics.changePassed(1);
                statistics.changePassedByDepsAdded(1);
            }
            case TDT_FAILED -> {
                statistics.changeFailed(1);
                if (isStrongMode) {
                    statistics.changeFailedInStrongMode(1);
                }
            }
            case TDT_FAILED_BROKEN, TDT_FAILED_NEW -> {
                statistics.changeFailed(1);
                statistics.changeFailedAdded(1);
                if (isStrongMode) {
                    statistics.changeFailedInStrongMode(1);
                }
            }
            case TDT_FAILED_BY_DEPS -> {
                statistics.changeFailedByDeps(1);
            }
            case TDT_FAILED_BY_DEPS_BROKEN, TDT_FAILED_BY_DEPS_NEW -> {
                statistics.changeFailedByDeps(1);
                statistics.changeFailedByDepsAdded(1);
                if (isStrongMode) {
                    statistics.changeFailedInStrongMode(1);
                }
            }
            case TDT_SKIPPED -> {
                statistics.changeSkipped(1);
                return statistics.toImmutable(); // Not counted in total.
            }
            case TDT_SKIPPED_NEW -> {
                statistics.changeSkipped(1);
                statistics.changeSkippedAdded(1);
                return statistics.toImmutable(); // Not counted in total.
            }
            case TDT_DELETED -> {
            }
            case TDT_TIMEOUT_FIXED, TDT_FLAKY_FIXED, TDT_EXTERNAL_FIXED, TDT_INTERNAL_FIXED -> {
                statistics.changePassed(1);
                if (isStrongMode || isBuild) {
                    statistics.changePassedAdded(1);
                }
            }
            case TDT_TIMEOUT_BROKEN, TDT_TIMEOUT_NEW, TDT_FLAKY_BROKEN, TDT_FLAKY_NEW, TDT_EXTERNAL_BROKEN,
                    TDT_EXTERNAL_NEW, TDT_INTERNAL_BROKEN, TDT_INTERNAL_NEW -> {
                if (isStrongMode || isBuild) {
                    statistics.changeFailed(1);
                    statistics.changeFailedAdded(1);
                    if (isStrongMode) {
                        statistics.changeFailedInStrongMode(1);
                    }
                }
            }
            case TDT_TIMEOUT_FAILED, TDT_FLAKY_FAILED, TDT_EXTERNAL_FAILED, TDT_INTERNAL_FAILED -> {
                if (isStrongMode || isBuild) {
                    statistics.changeFailed(1);
                    if (isStrongMode) {
                        statistics.changeFailedInStrongMode(1);
                    }
                }
            }
            case UNRECOGNIZED -> {
                throw new RuntimeException("Unrecognized diff type " + diffType);
            }
            case TDT_SUITE_PROBLEMS -> {
                // skipping
            }
            default -> throw new RuntimeException("Unsupported diff type " + diffType);
        }

        return statistics.toImmutable();
    }
}
