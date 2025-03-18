package ru.yandex.ci.observer.core.utils;

import java.time.Instant;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;

import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationNumberStagesAggregation;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.core.db.model.traces.CheckTaskPartitionTraceEntity;
import ru.yandex.ci.observer.core.db.model.traces.DurationStages;
import ru.yandex.ci.observer.core.db.model.traces.TimestampedTraceStages;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.constant.CheckTypeUtils;

import static java.util.Objects.requireNonNullElse;

public class StageAggregationUtils {
    public static final String PRE_CREATION_STAGE = "preIterationCreation";
    public static final String CREATION_STAGE = "creation";
    public static final String SANDBOX_STAGE = "sandbox";
    public static final String CONFIGURE_STAGE = "configure";
    public static final String GRAPH_STAGE = "graph";
    public static final String BUILD_STAGE = "build";
    public static final String RESULTS_STAGE = "resultsProcessing";

    public static final List<String> STAGE_TRACES = List.of(
            "storage/check_task_created",
            "distbuild/started",
            "distbuild/main/started",
            "distbuild/main/finished",
            "storage/check_task_finished",
            "distbuild/graph/started",
            "distbuild/graph/finished",
            "distbuild/finished"
    );
    public static final List<String> STAGES = List.of(
            PRE_CREATION_STAGE,
            CREATION_STAGE,
            SANDBOX_STAGE,
            CONFIGURE_STAGE,
            BUILD_STAGE,
            RESULTS_STAGE
    );
    public static final Map<String, List<String>> TRACE_TO_STAGE_STARTS = Map.of(
            STAGE_TRACES.get(0), List.of(SANDBOX_STAGE),
            STAGE_TRACES.get(1), List.of(CONFIGURE_STAGE, RESULTS_STAGE),
            STAGE_TRACES.get(2), List.of(BUILD_STAGE),
            STAGE_TRACES.get(5), List.of(GRAPH_STAGE)
    );
    public static final Map<String, List<String>> TRACE_TO_STAGE_FINISHES = Map.of(
            STAGE_TRACES.get(0), List.of(CREATION_STAGE),
            STAGE_TRACES.get(2), List.of(CONFIGURE_STAGE),
            STAGE_TRACES.get(3), List.of(BUILD_STAGE),
            STAGE_TRACES.get(4), List.of(RESULTS_STAGE),
            STAGE_TRACES.get(6), List.of(GRAPH_STAGE),
            STAGE_TRACES.get(7), List.of(SANDBOX_STAGE)
    );
    public static final Map<String, List<String>> TRACE_TO_STAGE_STARTS_FLAT = Map.of(
            STAGE_TRACES.get(0), List.of(SANDBOX_STAGE),
            STAGE_TRACES.get(1), List.of(CONFIGURE_STAGE),
            STAGE_TRACES.get(2), List.of(BUILD_STAGE),
            STAGE_TRACES.get(3), List.of(RESULTS_STAGE)
    );
    public static final Map<String, List<String>> TRACE_TO_STAGE_FINISHES_FLAT = Map.of(
            STAGE_TRACES.get(0), List.of(CREATION_STAGE),
            STAGE_TRACES.get(1), List.of(SANDBOX_STAGE),
            STAGE_TRACES.get(2), List.of(CONFIGURE_STAGE),
            STAGE_TRACES.get(3), List.of(BUILD_STAGE),
            STAGE_TRACES.get(4), List.of(RESULTS_STAGE)
    );
    private static final Set<String> ALL_PARTITION_TRACES = Set.of(STAGE_TRACES.get(0), STAGE_TRACES.get(4));

    private StageAggregationUtils() {
    }

    public static CheckTaskEntity updateCheckTaskAggregation(CheckTaskEntity task,
                                                             @Nonnull Instant iterationCreated,
                                                             @Nonnull List<CheckTaskPartitionTraceEntity> traces) {
        Map<Integer, TimestampedTraceStages> newTmStagesByPartitions = task.getTimestampedStagesByPartitions() == null
                ? new HashMap<>()
                : new HashMap<>(task.getTimestampedStagesByPartitions());

        var tracesByPartitions = traces.stream().collect(
                Collectors.groupingBy(t -> t.getId().getPartition(), Collectors.toList())
        );

        for (var partitionTraces : tracesByPartitions.entrySet()) {
            if (partitionTraces.getKey() == CheckTaskPartitionTraceEntity.Id.ALL_PARTITIONS) {
                continue;
            }

            Map<String, List<Instant>> fullPartitionTraces = Stream.concat(
                    partitionTraces.getValue().stream(),
                    tracesByPartitions.getOrDefault(CheckTaskPartitionTraceEntity.Id.ALL_PARTITIONS, List.of()).stream()
            ).collect(
                    Collectors.toMap(tr -> tr.getId().getTraceType(), tr -> List.of(tr.getTime()))
            );

            newTmStagesByPartitions.put(
                    partitionTraces.getKey(), tracesToTmStage(
                            iterationCreated, 1, fullPartitionTraces,
                            TRACE_TO_STAGE_STARTS, TRACE_TO_STAGE_FINISHES
                    )
            );
        }

        var updatedTask = task.toBuilder()
                .timestampedStagesByPartitions(newTmStagesByPartitions);

        TimestampedTraceStages stages;
        if (newTmStagesByPartitions.keySet().size() >= task.getNumberOfPartitions()) {
            stages = tracesToTmStage(
                    iterationCreated,
                    task.getNumberOfPartitions(),
                    traces.stream().collect(Collectors.groupingBy(
                            tr -> tr.getId().getTraceType(),
                            Collectors.mapping(CheckTaskPartitionTraceEntity::getTime, Collectors.toList())
                    )),
                    TRACE_TO_STAGE_STARTS_FLAT, TRACE_TO_STAGE_FINISHES_FLAT
            );
        } else {
            stages = allPartitionsTracesToCheckTaskStage(
                    iterationCreated,
                    tracesByPartitions.getOrDefault(CheckTaskPartitionTraceEntity.Id.ALL_PARTITIONS, List.of())
            );
        }

        //Finish check task when resultProcessing is finished
        if (stages.getStagesFinishes().containsKey(RESULTS_STAGE) && CheckStatusUtils.isRunning(task.getStatus())) {
            var finishTrace = tracesByPartitions.getOrDefault(
                            CheckTaskPartitionTraceEntity.Id.ALL_PARTITIONS, List.of()
                    ).stream()
                    .filter(t -> t.getId().getTraceType().equals(STAGE_TRACES.get(4)))
                    .findFirst().orElseThrow();

            updatedTask.status(Common.CheckStatus.COMPLETED_SUCCESS)
                    .finish(finishTrace.getTime());
        } else if (stages.getStagesFinishes().containsKey(CREATION_STAGE)
                && task.getStatus() == Common.CheckStatus.CREATED) {
            updatedTask.status(Common.CheckStatus.RUNNING);
        }

        updatedTask.timestampedStagesAggregation(stages);

        return updatedTask.build();
    }

    public static CheckIterationEntity updateIterationAggregation(
            CheckIterationEntity iteration,
            Map<CheckIterationEntity.CheckTaskKey, CheckTaskEntity> tasks
    ) {
        var stagesTmAggregation = checkTaskTmStagesToIterationTmStages(
                iteration.getCreated(), iteration.getExpectedCheckTasks(), tasks, iteration.getFinish()
        );

        addPreCreationTmStage(iteration.getCheckType(), iteration.getCreated(), iteration.getDiffSetEventCreated(),
                iteration.getRightRevisionTimestamp(), stagesTmAggregation);

        var stagesAggregation = iterationTmStagesToTraceStages(stagesTmAggregation);

        var updatedIteration = iteration.toBuilder()
                .timestampedStagesAggregation(stagesTmAggregation)
                .stagesAggregation(stagesAggregation);

        if (stagesTmAggregation.getStagesFinishes().containsKey(CREATION_STAGE)
                && iteration.getStatus() == Common.CheckStatus.CREATED) {
            updatedIteration.status(Common.CheckStatus.RUNNING);
        }

        return updatedIteration.build();
    }

    public static CheckIterationEntity updateParentIterationOnRecheck(
            CheckIterationEntity parentIteration,
            CheckIterationEntity recheckIteration
    ) {
        var recheckNumber = recheckIteration.getId().getNumber();
        var parentIterationBuilder = parentIteration.toBuilder();

        var recheckAggregations = new HashMap<>(parentIteration.getRecheckAggregationsByNumber());
        recheckAggregations.put(recheckNumber, CheckIterationNumberStagesAggregation.of(recheckIteration));

        var unfinishedNumbers = new HashSet<>(parentIteration.getUnfinishedRecheckIterationNumbers());
        if (CheckStatusUtils.isActive(recheckIteration.getStatus())) {
            unfinishedNumbers.add(recheckNumber);
        } else {
            unfinishedNumbers.remove(recheckNumber);
        }

        if (unfinishedNumbers.isEmpty()) {
            var recheckFinish = recheckIteration.getFinish();
            if (recheckFinish != null) {
                var prevTotalDuration = requireNonNullElse(parentIteration.getTotalDurationSeconds(), 0L);
                var parentFinish = requireNonNullElse(parentIteration.getFinish(), recheckIteration.getCreated());

                parentIterationBuilder.totalDurationSeconds(Math.max(
                        prevTotalDuration,
                        computeTotalDurationSeconds(
                                parentFinish, recheckFinish, parentIteration.getCreated()
                        )
                ));
            }

            parentIterationBuilder.finalStatus(recheckIteration.getStatus());
        }

        return parentIterationBuilder
                .hasUnfinishedRechecks(!unfinishedNumbers.isEmpty())
                .unfinishedRecheckIterationNumbers(unfinishedNumbers)
                .recheckAggregationsByNumber(recheckAggregations)
                .mergeCheckRelatedLinks(recheckIteration.getCheckRelatedLinks())
                .build();
    }

    public static boolean isAggregatableStageTrace(String traceType) {
        return STAGE_TRACES.contains(traceType);
    }

    public static Instant getPreCreationStartedAt(
            CheckOuterClass.CheckType checkType,
            Instant created,
            @Nullable Instant diffSetEventCreated,
            @Nullable Instant rightRevisionTimestamp
    ) {
        if (CheckTypeUtils.isPrecommitCheck(checkType)) {
            return diffSetEventCreated != null && diffSetEventCreated.isAfter(Instant.EPOCH)
                    ? diffSetEventCreated
                    : created;
        }
        return requireNonNullElse(rightRevisionTimestamp, created);
    }

    @VisibleForTesting
    static TimestampedTraceStages checkTaskTmStagesToIterationTmStages(
            Instant iterationCreated,
            List<CheckIterationEntity.CheckTaskKey> expectedTasks,
            Map<CheckIterationEntity.CheckTaskKey, CheckTaskEntity> tasks,
            @Nullable Instant iterationFinishTime
    ) {
        var stages = new TimestampedTraceStages();
        var filteredTasksAggregations = expectedTasks.stream()
                .map(tasks::get)
                .filter(Objects::nonNull)
                .map(CheckTaskEntity::getTimestampedStagesAggregation)
                .filter(Objects::nonNull)
                .collect(Collectors.toList());

        if (filteredTasksAggregations.size() < expectedTasks.size()) {
            return stages;
        }

        for (var stage : STAGES) {
            // Results processing stage also includes time from finish processing tasks to iteration finish message
            if (stage.equals(RESULTS_STAGE)) {
                continue;
            }

            aggregateStage(stage, filteredTasksAggregations, stages);
        }

        stages.putStageStart(CREATION_STAGE, iterationCreated);

        if (iterationFinishTime != null) {
            stages.putStageFinish(RESULTS_STAGE, iterationFinishTime);
        }

        return stages;
    }

    @VisibleForTesting
    static DurationStages iterationTmStagesToTraceStages(TimestampedTraceStages tmStages) {
        // PRE_CREATION_STAGE always set after CREATION_STAGE started, i.e. always finished
        var result = new DurationStages();
        var stagesFinishes = tmStages.getStagesFinishes();
        result.putStageDuration(
                PRE_CREATION_STAGE,
                Math.max(0, stagesFinishes.get(PRE_CREATION_STAGE).getEpochSecond()
                        - tmStages.getStagesStarts().get(PRE_CREATION_STAGE).getEpochSecond())
        );

        if (!stagesFinishes.containsKey(CREATION_STAGE)) {
            return result;
        }

        var prevStageFinished = tmStages.getStagesStarts().get(CREATION_STAGE).getEpochSecond();
        for (var stage : STAGES) {
            if (stage.equals(PRE_CREATION_STAGE)) {
                continue;
            }

            if (stagesFinishes.containsKey(stage)) {
                result.putStageDuration(
                        stage,
                        Math.max(0, stagesFinishes.get(stage).getEpochSecond() - prevStageFinished)
                );

                prevStageFinished = Math.max(prevStageFinished, stagesFinishes.get(stage).getEpochSecond());
            }
        }

        return result;
    }

    @VisibleForTesting
    static TimestampedTraceStages tracesToTmStage(
            Instant iterationCreated,
            int requiredNumberTracesOfStage,
            Map<String, List<Instant>> traces,
            Map<String, List<String>> startTraceStageMapping,
            Map<String, List<String>> finishTraceStageMapping
    ) {
        var stages = new TimestampedTraceStages();
        traces.forEach((trace, times) -> {
            if (times.size() < requiredNumberTracesOfStage && !ALL_PARTITION_TRACES.contains(trace)) {
                return;
            }

            times.forEach(t -> stages.putTraceTime(startTraceStageMapping, finishTraceStageMapping, trace, t));
        });
        stages.putStageStart(CREATION_STAGE, iterationCreated);

        return stages;
    }

    private static TimestampedTraceStages allPartitionsTracesToCheckTaskStage(
            Instant iterationCreated, List<CheckTaskPartitionTraceEntity> traces
    ) {
        var taskStages = new TimestampedTraceStages();

        for (var trace : traces) {
            if (trace.getId().getTraceType().equals(STAGE_TRACES.get(0))) {
                taskStages.putStageStart(CREATION_STAGE, iterationCreated);
                taskStages.putStageFinish(CREATION_STAGE, trace.getTime());
            }

            if (trace.getId().getTraceType().equals(STAGE_TRACES.get(4))) {
                taskStages.putStageFinish(RESULTS_STAGE, trace.getTime());
            }
        }

        return taskStages;
    }

    @VisibleForTesting
    static void aggregateStage(
            String stage, List<TimestampedTraceStages> tasksAggregations, TimestampedTraceStages result
    ) {
        Instant maxStageFinish = Instant.ofEpochSecond(0);

        for (var aggr : tasksAggregations) {
            maxStageFinish = maxTime(maxStageFinish, aggr.getStagesFinishes().get(stage));
        }

        if (maxStageFinish != null && maxStageFinish.isAfter(Instant.ofEpochSecond(0))) {
            result.putStageFinish(stage, maxStageFinish);
        }
    }

    @VisibleForTesting
    static void addPreCreationTmStage(
            CheckOuterClass.CheckType checkType,
            Instant created,
            @Nullable Instant diffSetEventCreated,
            Instant rightRevisionTimestamp,
            TimestampedTraceStages stagesTmAggregation
    ) {
        var startedAt = getPreCreationStartedAt(checkType, created, diffSetEventCreated, rightRevisionTimestamp);

        stagesTmAggregation.putStageStart(PRE_CREATION_STAGE, startedAt);
        stagesTmAggregation.putStageFinish(PRE_CREATION_STAGE, created);
    }

    @Nullable
    private static Instant maxTime(@Nullable Instant tm1, @Nullable Instant tm2) {
        if (tm2 == null || tm1 == null) {
            return null;
        }

        return tm1.isAfter(tm2) ? tm1 : tm2;
    }

    private static Long computeTotalDurationSeconds(
            @Nullable Instant finish,
            @Nullable Instant recheckFinish,
            Instant created
    ) {
        if (finish == null && recheckFinish == null) {
            return 0L;
        }

        if (recheckFinish == null) {
            return finish.getEpochSecond() - created.getEpochSecond();
        }

        return Math.max(finish.getEpochSecond(), recheckFinish.getEpochSecond()) - created.getEpochSecond();
    }
}
