package ru.yandex.ci.observer.core.utils;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationNumberStagesAggregation;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.core.db.model.traces.DurationStages;
import ru.yandex.ci.observer.core.db.model.traces.TimestampedTraceStages;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;

class StageAggregationUtilsTest {
    private static final Instant CREATED = Instant.parse("2021-09-24T07:57:08.964949Z");
    private static final Map<String, List<Instant>> SINGLE_PARTITION_TRACES = new HashMap<>();
    private static final Map<String, List<Instant>> MULTIPLE_PARTITIONS_TRACES = new HashMap<>();

    static {
        SINGLE_PARTITION_TRACES.put(
                "storage/check_task_created", List.of(Instant.parse("2021-09-24T07:57:16.978266Z"))
        );
        SINGLE_PARTITION_TRACES.put("distbuild/started", List.of(Instant.parse("2021-09-24T07:57:30.000000Z")));
        SINGLE_PARTITION_TRACES.put("distbuild/graph/started", List.of(Instant.parse("2021-09-24T07:57:31.000000Z")));
        SINGLE_PARTITION_TRACES.put("distbuild/graph/finished", List.of(Instant.parse("2021-09-24T08:02:45.000000Z")));
        SINGLE_PARTITION_TRACES.put("distbuild/main/started", List.of(Instant.parse("2021-09-24T08:02:47.000000Z")));
        SINGLE_PARTITION_TRACES.put("distbuild/main/queued", List.of(Instant.parse("2021-09-24T08:02:51.000000Z")));
        SINGLE_PARTITION_TRACES.put(
                "distbuild/main/build_started", List.of(Instant.parse("2021-09-24T08:03:11.000000Z"))
        );
        SINGLE_PARTITION_TRACES.put(
                "distbuild/main/build_finished", List.of(Instant.parse("2021-09-24T08:14:32.000000Z"))
        );
        SINGLE_PARTITION_TRACES.put("distbuild/main/finished", List.of(Instant.parse("2021-09-24T08:14:36.000000Z")));
        SINGLE_PARTITION_TRACES.put(
                "storage/check_task_finished", List.of(Instant.parse("2021-09-24T08:14:36.666065Z"))
        );
        SINGLE_PARTITION_TRACES.put("distbuild/finished", List.of(Instant.parse("2021-09-24T08:14:38.000000Z")));

        MULTIPLE_PARTITIONS_TRACES.put(
                "storage/check_task_created", List.of(Instant.parse("2021-09-24T07:57:18.593049Z"))
        );
        MULTIPLE_PARTITIONS_TRACES.put(
                "distbuild/graph/started",
                List.of(Instant.parse("2021-09-24T07:57:34.000000Z"), Instant.parse("2021-09-24T07:57:35.000000Z"))
        );
        MULTIPLE_PARTITIONS_TRACES.put(
                "distbuild/started",
                List.of(Instant.parse("2021-09-24T07:57:34.000000Z"), Instant.parse("2021-09-24T07:57:35.000000Z"))
        );
        MULTIPLE_PARTITIONS_TRACES.put(
                "distbuild/graph/finished",
                List.of(Instant.parse("2021-09-24T08:09:52.000000Z"), Instant.parse("2021-09-24T08:14:52.000000Z"))
        );
        MULTIPLE_PARTITIONS_TRACES.put(
                "distbuild/main/started",
                List.of(Instant.parse("2021-09-24T08:11:25.000000Z"), Instant.parse("2021-09-24T08:16:10.000000Z"))
        );
        MULTIPLE_PARTITIONS_TRACES.put(
                "distbuild/main/queued",
                List.of(Instant.parse("2021-09-24T08:14:05.000000Z"), Instant.parse("2021-09-24T08:18:08.000000Z"))
        );
        MULTIPLE_PARTITIONS_TRACES.put(
                "distbuild/main/build_started",
                List.of(Instant.parse("2021-09-24T08:14:25.000000Z"), Instant.parse("2021-09-24T08:18:28.000000Z"))
        );
        MULTIPLE_PARTITIONS_TRACES.put(
                "distbuild/main/build_finished",
                List.of(Instant.parse("2021-09-24T08:41:23.000000Z"), Instant.parse("2021-09-24T08:52:57.000000Z"))
        );
        MULTIPLE_PARTITIONS_TRACES.put(
                "distbuild/main/finished",
                List.of(Instant.parse("2021-09-24T08:44:36.000000Z"), Instant.parse("2021-09-24T08:58:38.000000Z"))
        );
        MULTIPLE_PARTITIONS_TRACES.put(
                "distbuild/finished",
                List.of(Instant.parse("2021-09-24T08:45:19.000000Z"), Instant.parse("2021-09-24T09:00:06.000000Z"))
        );
        MULTIPLE_PARTITIONS_TRACES.put(
                "storage/check_task_finished", List.of(Instant.parse("2021-09-24T08:58:21.675166Z"))
        );

    }

    private static final CheckIterationEntity SAMPLE_ITERATION_PARENT = CheckIterationEntity.builder()
            .id(new CheckIterationEntity.Id(CheckEntity.Id.of(1L), CheckIteration.IterationType.FULL, 1))
            .created(Instant.parse("2021-10-20T12:00:00Z"))
            .finish(Instant.parse("2021-10-20T13:00:00Z"))
            .checkType(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
            .author("anmakon")
            .status(Common.CheckStatus.COMPLETED_FAILED)
            .advisedPool("test_pool")
            .totalDurationSeconds(3600L)
            .build();

    private static final CheckIterationEntity SAMPLE_ITERATION_RECHECK = CheckIterationEntity.builder()
            .id(new CheckIterationEntity.Id(CheckEntity.Id.of(1L), CheckIteration.IterationType.FULL, 2))
            .created(Instant.parse("2021-10-20T13:00:00Z"))
            .checkType(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
            .author("anmakon")
            .status(Common.CheckStatus.CREATED)
            .advisedPool("test_pool")
            .parentIterationNumber(1)
            .build();

    @Test
    void testTracesToTmStage() {
        var stages = StageAggregationUtils.tracesToTmStage(
                CREATED, 1, SINGLE_PARTITION_TRACES,
                StageAggregationUtils.TRACE_TO_STAGE_STARTS,
                StageAggregationUtils.TRACE_TO_STAGE_FINISHES
        );

        Assertions.assertEquals(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, CREATED,
                        StageAggregationUtils.SANDBOX_STAGE, Instant.parse("2021-09-24T07:57:16.978266Z"),
                        StageAggregationUtils.CONFIGURE_STAGE, Instant.parse("2021-09-24T07:57:30.000000Z"),
                        StageAggregationUtils.GRAPH_STAGE, Instant.parse("2021-09-24T07:57:31.000000Z"),
                        StageAggregationUtils.BUILD_STAGE, Instant.parse("2021-09-24T08:02:47.000000Z"),
                        StageAggregationUtils.RESULTS_STAGE, Instant.parse("2021-09-24T07:57:30.000000Z")
                ),
                stages.getStagesStarts());

        Assertions.assertEquals(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, Instant.parse("2021-09-24T07:57:16.978266Z"),
                        StageAggregationUtils.SANDBOX_STAGE, Instant.parse("2021-09-24T08:14:38.000000Z"),
                        StageAggregationUtils.CONFIGURE_STAGE, Instant.parse("2021-09-24T08:02:47.000000Z"),
                        StageAggregationUtils.GRAPH_STAGE, Instant.parse("2021-09-24T08:02:45.000000Z"),
                        StageAggregationUtils.BUILD_STAGE, Instant.parse("2021-09-24T08:14:36.000000Z"),
                        StageAggregationUtils.RESULTS_STAGE, Instant.parse("2021-09-24T08:14:36.666065Z")
                ),
                stages.getStagesFinishes());
    }

    @Test
    void testTracesToTmStageFlat() {
        var stages = StageAggregationUtils.tracesToTmStage(
                CREATED, 1, SINGLE_PARTITION_TRACES,
                StageAggregationUtils.TRACE_TO_STAGE_STARTS_FLAT,
                StageAggregationUtils.TRACE_TO_STAGE_FINISHES_FLAT
        );

        Assertions.assertEquals(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, CREATED,
                        StageAggregationUtils.SANDBOX_STAGE, Instant.parse("2021-09-24T07:57:16.978266Z"),
                        StageAggregationUtils.CONFIGURE_STAGE, Instant.parse("2021-09-24T07:57:30.000000Z"),
                        StageAggregationUtils.BUILD_STAGE, Instant.parse("2021-09-24T08:02:47.000000Z"),
                        StageAggregationUtils.RESULTS_STAGE, Instant.parse("2021-09-24T08:14:36.000000Z")
                ),
                stages.getStagesStarts());

        Assertions.assertEquals(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, Instant.parse("2021-09-24T07:57:16.978266Z"),
                        StageAggregationUtils.SANDBOX_STAGE, Instant.parse("2021-09-24T07:57:30.000000Z"),
                        StageAggregationUtils.CONFIGURE_STAGE, Instant.parse("2021-09-24T08:02:47.000000Z"),
                        StageAggregationUtils.BUILD_STAGE, Instant.parse("2021-09-24T08:14:36.000000Z"),
                        StageAggregationUtils.RESULTS_STAGE, Instant.parse("2021-09-24T08:14:36.666065Z")
                ),
                stages.getStagesFinishes());
    }

    @Test
    void testTracesToTmStageFlatMultiplePartitions() {
        var stages = StageAggregationUtils.tracesToTmStage(
                CREATED, 2, MULTIPLE_PARTITIONS_TRACES,
                StageAggregationUtils.TRACE_TO_STAGE_STARTS_FLAT,
                StageAggregationUtils.TRACE_TO_STAGE_FINISHES_FLAT
        );

        Assertions.assertEquals(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, CREATED,
                        StageAggregationUtils.SANDBOX_STAGE, Instant.parse("2021-09-24T07:57:18.593049Z"),
                        StageAggregationUtils.CONFIGURE_STAGE, Instant.parse("2021-09-24T07:57:34.000000Z"),
                        StageAggregationUtils.BUILD_STAGE, Instant.parse("2021-09-24T08:11:25.000000Z"),
                        StageAggregationUtils.RESULTS_STAGE, Instant.parse("2021-09-24T08:44:36.000000Z")
                ),
                stages.getStagesStarts());

        Assertions.assertEquals(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, Instant.parse("2021-09-24T07:57:18.593049Z"),
                        StageAggregationUtils.SANDBOX_STAGE, Instant.parse("2021-09-24T07:57:35.000000Z"),
                        StageAggregationUtils.CONFIGURE_STAGE, Instant.parse("2021-09-24T08:16:10.000000Z"),
                        StageAggregationUtils.BUILD_STAGE, Instant.parse("2021-09-24T08:58:38.000000Z"),
                        StageAggregationUtils.RESULTS_STAGE, Instant.parse("2021-09-24T08:58:21.675166Z")
                ),
                stages.getStagesFinishes());
    }

    @Test
    void testCheckTaskTmStagesToIterationTmStages() {
        var first = StageAggregationUtils.tracesToTmStage(
                CREATED, 1, SINGLE_PARTITION_TRACES,
                StageAggregationUtils.TRACE_TO_STAGE_STARTS_FLAT,
                StageAggregationUtils.TRACE_TO_STAGE_FINISHES_FLAT
        );
        var second = StageAggregationUtils.tracesToTmStage(
                CREATED, 2, MULTIPLE_PARTITIONS_TRACES,
                StageAggregationUtils.TRACE_TO_STAGE_STARTS_FLAT,
                StageAggregationUtils.TRACE_TO_STAGE_FINISHES_FLAT
        );
        var expectedTasks = List.of(
                new CheckIterationEntity.CheckTaskKey("first", true),
                new CheckIterationEntity.CheckTaskKey("second", true)
        );
        var taskId = new CheckTaskEntity.Id(
                new CheckIterationEntity.Id(CheckEntity.Id.of(1L), CheckIteration.IterationType.FULL, 1),
                "taskId"
        );

        var iterationStages = StageAggregationUtils.checkTaskTmStagesToIterationTmStages(
                CREATED,
                expectedTasks,
                Map.of(
                        expectedTasks.get(0),
                        CheckTaskEntity.builder()
                                .id(taskId)
                                .checkType(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
                                .jobName("first")
                                .created(CREATED)
                                .rightRevisionTimestamp(CREATED)
                                .timestampedStagesAggregation(first)
                                .build(),
                        expectedTasks.get(1),
                        CheckTaskEntity.builder()
                                .id(taskId)
                                .checkType(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
                                .jobName("second")
                                .created(CREATED)
                                .rightRevisionTimestamp(CREATED)
                                .timestampedStagesAggregation(second)
                                .build()
                ),
                Instant.parse("2021-09-24T08:58:21.675166Z")
        );

        Assertions.assertEquals(
                Map.of(StageAggregationUtils.CREATION_STAGE, CREATED), iterationStages.getStagesStarts()
        );

        Assertions.assertEquals(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, Instant.parse("2021-09-24T07:57:18.593049Z"),
                        StageAggregationUtils.SANDBOX_STAGE, Instant.parse("2021-09-24T07:57:35.000000Z"),
                        StageAggregationUtils.CONFIGURE_STAGE, Instant.parse("2021-09-24T08:16:10.000000Z"),
                        StageAggregationUtils.BUILD_STAGE, Instant.parse("2021-09-24T08:58:38.000000Z"),
                        StageAggregationUtils.RESULTS_STAGE, Instant.parse("2021-09-24T08:58:21.675166Z")
                ),
                iterationStages.getStagesFinishes());
    }

    @Test
    void testIterationTmStagesToTraceStages() {
        var first = StageAggregationUtils.tracesToTmStage(
                CREATED, 1, SINGLE_PARTITION_TRACES,
                StageAggregationUtils.TRACE_TO_STAGE_STARTS_FLAT,
                StageAggregationUtils.TRACE_TO_STAGE_FINISHES_FLAT
        );
        var second = StageAggregationUtils.tracesToTmStage(
                CREATED, 2, MULTIPLE_PARTITIONS_TRACES,
                StageAggregationUtils.TRACE_TO_STAGE_STARTS_FLAT,
                StageAggregationUtils.TRACE_TO_STAGE_FINISHES_FLAT
        );
        var expectedTasks = List.of(
                new CheckIterationEntity.CheckTaskKey("first", true),
                new CheckIterationEntity.CheckTaskKey("second", true)
        );
        var taskId = new CheckTaskEntity.Id(
                new CheckIterationEntity.Id(CheckEntity.Id.of(1L), CheckIteration.IterationType.FULL, 1),
                "taskId"
        );
        var iterationTmStages = StageAggregationUtils.checkTaskTmStagesToIterationTmStages(
                CREATED,
                expectedTasks,
                Map.of(
                        expectedTasks.get(0),
                        CheckTaskEntity.builder()
                                .id(taskId)
                                .checkType(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
                                .jobName("first")
                                .created(CREATED)
                                .rightRevisionTimestamp(CREATED)
                                .timestampedStagesAggregation(first)
                                .build(),
                        expectedTasks.get(1),
                        CheckTaskEntity.builder()
                                .id(taskId)
                                .checkType(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
                                .jobName("second")
                                .created(CREATED)
                                .rightRevisionTimestamp(CREATED)
                                .timestampedStagesAggregation(second)
                                .build()
                ), null
        );
        StageAggregationUtils.addPreCreationTmStage(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT, CREATED, CREATED,
                CREATED, iterationTmStages);
        var expectedDirationStages = new DurationStages(true);
        expectedDirationStages.putStageDuration(StageAggregationUtils.CREATION_STAGE, 10);
        expectedDirationStages.putStageDuration(StageAggregationUtils.SANDBOX_STAGE, 17);
        expectedDirationStages.putStageDuration(StageAggregationUtils.CONFIGURE_STAGE, 1115);
        expectedDirationStages.putStageDuration(StageAggregationUtils.BUILD_STAGE, 2548);

        var iterationDurationStages = StageAggregationUtils.iterationTmStagesToTraceStages(iterationTmStages);

        Assertions.assertEquals(expectedDirationStages, iterationDurationStages);
    }

    @Test
    void testIterationTmStagesToTraceStages_whenIterationFinished() {
        var first = StageAggregationUtils.tracesToTmStage(
                CREATED, 1, SINGLE_PARTITION_TRACES,
                StageAggregationUtils.TRACE_TO_STAGE_STARTS_FLAT,
                StageAggregationUtils.TRACE_TO_STAGE_FINISHES_FLAT
        );
        var second = StageAggregationUtils.tracesToTmStage(
                CREATED, 2, MULTIPLE_PARTITIONS_TRACES,
                StageAggregationUtils.TRACE_TO_STAGE_STARTS_FLAT,
                StageAggregationUtils.TRACE_TO_STAGE_FINISHES_FLAT
        );
        var expectedTasks = List.of(
                new CheckIterationEntity.CheckTaskKey("first", true),
                new CheckIterationEntity.CheckTaskKey("second", true)
        );
        var taskId = new CheckTaskEntity.Id(
                new CheckIterationEntity.Id(CheckEntity.Id.of(1L), CheckIteration.IterationType.FULL, 1),
                "taskId"
        );
        var iterationTmStages = StageAggregationUtils.checkTaskTmStagesToIterationTmStages(
                CREATED,
                expectedTasks,
                Map.of(
                        expectedTasks.get(0),
                        CheckTaskEntity.builder()
                                .id(taskId)
                                .checkType(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
                                .jobName("first")
                                .created(CREATED)
                                .rightRevisionTimestamp(CREATED)
                                .timestampedStagesAggregation(first)
                                .build(),
                        expectedTasks.get(1),
                        CheckTaskEntity.builder()
                                .id(taskId)
                                .checkType(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
                                .jobName("second")
                                .created(CREATED)
                                .rightRevisionTimestamp(CREATED)
                                .timestampedStagesAggregation(second)
                                .build()
                ), Instant.parse("2021-09-24T08:58:21.675166Z")
        );
        StageAggregationUtils.addPreCreationTmStage(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT, CREATED, CREATED,
                CREATED, iterationTmStages);
        var expectedDirationStages = new DurationStages(true);
        expectedDirationStages.putStageDuration(StageAggregationUtils.CREATION_STAGE, 10);
        expectedDirationStages.putStageDuration(StageAggregationUtils.SANDBOX_STAGE, 17);
        expectedDirationStages.putStageDuration(StageAggregationUtils.CONFIGURE_STAGE, 1115);
        expectedDirationStages.putStageDuration(StageAggregationUtils.BUILD_STAGE, 2548);
        expectedDirationStages.putStageDuration(StageAggregationUtils.RESULTS_STAGE, 0);

        var iterationDurationStages = StageAggregationUtils.iterationTmStagesToTraceStages(iterationTmStages);

        Assertions.assertEquals(expectedDirationStages, iterationDurationStages);
    }

    @Test
    void testUpdateParentIterationOnRecheck_WhenRecheckFirstUpdated() {
        var recheckNumber = SAMPLE_ITERATION_RECHECK.getId().getNumber();
        var expectedParentIteration = SAMPLE_ITERATION_PARENT.toBuilder()
                .hasUnfinishedRechecks(true)
                .recheckAggregationsByNumber(Map.of(
                        recheckNumber, CheckIterationNumberStagesAggregation.of(SAMPLE_ITERATION_RECHECK)
                ))
                .unfinishedRecheckIterationNumbers(Set.of(recheckNumber))
                .build();

        var updatedParentIteration = StageAggregationUtils.updateParentIterationOnRecheck(
                SAMPLE_ITERATION_PARENT, SAMPLE_ITERATION_RECHECK
        );

        Assertions.assertNotSame(SAMPLE_ITERATION_PARENT, updatedParentIteration);
        Assertions.assertEquals(expectedParentIteration, updatedParentIteration);
    }

    @Test
    void testUpdateParentIterationOnRecheck_WhenRecheckNextUpdated() {
        var recheckNumber = SAMPLE_ITERATION_RECHECK.getId().getNumber();
        var parentIteration = SAMPLE_ITERATION_PARENT.toBuilder()
                .hasUnfinishedRechecks(true)
                .recheckAggregationsByNumber(Map.of(
                        recheckNumber, CheckIterationNumberStagesAggregation.of(SAMPLE_ITERATION_RECHECK)
                ))
                .unfinishedRecheckIterationNumbers(Set.of(recheckNumber))
                .build();

        var updatedParentIteration = StageAggregationUtils.updateParentIterationOnRecheck(
                parentIteration, SAMPLE_ITERATION_RECHECK
        );

        Assertions.assertNotSame(parentIteration, updatedParentIteration);
        Assertions.assertEquals(parentIteration, updatedParentIteration);
    }

    @Test
    void testUpdateParentIterationOnRecheck_WhenRecheckFinished() {
        var recheckNumber = SAMPLE_ITERATION_RECHECK.getId().getNumber();
        var parentIteration = SAMPLE_ITERATION_PARENT.toBuilder()
                .hasUnfinishedRechecks(true)
                .recheckAggregationsByNumber(Map.of(
                        recheckNumber, CheckIterationNumberStagesAggregation.of(SAMPLE_ITERATION_RECHECK)
                ))
                .unfinishedRecheckIterationNumbers(Set.of(recheckNumber))
                .build();
        var recheckIteration = SAMPLE_ITERATION_RECHECK.toBuilder()
                .status(Common.CheckStatus.COMPLETED_SUCCESS)
                .finish(Instant.parse("2021-10-20T14:00:00Z"))
                .build();
        var expectedParentIteration = SAMPLE_ITERATION_PARENT.toBuilder()
                .totalDurationSeconds(7200L)
                .finalStatus(Common.CheckStatus.COMPLETED_SUCCESS)
                .recheckAggregationsByNumber(Map.of(
                        recheckNumber, CheckIterationNumberStagesAggregation.of(recheckIteration)
                ))
                .build();

        var updatedParentIteration = StageAggregationUtils.updateParentIterationOnRecheck(
                parentIteration, recheckIteration
        );

        Assertions.assertNotSame(parentIteration, updatedParentIteration);
        Assertions.assertEquals(expectedParentIteration, updatedParentIteration);
    }

    @Test
    void testUpdateParentIterationOnRecheck_WhenRecheckFinished_ParentIterationNoFinishTimestamp() {
        var recheckNumber = SAMPLE_ITERATION_RECHECK.getId().getNumber();
        var parentIteration = SAMPLE_ITERATION_PARENT.toBuilder()
                .finish(null)
                .hasUnfinishedRechecks(true)
                .recheckAggregationsByNumber(Map.of(
                        recheckNumber, CheckIterationNumberStagesAggregation.of(SAMPLE_ITERATION_RECHECK)
                ))
                .unfinishedRecheckIterationNumbers(Set.of(recheckNumber))
                .build();
        var recheckIteration = SAMPLE_ITERATION_RECHECK.toBuilder()
                .status(Common.CheckStatus.COMPLETED_SUCCESS)
                .finish(Instant.parse("2021-10-20T14:00:00Z"))
                .build();
        var expectedParentIteration = SAMPLE_ITERATION_PARENT.toBuilder()
                .finish(null)
                .totalDurationSeconds(7200L)
                .finalStatus(Common.CheckStatus.COMPLETED_SUCCESS)
                .recheckAggregationsByNumber(Map.of(
                        recheckNumber, CheckIterationNumberStagesAggregation.of(recheckIteration)
                ))
                .build();

        var updatedParentIteration = StageAggregationUtils.updateParentIterationOnRecheck(
                parentIteration, recheckIteration
        );

        Assertions.assertNotSame(parentIteration, updatedParentIteration);
        Assertions.assertEquals(expectedParentIteration, updatedParentIteration);
    }

    @Test
    void addPreCreationTmStage_whenTrunkPostCommit() {
        var actual = new TimestampedTraceStages();
        StageAggregationUtils.addPreCreationTmStage(
                CheckOuterClass.CheckType.TRUNK_POST_COMMIT, CREATED, null, CREATED.minus(1, ChronoUnit.DAYS),
                actual
        );

        var expected = new TimestampedTraceStages();
        expected.putStageStart(StageAggregationUtils.PRE_CREATION_STAGE, CREATED.minus(1, ChronoUnit.DAYS));
        expected.putStageFinish(StageAggregationUtils.PRE_CREATION_STAGE, CREATED);

        Assertions.assertEquals(expected, actual);
    }

    @Test
    void addPreCreationTmStage_whenTrunkPreCommit() {
        var actual = new TimestampedTraceStages();
        StageAggregationUtils.addPreCreationTmStage(
                CheckOuterClass.CheckType.TRUNK_PRE_COMMIT,
                CREATED,
                CREATED.minus(1, ChronoUnit.DAYS),
                CREATED.minus(20, ChronoUnit.DAYS),
                actual
        );

        var expected = new TimestampedTraceStages();
        expected.putStageStart(StageAggregationUtils.PRE_CREATION_STAGE, CREATED.minus(1, ChronoUnit.DAYS));
        expected.putStageFinish(StageAggregationUtils.PRE_CREATION_STAGE, CREATED);

        Assertions.assertEquals(expected, actual);
    }
}
