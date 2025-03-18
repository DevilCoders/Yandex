package ru.yandex.ci.storage.api.check;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.check.TaskResultComparer;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;

@Slf4j
@RequiredArgsConstructor
public class CheckComparer {
    private final CiStorageDb db;

    public LargeTestCheckResult compare(
            LargeTaskEntity.Id left,
            LargeTaskEntity.Id right,
            Predicate<Common.TestDiffType> diffTypesFilter
    ) {
        return db.currentOrReadOnly(() -> compareInTx(left, right, diffTypesFilter));
    }

    private LargeTestCheckResult compareInTx(
            LargeTaskEntity.Id left,
            LargeTaskEntity.Id right,
            Predicate<Common.TestDiffType> diffTypesFilter
    ) {
        log.info("Comparing two large tasks: {} and {}", left, right);

        var leftTask = db.largeTasks().get(left);
        var leftState = getTaskState(leftTask, "Left");
        if (leftState == TaskState.NOT_READY) {
            return LargeTestCheckResult.notReady();
        }

        var rightTask = db.largeTasks().get(right);
        var rightState = getTaskState(rightTask, "Right");
        if (rightState == TaskState.NOT_READY) {
            return LargeTestCheckResult.notReady();
        }

        if (leftState == TaskState.CANCELED || rightState == TaskState.CANCELED) {
            return LargeTestCheckResult.canceled(leftState == TaskState.CANCELED, rightState == TaskState.CANCELED);
        }

        var diffs = compareInTx(
                left.getIterationId(),
                right.getIterationId(),
                leftTask.getLeftLargeTestInfo().getSuiteHid().longValue(),
                rightTask.getLeftLargeTestInfo().getSuiteHid().longValue(),
                diffTypesFilter
        );

        return LargeTestCheckResult.ready(diffs);
    }

    private TaskState getTaskState(LargeTaskEntity task, String name) {
        // Yes, we always get the right task for checking
        var checkTaskId = task.toRightTaskId();
        log.info("{} task: {}", name, checkTaskId);

        var checkTask = db.checkTasks().get(checkTaskId);
        var status = checkTask.getStatus();

        if (CheckStatusUtils.isCancelled(status)) {
            log.info("{} check task is canceled: {}", name, status);
            return TaskState.CANCELED;
        } else if (CheckStatusUtils.isCompleted(status)) {
            return TaskState.READY;
        } else {
            log.info("{} check task is not ready: {}", name, status);
            return TaskState.NOT_READY;
        }
    }

    private List<TestDiffByHashEntity> compareInTx(
            CheckIterationEntity.Id leftId,
            CheckIterationEntity.Id rightId,
            long leftSuiteId,
            long rightSuiteId,
            Predicate<Common.TestDiffType> diffTypesFilter
    ) {
        log.info("Comparing check iterations: {}/{} and {}/{}, filter: {}",
                leftId, leftSuiteId, rightId, rightSuiteId, diffTypesFilter);

        var leftSuite = db.testDiffsBySuite().findSuite(leftId, leftSuiteId);
        var rightSuite = db.testDiffsBySuite().findSuite(rightId, rightSuiteId);

        var leftTests = db.testDiffsByHash().getSuite(
                new ChunkAggregateEntity.Id(leftSuite.getId().getIterationId(), leftSuite.getChunkId()),
                leftSuiteId,
                TestEntity.ALL_TOOLCHAINS
        ).stream().collect(Collectors.toMap(x -> x.getId().getTestId().getId(), Function.identity()));

        var rightTests = db.testDiffsByHash().getSuite(
                new ChunkAggregateEntity.Id(rightSuite.getId().getIterationId(), rightSuite.getChunkId()),
                rightSuiteId,
                TestEntity.ALL_TOOLCHAINS
        ).stream().collect(Collectors.toMap(x -> x.getId().getTestId().getId(), Function.identity()));

        var result = new ArrayList<TestDiffByHashEntity>();
        for (var right : rightTests.values()) {
            var left = leftTests.get(right.getId().getTestId().getId());
            var leftStatus = left == null ? Common.TestStatus.TS_NONE : left.getRight();
            if (leftStatus != right.getRight()) {
                var diffType = TaskResultComparer.compare(leftStatus, right.getRight(), right.isExternal());
                if (diffTypesFilter.test(diffType)) {
                    result.add(
                            right.toBuilder()
                                    .left(leftStatus)
                                    .diffType(diffType)
                                    .build()
                    );
                }
            }
        }

        for (var left : leftTests.values()) {
            var right = rightTests.get(left.getId().getTestId().getId());
            if (right != null) {
                continue;
            }
            var diffType = TaskResultComparer.compare(left.getRight(), Common.TestStatus.TS_NONE, left.isExternal());
            if (diffTypesFilter.test(diffType)) {
                result.add(
                        left.toBuilder()
                                .left(left.getRight())
                                .diffType(diffType)
                                .build()
                );
            }
        }

        return result;
    }

    @Value
    public static class LargeTestCheckResult {
        boolean compareReady;
        boolean canceledLeft;
        boolean canceledRight;
        @Nonnull
        List<TestDiffByHashEntity> diffs;

        public static LargeTestCheckResult notReady() {
            return new LargeTestCheckResult(false, false, false, List.of());
        }

        public static LargeTestCheckResult ready(List<TestDiffByHashEntity> diffs) {
            return new LargeTestCheckResult(true, false, false, diffs);
        }

        public static LargeTestCheckResult canceled(boolean canceledLeft, boolean canceledRight) {
            return new LargeTestCheckResult(true, canceledLeft, canceledRight, List.of());
        }
    }

    enum TaskState {
        READY, NOT_READY, CANCELED
    }
}
