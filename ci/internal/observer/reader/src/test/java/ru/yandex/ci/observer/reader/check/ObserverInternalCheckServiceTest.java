package ru.yandex.ci.observer.reader.check;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationNumberStagesAggregation;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.core.db.model.traces.CheckTaskPartitionTraceEntity;
import ru.yandex.ci.observer.core.db.model.traces.DurationStages;
import ru.yandex.ci.observer.core.db.model.traces.TimestampedTraceStages;
import ru.yandex.ci.observer.core.utils.StageAggregationUtils;
import ru.yandex.ci.observer.reader.ObserverReaderYdbTestBase;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;

public class ObserverInternalCheckServiceTest extends ObserverReaderYdbTestBase {
    private ObserverInternalCheckService checkService;

    @BeforeEach
    public void setup() {
        this.checkService = new ObserverInternalCheckService(cache, db);
    }

    @Test
    void processCheckTaskFatalError() {
        db.tx(() -> {
            db.checks().save(SAMPLE_CHECK);
            db.iterations().save(SAMPLE_ITERATION);
            db.tasks().save(SAMPLE_TASK, SAMPLE_TASK_2);
        });
        var finishTime = TIME.plusSeconds(100);
        var expectedCheck = SAMPLE_CHECK.toBuilder()
                .status(Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR)
                .completed(finishTime)
                .build();
        var expectedIteration = SAMPLE_ITERATION.toBuilder()
                .status(Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR)
                .finish(finishTime)
                .build();
        var expectedTask = SAMPLE_TASK.toBuilder()
                .status(Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR)
                .finish(finishTime)
                .build();
        var expectedTask2 = SAMPLE_TASK_2.toBuilder()
                .status(Common.CheckStatus.CANCELLED)
                .finish(finishTime)
                .build();

        checkService.processCheckTaskFatalError(SAMPLE_TASK_ID, finishTime);

        var updatedCheck = cache.checks().getOrThrow(SAMPLE_CHECK_ID);
        var updatedIteration = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID);
        var updatedTask = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID);
        var updatedTask2 = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID_2);

        Assertions.assertEquals(expectedCheck, updatedCheck);
        Assertions.assertEquals(expectedIteration, updatedIteration);
        Assertions.assertEquals(expectedTask, updatedTask);
        Assertions.assertEquals(expectedTask2, updatedTask2);
    }

    @Test
    void processPartitionFinished() {
        db.tx(() -> db.tasks().save(SAMPLE_TASK));
        var expectedTask = SAMPLE_TASK.toBuilder()
                .completedPartitions(Set.of(0, 3, 5))
                .build();

        checkService.processPartitionFinished(SAMPLE_TASK_ID, List.of(0));
        checkService.processPartitionFinished(SAMPLE_TASK_ID, List.of(3, 5));

        var updatedTask = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID);

        Assertions.assertEquals(expectedTask, updatedTask);
    }

    @Test
    void processPartitionFinished_WhenPartitionFinishDuplicates() {
        db.tx(() -> db.tasks().save(SAMPLE_TASK));
        var expectedTask = SAMPLE_TASK.toBuilder()
                .completedPartitions(Set.of(0, 3))
                .build();

        checkService.processPartitionFinished(SAMPLE_TASK_ID, List.of(0, 3));
        checkService.processPartitionFinished(SAMPLE_TASK_ID, List.of(3));

        var updatedTask = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID);

        Assertions.assertEquals(expectedTask, updatedTask);
    }

    @Test
    void processCheckTaskCancel() {
        db.tx(() -> {
            db.checks().save(SAMPLE_CHECK);
            db.iterations().save(SAMPLE_ITERATION);
            db.tasks().save(SAMPLE_TASK, SAMPLE_TASK_2);
        });
        var finishTime = TIME.plusSeconds(100);
        var expectedCheck = SAMPLE_CHECK.toBuilder()
                .status(Common.CheckStatus.CANCELLED)
                .completed(finishTime)
                .build();
        var expectedIteration = SAMPLE_ITERATION.toBuilder()
                .status(Common.CheckStatus.CANCELLED)
                .finish(finishTime)
                .build();
        var expectedTask = SAMPLE_TASK.toBuilder()
                .status(Common.CheckStatus.CANCELLED)
                .finish(finishTime)
                .build();
        var expectedTask2 = SAMPLE_TASK_2.toBuilder()
                .status(Common.CheckStatus.CANCELLED)
                .finish(finishTime)
                .build();

        checkService.processCheckTaskCancel(SAMPLE_ITERATION_ID, finishTime);

        var updatedCheck = cache.checks().getOrThrow(SAMPLE_CHECK_ID);
        var updatedIteration = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID);
        var updatedTask = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID);
        var updatedTask2 = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID_2);

        Assertions.assertEquals(expectedCheck, updatedCheck);
        Assertions.assertEquals(expectedIteration, updatedIteration);
        Assertions.assertEquals(expectedTask, updatedTask);
        Assertions.assertEquals(expectedTask2, updatedTask2);
    }

    @Test
    void processCheckTaskCancel_WhenHasActiveOtherIteration() {
        var otherIteration = SAMPLE_ITERATION.toBuilder()
                .id(SAMPLE_ITERATION_ID_2)
                .build();
        db.tx(() -> {
            db.checks().save(SAMPLE_CHECK);
            db.iterations().save(SAMPLE_ITERATION, otherIteration);
            db.tasks().save(SAMPLE_TASK, SAMPLE_TASK_2);
        });
        var finishTime = TIME.plusSeconds(100);
        var expectedIteration = SAMPLE_ITERATION.toBuilder()
                .status(Common.CheckStatus.CANCELLED)
                .finish(finishTime)
                .build();
        var expectedTask = SAMPLE_TASK.toBuilder()
                .status(Common.CheckStatus.CANCELLED)
                .finish(finishTime)
                .build();
        var expectedTask2 = SAMPLE_TASK_2.toBuilder()
                .status(Common.CheckStatus.CANCELLED)
                .finish(finishTime)
                .build();

        checkService.processCheckTaskCancel(SAMPLE_ITERATION_ID, finishTime);

        var updatedCheck = cache.checks().getOrThrow(SAMPLE_CHECK_ID);
        var updatedIteration = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID);
        var otherIterationFromCache = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID_2);
        var updatedTask = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID);
        var updatedTask2 = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID_2);

        Assertions.assertEquals(SAMPLE_CHECK, updatedCheck);
        Assertions.assertEquals(expectedIteration, updatedIteration);
        Assertions.assertEquals(otherIteration, otherIterationFromCache);
        Assertions.assertEquals(expectedTask, updatedTask);
        Assertions.assertEquals(expectedTask2, updatedTask2);
    }

    @Test
    void aggregateTraceStages() {
        var iteration = SAMPLE_ITERATION.toBuilder()
                .expectedCheckTasks(List.of(
                        new CheckIterationEntity.CheckTaskKey(SAMPLE_TASK_2.getJobName(), SAMPLE_TASK_2.isRight())
                ))
                .build();
        db.tx(() -> {
            db.iterations().save(iteration);
            db.tasks().save(SAMPLE_TASK_2);
            db.traces().save(createTraces(
                    SAMPLE_TASK_ID_2,
                    0,
                    Map.of(
                            "distbuild/started",  TIME.plusSeconds(20),
                            "distbuild/main/started", TIME.plusSeconds(30),
                            "distbuild/main/finished", TIME.plusSeconds(40),
                            "distbuild/finished",  TIME.plusSeconds(45)
                    )
            ));
            db.traces().save(createTraces(
                    SAMPLE_TASK_ID_2,
                    CheckTaskPartitionTraceEntity.Id.ALL_PARTITIONS,
                    Map.of(
                            "storage/check_task_created", TIME.plusSeconds(5),
                            "storage/check_task_finished", TIME.plusSeconds(50)
                    )
            ));
        });

        var timestampedStagesAggregation = createTimestampedTraceStages(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME,
                        StageAggregationUtils.SANDBOX_STAGE, TIME.plusSeconds(5),
                        StageAggregationUtils.CONFIGURE_STAGE, TIME.plusSeconds(20),
                        StageAggregationUtils.BUILD_STAGE, TIME.plusSeconds(30),
                        StageAggregationUtils.RESULTS_STAGE, TIME.plusSeconds(40)
                ),
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME.plusSeconds(5),
                        StageAggregationUtils.SANDBOX_STAGE, TIME.plusSeconds(20),
                        StageAggregationUtils.CONFIGURE_STAGE, TIME.plusSeconds(30),
                        StageAggregationUtils.BUILD_STAGE, TIME.plusSeconds(40),
                        StageAggregationUtils.RESULTS_STAGE, TIME.plusSeconds(50)
                )
        );
        var timstampedByPartition = createTimestampedTraceStages(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME,
                        StageAggregationUtils.SANDBOX_STAGE, TIME.plusSeconds(5),
                        StageAggregationUtils.CONFIGURE_STAGE, TIME.plusSeconds(20),
                        StageAggregationUtils.BUILD_STAGE, TIME.plusSeconds(30),
                        StageAggregationUtils.RESULTS_STAGE, TIME.plusSeconds(20)
                ),
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME.plusSeconds(5),
                        StageAggregationUtils.SANDBOX_STAGE, TIME.plusSeconds(45),
                        StageAggregationUtils.CONFIGURE_STAGE, TIME.plusSeconds(30),
                        StageAggregationUtils.BUILD_STAGE, TIME.plusSeconds(40),
                        StageAggregationUtils.RESULTS_STAGE, TIME.plusSeconds(50)
                )
        );
        var expectedTask = SAMPLE_TASK_2.toBuilder()
                .timestampedStagesByPartitions(Map.of(0, timstampedByPartition))
                .timestampedStagesAggregation(timestampedStagesAggregation)
                .finish(TIME.plusSeconds(50))
                .status(Common.CheckStatus.COMPLETED_SUCCESS)
                .build();

        var durationStages = createDurationStages(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, 5L,
                        StageAggregationUtils.SANDBOX_STAGE, 15L,
                        StageAggregationUtils.CONFIGURE_STAGE, 10L,
                        StageAggregationUtils.BUILD_STAGE, 10L
                )
        );
        timestampedStagesAggregation = createTimestampedTraceStages(
                Map.of(
                        StageAggregationUtils.PRE_CREATION_STAGE, TIME,
                        StageAggregationUtils.CREATION_STAGE, TIME
                ),
                Map.of(
                        StageAggregationUtils.PRE_CREATION_STAGE, TIME,
                        StageAggregationUtils.CREATION_STAGE, TIME.plusSeconds(5),
                        StageAggregationUtils.SANDBOX_STAGE, TIME.plusSeconds(20),
                        StageAggregationUtils.CONFIGURE_STAGE, TIME.plusSeconds(30),
                        StageAggregationUtils.BUILD_STAGE, TIME.plusSeconds(40)
                )
        );
        var expectedIteration = iteration.toBuilder()
                .stagesAggregation(durationStages)
                .timestampedStagesAggregation(timestampedStagesAggregation)
                .build();

        checkService.aggregateTraceStages(SAMPLE_ITERATION_ID, Set.of(SAMPLE_TASK_ID_2));

        var updatedTask = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID_2);
        var updatedIteration = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID);

        Assertions.assertEquals(expectedTask, updatedTask);
        Assertions.assertEquals(expectedIteration, updatedIteration);
    }

    @Test
    void aggregateTraceStages_WhenNotAllTasksStagesFinished() {
        var iteration = SAMPLE_ITERATION.toBuilder()
                .expectedCheckTasks(List.of(
                        new CheckIterationEntity.CheckTaskKey(SAMPLE_TASK.getJobName(), SAMPLE_TASK.isRight()),
                        new CheckIterationEntity.CheckTaskKey(SAMPLE_TASK_2.getJobName(), SAMPLE_TASK_2.isRight())
                ))
                .build();
        db.tx(() -> {
            db.iterations().save(iteration);
            db.tasks().save(SAMPLE_TASK, SAMPLE_TASK_2);
            db.traces().save(createTraces(
                    SAMPLE_TASK_ID,
                    CheckTaskPartitionTraceEntity.Id.ALL_PARTITIONS,
                    Map.of("storage/check_task_created", TIME.plusSeconds(10))
            ));
            db.traces().save(createTraces(
                    SAMPLE_TASK_ID_2,
                    0,
                    Map.of("distbuild/started",  TIME.plusSeconds(20), "distbuild/main/started", TIME.plusSeconds(30))
            ));
            db.traces().save(createTraces(
                    SAMPLE_TASK_ID_2,
                    CheckTaskPartitionTraceEntity.Id.ALL_PARTITIONS,
                    Map.of("storage/check_task_created", TIME.plusSeconds(5))
            ));
        });

        var timestampedByPartition = createTimestampedTraceStages(
                Map.of(StageAggregationUtils.CREATION_STAGE, TIME),
                Map.of(StageAggregationUtils.CREATION_STAGE, TIME.plusSeconds(10))
        );
        var expectedTask = SAMPLE_TASK.toBuilder()
                .timestampedStagesAggregation(timestampedByPartition)
                .build();

        timestampedByPartition = createTimestampedTraceStages(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME,
                        StageAggregationUtils.SANDBOX_STAGE, TIME.plusSeconds(5),
                        StageAggregationUtils.CONFIGURE_STAGE, TIME.plusSeconds(20),
                        StageAggregationUtils.BUILD_STAGE, TIME.plusSeconds(30),
                        StageAggregationUtils.RESULTS_STAGE, TIME.plusSeconds(20)
                ),
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME.plusSeconds(5),
                        StageAggregationUtils.CONFIGURE_STAGE, TIME.plusSeconds(30)
                )
        );
        var timestampedStagesAggregation = createTimestampedTraceStages(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME,
                        StageAggregationUtils.SANDBOX_STAGE, TIME.plusSeconds(5),
                        StageAggregationUtils.CONFIGURE_STAGE, TIME.plusSeconds(20),
                        StageAggregationUtils.BUILD_STAGE, TIME.plusSeconds(30)
                ),
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME.plusSeconds(5),
                        StageAggregationUtils.SANDBOX_STAGE, TIME.plusSeconds(20),
                        StageAggregationUtils.CONFIGURE_STAGE, TIME.plusSeconds(30)
                )
        );
        var expectedTask2 = SAMPLE_TASK_2.toBuilder()
                .timestampedStagesByPartitions(Map.of(0, timestampedByPartition))
                .timestampedStagesAggregation(timestampedStagesAggregation)
                .build();

        var durationStages = createDurationStages(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, 10L
                )
        );
        timestampedStagesAggregation = createTimestampedTraceStages(
                Map.of(
                        StageAggregationUtils.PRE_CREATION_STAGE, TIME,
                        StageAggregationUtils.CREATION_STAGE, TIME
                ),
                Map.of(
                        StageAggregationUtils.PRE_CREATION_STAGE, TIME,
                        StageAggregationUtils.CREATION_STAGE, TIME.plusSeconds(10)
                )
        );
        var expectedIteration = iteration.toBuilder()
                .stagesAggregation(durationStages)
                .timestampedStagesAggregation(timestampedStagesAggregation)
                .build();

        checkService.aggregateTraceStages(SAMPLE_ITERATION_ID, Set.of(SAMPLE_TASK_ID, SAMPLE_TASK_ID_2));

        var updatedTask = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID);
        var updatedTask2 = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID_2);
        var updatedIteration = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID);

        Assertions.assertEquals(expectedTask, updatedTask);
        Assertions.assertEquals(expectedTask2, updatedTask2);
        Assertions.assertEquals(expectedIteration, updatedIteration);
    }

    @Test
    void aggregateTraceStagesTask_WhenGraphTraceWithSandboxIdReceived() {
        var iteration = SAMPLE_ITERATION.toBuilder()
                .expectedCheckTasks(List.of(
                        new CheckIterationEntity.CheckTaskKey(SAMPLE_TASK_2.getJobName(), SAMPLE_TASK_2.isRight())
                ))
                .build();
        db.tx(() -> {
            db.iterations().save(iteration);
            db.tasks().save(SAMPLE_TASK_2);
            db.traces().save(createTraces(
                    SAMPLE_TASK_ID_2,
                    CheckTaskPartitionTraceEntity.Id.ALL_PARTITIONS,
                    Map.of("storage/check_task_created", TIME.plusSeconds(10))
            ));
            db.traces().save(createTraces(
                    SAMPLE_TASK_ID_2, 0,
                    Map.of("distbuild/started", TIME.plusSeconds(20))
            ));
            db.traces().save(
                    CheckTaskPartitionTraceEntity.builder()
                            .id(new CheckTaskPartitionTraceEntity.Id(SAMPLE_TASK_ID_2, 0, "distbuild/graph/started"))
                            .time(TIME.plusSeconds(40))
                            .attributes(Map.of("sandbox_task_id", "12345"))
                            .build()
            );
        });

        var timestampedByPartition = createTimestampedTraceStages(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME,
                        StageAggregationUtils.SANDBOX_STAGE, TIME.plusSeconds(10),
                        StageAggregationUtils.CONFIGURE_STAGE, TIME.plusSeconds(20),
                        StageAggregationUtils.GRAPH_STAGE, TIME.plusSeconds(40),
                        StageAggregationUtils.RESULTS_STAGE, TIME.plusSeconds(20)
                ),
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME.plusSeconds(10)
                )
        );
        var timestampedStagesAggregation = createTimestampedTraceStages(
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME,
                        StageAggregationUtils.SANDBOX_STAGE, TIME.plusSeconds(10),
                        StageAggregationUtils.CONFIGURE_STAGE, TIME.plusSeconds(20)
                ),
                Map.of(
                        StageAggregationUtils.CREATION_STAGE, TIME.plusSeconds(10),
                        StageAggregationUtils.SANDBOX_STAGE, TIME.plusSeconds(20)
                )
        );
        var expectedTask = SAMPLE_TASK_2.toBuilder()
                .timestampedStagesByPartitions(Map.of(0, timestampedByPartition))
                .timestampedStagesAggregation(timestampedStagesAggregation)
                .taskAttributes(Map.of("SANDBOX_TASK_URL", "https://sandbox.yandex-team.ru/task/12345/view"))
                .build();

        checkService.aggregateTraceStages(SAMPLE_ITERATION_ID, Set.of(SAMPLE_TASK_ID_2));
        var updatedTask = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID_2);

        Assertions.assertEquals(expectedTask, updatedTask);
    }

    @Test
    void addTechnicalStatistics() {
        var statistics = new TechnicalStatistics(123, 456, 789);
        var iteration = SAMPLE_ITERATION.withStatistics(statistics);
        db.tx(() -> {
            db.iterations().save(iteration);
            db.tasks().save(SAMPLE_TASK);
        });

        var expectedIteration = iteration.withStatistics(iteration.getStatistics().plus(statistics));
        var expectedTask = SAMPLE_TASK.toBuilder()
                .partitionsTechnicalStatistics(Map.of(0, statistics))
                .build();

        checkService.addTechnicalStatistics(SAMPLE_TASK_ID, 0, statistics);

        var updatedIteration = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID);
        var updatedTask = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID);

        Assertions.assertEquals(expectedIteration, updatedIteration);
        Assertions.assertEquals(expectedTask, updatedTask);
    }

    @Test
    void addTechnicalStatisticsDuplicate() {
        var statistics = new TechnicalStatistics(123, 456, 789);
        var secondStatistics = new TechnicalStatistics(234, 345, 567);
        var iteration = SAMPLE_ITERATION.withStatistics(statistics);
        var task = SAMPLE_TASK.toBuilder()
                .partitionsTechnicalStatistics(Map.of(0, statistics))
                .build();
        db.tx(() -> {
            db.iterations().save(iteration);
            db.tasks().save(task);
        });

        var expectedIteration = iteration.withStatistics(secondStatistics.max(statistics));
        var expectedTask = SAMPLE_TASK.toBuilder()
                .partitionsTechnicalStatistics(Map.of(0, secondStatistics.max(statistics)))
                .build();

        checkService.addTechnicalStatistics(SAMPLE_TASK_ID, 0, secondStatistics);

        var updatedIteration = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID);
        var updatedTask = cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID);

        Assertions.assertEquals(expectedIteration, updatedIteration);
        Assertions.assertEquals(expectedTask, updatedTask);
    }

    @Test
    void processPessimize() {
        db.tx(() -> db.iterations().save(SAMPLE_ITERATION));
        var expectedIteration = SAMPLE_ITERATION.withPessimized(true);

        checkService.processPessimize(SAMPLE_ITERATION_ID);

        var iteration = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID);

        Assertions.assertEquals(expectedIteration, iteration);
    }

    @Test
    void processIterationFinish() {
        db.tx(() -> db.iterations().save(SAMPLE_ITERATION));
        var finishStatus = Common.CheckStatus.COMPLETED_FAILED;
        var finishTime = TIME.plusSeconds(200);
        var timestampedStagesAggregation = new TimestampedTraceStages();
        timestampedStagesAggregation.putStageStart(StageAggregationUtils.PRE_CREATION_STAGE, TIME);
        timestampedStagesAggregation.putStageStart(StageAggregationUtils.CREATION_STAGE, TIME);
        timestampedStagesAggregation.putStageFinish(StageAggregationUtils.PRE_CREATION_STAGE, TIME);
        timestampedStagesAggregation.putStageFinish(StageAggregationUtils.RESULTS_STAGE, finishTime);
        var expectedIteration = SAMPLE_ITERATION.toBuilder()
                .status(finishStatus)
                .finish(finishTime)
                .totalDurationSeconds(200L)
                .timestampedStagesAggregation(timestampedStagesAggregation)
                .build();

        checkService.processIterationFinish(SAMPLE_ITERATION_ID, finishStatus, finishTime);

        var updatedIteration = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID);

        Assertions.assertEquals(expectedIteration, updatedIteration);
    }

    @Test
    void processIterationFinish_WhenFinishRecheckIteration() {
        var recheckIterationNumber = SAMPLE_ITERATION_ID_2.getNumber();
        var recheckIteration = SAMPLE_ITERATION.toBuilder()
                .id(SAMPLE_ITERATION_ID_2)
                .created(TIME.plusSeconds(100))
                .parentIterationNumber(SAMPLE_ITERATION_ID.getNumber())
                .build();
        var parentIteration = SAMPLE_ITERATION.toBuilder()
                .hasUnfinishedRechecks(true)
                .finish(TIME.plusSeconds(100))
                .totalDurationSeconds(100L)
                .unfinishedRecheckIterationNumbers(Set.of(recheckIterationNumber))
                .status(Common.CheckStatus.COMPLETED_FAILED)
                .recheckAggregationsByNumber(Map.of(
                        recheckIterationNumber,
                        CheckIterationNumberStagesAggregation.of(recheckIteration)
                ))
                .build();
        db.tx(() -> db.iterations().save(parentIteration, recheckIteration));
        var finishStatus = Common.CheckStatus.COMPLETED_SUCCESS;
        var finishTime = TIME.plusSeconds(200);
        var timestampedStagesAggregation = new TimestampedTraceStages();
        timestampedStagesAggregation.putStageStart(StageAggregationUtils.PRE_CREATION_STAGE, TIME.plusSeconds(100));
        timestampedStagesAggregation.putStageStart(StageAggregationUtils.CREATION_STAGE, TIME.plusSeconds(100));
        timestampedStagesAggregation.putStageFinish(StageAggregationUtils.PRE_CREATION_STAGE, TIME.plusSeconds(100));
        timestampedStagesAggregation.putStageFinish(StageAggregationUtils.RESULTS_STAGE, finishTime);

        var expectedRecheckIteration = recheckIteration.toBuilder()
                .status(finishStatus)
                .finish(finishTime)
                .totalDurationSeconds(100L)
                .timestampedStagesAggregation(timestampedStagesAggregation)
                .build();
        var expectedParentIteration = parentIteration.toBuilder()
                .totalDurationSeconds(200L)
                .finalStatus(finishStatus)
                .hasUnfinishedRechecks(false)
                .unfinishedRecheckIterationNumbers(Set.of())
                .recheckAggregationsByNumber(Map.of(
                        recheckIterationNumber,
                        CheckIterationNumberStagesAggregation.of(expectedRecheckIteration)
                ))
                .build();

        checkService.processIterationFinish(SAMPLE_ITERATION_ID_2, finishStatus, finishTime);

        var updatedRecheckIteration = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID_2);
        var updatedParentIteration = cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID);

        Assertions.assertEquals(expectedRecheckIteration, updatedRecheckIteration);
        Assertions.assertEquals(expectedParentIteration, updatedParentIteration);
    }

    private List<CheckTaskPartitionTraceEntity> createTraces(
            CheckTaskEntity.Id taskId, int partition, Map<String, Instant> traceTypeTimes
    ) {
        var result = new ArrayList<CheckTaskPartitionTraceEntity>();

        for (var entry : traceTypeTimes.entrySet()) {
            result.add(
                    CheckTaskPartitionTraceEntity.builder()
                            .id(new CheckTaskPartitionTraceEntity.Id(taskId, partition, entry.getKey()))
                            .time(entry.getValue())
                            .build()
            );
        }

        return result;
    }

    private TimestampedTraceStages createTimestampedTraceStages(
            Map<String, Instant> stagesStarts, Map<String, Instant> stagesFinish
    ) {
        var result = new TimestampedTraceStages();
        stagesStarts.forEach(result::putStageStart);
        stagesFinish.forEach(result::putStageFinish);

        return result;
    }

    private DurationStages createDurationStages(Map<String, Long> stagesDurations) {
        var result = new DurationStages(true);
        stagesDurations.forEach(result::putStageDuration);

        return result;
    }
}
