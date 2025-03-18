package ru.yandex.ci.observer.reader.check;

import java.time.Instant;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.observer.core.cache.ObserverCache;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.core.db.model.traces.CheckTaskPartitionTraceEntity;
import ru.yandex.ci.observer.core.utils.StageAggregationUtils;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;
import ru.yandex.ci.storage.core.utils.YandexUrls;

@Slf4j
public class ObserverInternalCheckService {
    private final ObserverCache cache;
    private final CiObserverDb db;

    public ObserverInternalCheckService(ObserverCache cache, CiObserverDb db) {
        this.cache = cache;
        this.db = db;
    }

    public void processCheckTaskFatalError(CheckTaskEntity.Id taskId, Instant messageTime) {
        log.info("Processing fatal error message for task {}", taskId);
        var tasks = cache.tasksGrouped().getAll(taskId.getIterationId());
        var fatalErrorTask = tasks.stream().filter(t -> t.getId().equals(taskId))
                .findFirst().orElseThrow(() -> new NoSuchElementException(String.format(
                        "CheckTask with id %s not found in tasks %s", taskId, tasks
                )));
        if (CheckStatusUtils.isCompleted(fatalErrorTask.getStatus())) {
            log.warn("Check task {} already completed, skipping CANCEL_BY_FATAL_ERROR message", fatalErrorTask.getId());
            return;
        }

        for (var task : tasks) {
            if (task.getId().equals(taskId) || CheckStatusUtils.isCompleted(task.getStatus())) {
                continue;
            }

            processTaskFinished(task, FinishType.CANCEL, messageTime);
        }

        processTaskFinished(fatalErrorTask, FinishType.CANCEL_BY_FATAL_ERROR, messageTime);
    }

    public void processPartitionFinished(CheckTaskEntity.Id taskId, List<Integer> partitions) {
        var task = cache.tasksGrouped().get(taskId).orElseThrow();

        log.info(
                "Processing partition finished for task: '{}', partitions: {}, current finished: {}",
                task.getId(), partitions, task.getCompletedPartitions()
        );

        var taskChanged = false;

        for (var partition : partitions) {
            if (task.getCompletedPartitions().contains(partition)) {
                log.warn("Partition was already finished");
                continue;
            }

            var completedPartitions = new HashSet<>(task.getCompletedPartitions());
            completedPartitions.add(partition);

            task = task.toBuilder().completedPartitions(completedPartitions).build();
            taskChanged = true;

            log.info("task '{}' completed partitions [{}]", task.getId(), task.getCompletedPartitions());
        }

        if (taskChanged) {
            var updatedTask = task;
            cache.modifyWithDbTx(cache -> cache.tasksGrouped().writeThrough(updatedTask));
        }
    }

    public void processCheckTaskCancel(CheckIterationEntity.Id iterationId, Instant finishTime) {
        log.info("Processing cancel message for iteration {}", iterationId);
        var iteration = cache.iterationsGrouped().get(iterationId).orElseThrow();
        if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
            log.warn("Check iteration {} already completed, skipping CANCEL message", iterationId);
            return;
        }

        var tasks = cache.tasksGrouped().getAll(iterationId).stream()
                .filter(t -> CheckStatusUtils.isRunning(t.getStatus()))
                .map(t -> t.toBuilder().status(Common.CheckStatus.CANCELLED).finish(finishTime).build())
                .collect(Collectors.toList());

        var updated = processIterationFinish(iteration, FinishType.CANCEL, Common.CheckStatus.CANCELLED, finishTime);
        var updParentIteration = updateParentIteration(updated.getIteration());

        cache.modifyWithDbTx(c -> {
            c.tasksGrouped().writeThrough(tasks);
            writeThroughUpdatedEntities(c, updated);

            if (updParentIteration != null) {
                c.iterationsGrouped().writeThrough(updParentIteration);
            }
        });
    }

    public void aggregateTraceStages(CheckIterationEntity.Id iterationId, Set<CheckTaskEntity.Id> taskIds) {
        var tasks = cache.tasksGrouped().getAll(iterationId);
        var tasksToUpdate = tasks.stream()
                .filter(t -> taskIds.contains(t.getId()))
                .toList();
        var otherTasks = tasks.stream()
                .filter(t -> !taskIds.contains(t.getId()))
                .toList();
        var iteration = cache.iterationsGrouped().get(iterationId).orElseThrow();
        var updatedTasks = new HashSet<CheckTaskEntity>();

        for (var task : tasksToUpdate) {
            log.info("Aggregating task stages for task {}", task.getId());

            var traces = db.readOnly(() -> db.traces().findByCheckTask(task.getId()));
            var newTask = StageAggregationUtils.updateCheckTaskAggregation(task, iteration.getCreated(), traces);
            var newTaskWithAttributes = updateTaskAttributes(newTask, traces);
            updatedTasks.add(newTaskWithAttributes);
        }

        var updatedIteration = StageAggregationUtils.updateIterationAggregation(
                iteration,
                Stream.concat(otherTasks.stream(), updatedTasks.stream()).collect(Collectors.toMap(
                        t -> new CheckIterationEntity.CheckTaskKey(t.getJobName(), t.isRight()),
                        Function.identity(),
                        //TODO remove after CI-2147
                        (t1, t2) -> {
                            log.warn("CheckTasks collision between {} and {}", t1, t2);
                            return t1.getCreated().isBefore(t2.getCreated()) ? t1 : t2;
                        }
                ))
        );

        var updParentIteration = updateParentIteration(updatedIteration);

        cache.modifyWithDbTx(c -> {
            c.tasksGrouped().writeThrough(updatedTasks);
            c.iterationsGrouped().writeThrough(updatedIteration);

            if (updParentIteration != null) {
                c.iterationsGrouped().writeThrough(updParentIteration);
            }
        });

        log.info("Stages aggregated successfully, parent iteration: {}, iteration: {}, tasks: {}",
                updParentIteration != null ? updParentIteration.getId() : null,
                updatedIteration.getId(),
                taskIds
        );
    }

    public void addTechnicalStatistics(
            CheckTaskEntity.Id taskId,
            int partition,
            TechnicalStatistics technicalStatistics
    ) {
        log.info("Processing technical statistics message from task {}, partition {}", taskId, partition);
        var task = cache.tasksGrouped().get(taskId).orElseThrow();

        var taskStatistics = new HashMap<>(task.getPartitionsTechnicalStatistics());
        var previousTaskStatistics = taskStatistics.getOrDefault(partition, TechnicalStatistics.EMPTY);
        var updatedTechnicalStatistics = technicalStatistics.max(previousTaskStatistics);
        taskStatistics.put(partition, updatedTechnicalStatistics);

        var iteration = cache.iterationsGrouped().get(taskId.getIterationId()).orElseThrow();


        var updatedTask = task.toBuilder().partitionsTechnicalStatistics(taskStatistics).build();
        var updatedIteration = iteration.withStatistics(
                iteration.getStatistics().plus(updatedTechnicalStatistics).minus(previousTaskStatistics)
        );

        cache.modifyWithDbTx(cache -> {
            cache.iterationsGrouped().writeThrough(updatedIteration);
            cache.tasksGrouped().writeThrough(updatedTask);
        });
    }

    public void processPessimize(CheckIterationEntity.Id iterationId) {
        log.info("Processing pessimization message for iteration {}", iterationId);
        var iteration = cache.iterationsGrouped().get(iterationId).orElseThrow();

        if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
            log.warn("Iteration {} is completed, skipping pessimize", iterationId);
            return;
        }

        if (iteration.isPessimized()) {
            log.warn("Iteration {} is already pessimized, skipping pessimize", iterationId);
            return;
        }

        cache.modifyWithDbTx(cache -> cache.iterationsGrouped().writeThrough(iteration.withPessimized(true)));
    }

    public void processIterationFinish(
            CheckIterationEntity.Id iterationId, Common.CheckStatus status, Instant finishTime
    ) {
        log.info("Processing iteration finish message for iteration {}, with status {}", iterationId, status);
        var iteration = cache.iterationsGrouped().get(iterationId).orElseThrow();

        if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
            log.warn("Iteration {} is completed, skipping finish message", iterationId);
            return;
        }

        var iterationWithUpdatedFinish = iteration.toBuilder()
                .finish(finishTime)
                .status(status)
                .totalDurationSeconds(finishTime.getEpochSecond() - iteration.getCreated().getEpochSecond())
                .build();

        var tasks = cache.tasksGrouped().getAll(iterationId).stream()
                .collect(Collectors.toMap(
                        t -> new CheckIterationEntity.CheckTaskKey(t.getJobName(), t.isRight()),
                        Function.identity(),
                        //TODO remove after CI-2147
                        (t1, t2) -> {
                            log.warn("CheckTasks collision between {} and {}", t1, t2);
                            return t1.getCreated().isBefore(t2.getCreated()) ? t1 : t2;
                        }
                ));

        var updatedIteration = StageAggregationUtils.updateIterationAggregation(
                iterationWithUpdatedFinish, tasks
        );
        var updatedParentIteration = updateParentIteration(updatedIteration);

        cache.modifyWithDbTx(cache -> {
            cache.iterationsGrouped().writeThrough(updatedIteration);
            log.info("Iteration {} finish message with status {} processed", iterationId, status);

            if (updatedParentIteration != null) {
                cache.iterationsGrouped().writeThrough(updatedParentIteration);
                log.info(
                        "Parent iteration {} of iteration {} updated on iteration finish",
                        updatedParentIteration.getId(), iterationId
                );
            }
        });
    }

    private void processTaskFinished(CheckTaskEntity task, FinishType finishType, Instant finishTime) {
        var taskId = task.getId();
        var updatedTask = switch (finishType) {
            case CANCEL -> task.toBuilder()
                    .status(Common.CheckStatus.CANCELLED)
                    .finish(finishTime)
                    .build();
            case CANCEL_BY_FATAL_ERROR -> task.toBuilder()
                    .status(Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR)
                    .finish(finishTime)
                    .build();
            case COMPLETE -> task.toBuilder()
                    .status(Common.CheckStatus.COMPLETED_SUCCESS)
                    .finish(finishTime)
                    .build();
        };

        var tasks = cache.tasksGrouped().getAll(taskId.getIterationId()).stream()
                .filter(t -> !t.getId().equals(taskId))
                .collect(Collectors.toList());
        if (tasks.stream().allMatch(t -> CheckStatusUtils.isCompleted(t.getStatus()))) {
            var updated = processIterationFinish(taskId, finishType, tasks, finishTime);
            var updParentIteration = updateParentIteration(updated.getIteration());

            cache.modifyWithDbTx(c -> {
                c.tasksGrouped().writeThrough(updatedTask);
                writeThroughUpdatedEntities(c, updated);

                if (updParentIteration != null) {
                    c.iterationsGrouped().writeThrough(updParentIteration);
                }
            });
            log.info("task '{}' completed, iteration {} finished", task.getId(), taskId.getIterationId());
        } else {
            cache.modifyWithDbTx(c -> c.tasksGrouped().writeThrough(updatedTask));
            log.info("task '{}' completed", task.getId());
        }
    }

    private UpdatedEntities processIterationFinish(
            CheckTaskEntity.Id triggerTaskId,
            FinishType finishType,
            List<CheckTaskEntity> taskTraces,
            Instant finishTime
    ) {
        var iteration = cache.iterationsGrouped().get(triggerTaskId.getIterationId()).orElseThrow();
        var nextStatus = getNextStatus(
                finishType,
                taskTraces.stream().map(CheckTaskEntity::getStatus).collect(Collectors.toList())
        );

        return processIterationFinish(iteration, finishType, nextStatus, finishTime);
    }

    private Common.CheckStatus getNextStatus(FinishType finishType, List<Common.CheckStatus> statuses) {
        if (finishType == FinishType.CANCEL_BY_FATAL_ERROR) {
            return Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR;
        }

        Common.CheckStatus nextStatus = Common.CheckStatus.COMPLETED_SUCCESS;
        for (var status : statuses) {
            if (status == Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR) {
                return status;
            }
        }

        if (finishType == FinishType.CANCEL) {
            return Common.CheckStatus.CANCELLED;
        }

        return nextStatus;
    }

    private UpdatedEntities processIterationFinish(
            CheckIterationEntity iteration, FinishType finishType, Common.CheckStatus nextStatus, Instant finishTime
    ) {
        iteration = iteration.toBuilder().status(nextStatus).finish(finishTime).build();

        var activeIterations = cache.iterationsGrouped().countActive(iteration.getId().getCheckId());
        if (activeIterations > 1) {
            return new UpdatedEntities(iteration, null);
        }

        log.info("Finishing check {}, caused by iteration {}", iteration.getId().getCheckId(), iteration.getId());

        return new UpdatedEntities(iteration, processCheckFinish(finishType, iteration.getId(), finishTime));
    }

    private CheckEntity processCheckFinish(
            FinishType finishType, CheckIterationEntity.Id triggerIterationId, Instant finishTime
    ) {
        var check = cache.checks().get(triggerIterationId.getCheckId()).orElseThrow();

        var iterationStatuses = this.cache.iterationsGrouped().getAll(check.getId()).stream()
                .filter(it -> !it.getId().equals(triggerIterationId))
                .map(CheckIterationEntity::getStatus)
                .collect(Collectors.toList());

        return check.toBuilder().status(getNextStatus(finishType, iterationStatuses)).completed(finishTime).build();
    }

    private void writeThroughUpdatedEntities(ObserverCache.Modifiable cache, UpdatedEntities updated) {
        cache.iterationsGrouped().writeThrough(updated.getIteration());
        if (updated.getCheck() != null) {
            cache.checks().writeThrough(updated.getCheck());
        }
    }

    @Nullable
    private CheckIterationEntity updateParentIteration(@Nullable CheckIterationEntity iteration) {
        if (iteration == null || iteration.getParentIterationNumber() == null) {
            return null;
        }

        return StageAggregationUtils.updateParentIterationOnRecheck(
                cache.iterationsGrouped().getOrThrow(
                        new CheckIterationEntity.Id(
                                iteration.getId().getCheckId(),
                                iteration.getId().getIterType(),
                                iteration.getParentIterationNumber()
                        )
                ),
                iteration
        );
    }

    private CheckTaskEntity updateTaskAttributes(CheckTaskEntity task, List<CheckTaskPartitionTraceEntity> traces) {
        if (task.getTaskAttributes().containsKey("SANDBOX_TASK_URL")) {
            return task;
        }

        Map<String, String> updatedTaskAttributes = new HashMap<>();
        var graphTrace = traces.stream()
                .filter(t -> t.getId().getTraceType().equals("distbuild/graph/started"))
                .filter(t -> t.getAttributes().containsKey("sandbox_task_id"))
                .findFirst();

        graphTrace.ifPresent(t -> {
            var sandboxTaskId = parseLong(t.getAttributes().get("sandbox_task_id"));

            if (sandboxTaskId != null) {
                updatedTaskAttributes.put("SANDBOX_TASK_URL", YandexUrls.sandboxTaskUrl(sandboxTaskId));
            }
        });

        if (updatedTaskAttributes.isEmpty()) {
            return task;
        }

        task.getTaskAttributes().forEach(updatedTaskAttributes::putIfAbsent);
        return task.toBuilder()
                .taskAttributes(updatedTaskAttributes)
                .build();
    }

    @Nullable
    private Long parseLong(String raw) {
        try {
            return Long.parseLong(raw);
        } catch (NumberFormatException numberFormatException) {
            log.error("Unable to parse long from {}", raw, numberFormatException);
            return null;
        }
    }

    private enum FinishType {
        CANCEL,
        CANCEL_BY_FATAL_ERROR,
        COMPLETE
    }

    @Value
    private static class UpdatedEntities {
        CheckIterationEntity iteration;
        @Nullable
        CheckEntity check;
    }
}
