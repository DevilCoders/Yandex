package ru.yandex.ci.observer.reader.message.internal;

import java.time.Instant;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

import lombok.Value;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.observer.core.Internal;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.reader.check.ObserverInternalCheckService;
import ru.yandex.ci.observer.reader.message.IterationFinishStatusWithTime;
import ru.yandex.ci.observer.reader.proto.ObserverProtoMappers;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;

public class ObserverInternalStreamMessageProcessor {
    private final ObserverInternalCheckService checkService;
    private final ObserverEntitiesChecker checker;

    public ObserverInternalStreamMessageProcessor(ObserverInternalCheckService checkService,
                                                  ObserverEntitiesChecker checker) {
        this.checkService = checkService;
        this.checker = checker;
    }

    public void processRegisteredMessages(List<Internal.Registered> messages) {
        for (var message : messages) {
            if (message.hasIterationId()) {
                checker.checkExistingIterationWithRegistrationLag(message.getIterationId(), false);
            }
            if (message.hasTaskId()) {
                checker.checkExistingTaskWithRegistrationLag(message.getTaskId(), false);
            }
        }
    }

    public void processAggregateMessages(Map<CheckIteration.IterationId, List<Internal.AggregateStages>> messages) {
        for (var e : messages.entrySet()) {
            var iterationOpt = checker.checkExistingIterationWithRegistrationLag(e.getKey(), false);
            if (iterationOpt.isEmpty()) {
                continue;
            }

            var taskIds = Set.copyOf(checker.filterExistingTasksWithRegistrationLag(
                    e.getValue().stream()
                            .flatMap(m -> m.getTaskIdsList().stream())
                            .distinct()
                            .collect(Collectors.toList()),
                    false
            ));

            checkService.aggregateTraceStages(iterationOpt.get(), taskIds);
        }
    }

    public void processFatalErrorMessages(List<Internal.InternalMessage> messages) {
        for (var message : messages) {
            var taskOpt = checker.checkExistingTaskWithRegistrationLag(message.getFatalError().getTaskId(), true);
            if (taskOpt.isEmpty()) {
                continue;
            }

            checkService.processCheckTaskFatalError(
                    taskOpt.get(), ProtoConverter.convert(message.getMeta().getTimestamp())
            );
        }
    }

    public void processCancelMessages(List<Internal.InternalMessage> messages) {
        Map<CheckIterationEntity.Id, Instant> iterationsWithTime = new HashMap<>();

        for (var message : messages) {
            var iterationOpt = checker.checkExistingIterationWithRegistrationLag(
                    message.getCancel().getIterationId(), true
            );
            if (iterationOpt.isEmpty()) {
                continue;
            }

            iterationsWithTime.putIfAbsent(
                    iterationOpt.get(),
                    ProtoConverter.convert(message.getMeta().getTimestamp())
            );
        }

        iterationsWithTime.forEach(checkService::processCheckTaskCancel);
    }

    public void processPessimizeMessages(List<CheckIteration.IterationId> iterationsToPessimize) {
        iterationsToPessimize.stream()
                .map(i -> {
                    var iterationOpt = checker.checkExistingIterationWithRegistrationLag(i, true);
                    if (iterationOpt.isEmpty()) {
                        return null;
                    }

                    return iterationOpt.get();
                })
                .filter(Objects::nonNull)
                .forEach(checkService::processPessimize);
    }

    public void processTechnicalStatsMessages(List<Internal.TechnicalStatisticsMessage> messages) {
        for (var m : messages) {
            var taskOpt = checker.checkExistingTaskWithRegistrationLag(m.getTaskId(), true);
            var iterationOpt = checker.checkExistingIterationWithRegistrationLag(
                    m.getTaskId().getIterationId(), true
            );

            if (iterationOpt.isEmpty() || taskOpt.isEmpty()) {
                continue;
            }

            var statistics = ObserverProtoMappers.toTechnicalStatistics(m.getTechnicalStatistics());

            checkService.addTechnicalStatistics(taskOpt.get(), m.getPartition(), statistics);
        }
    }

    public void processFinishPartitionMessages(
            Map<CheckTaskOuterClass.FullTaskId, List<Internal.FinishPartition>> messagesByTaskIds
    ) {
        messagesByTaskIds.entrySet().stream()
                .map(e -> {
                    var taskIdOpt = checker.checkExistingTaskWithRegistrationLag(e.getKey(), true);
                    if (taskIdOpt.isEmpty()) {
                        return null;
                    }

                    return new TaskIdPartition(
                            taskIdOpt.get(),
                            e.getValue().stream()
                                    .map(Internal.FinishPartition::getPartition)
                                    .collect(Collectors.toList())
                    );
                })
                .filter(Objects::nonNull)
                .forEach(t -> checkService.processPartitionFinished(t.getTaskId(), t.getPartitions()));
    }

    public void processIterationFinishMessages(
            Map<CheckIteration.IterationId, Internal.IterationFinish> iterationToFinish
    ) {
        iterationToFinish.entrySet().stream()
                .map(e -> {
                    var iterationOpt = checker.checkExistingIterationWithRegistrationLag(e.getKey(), true);
                    if (iterationOpt.isEmpty()) {
                        return null;
                    }

                    return new IterationFinishStatusWithTime(
                            iterationOpt.get(),
                            e.getValue().getStatus(),
                            e.getValue().hasFinishTimestamp()
                                    ? ProtoConverter.convert(e.getValue().getFinishTimestamp())
                                    : Instant.now()
                    );
                })
                .filter(Objects::nonNull)
                .forEach(e -> checkService.processIterationFinish(
                        e.getIterationId(), e.getStatus(), e.getTime()
                ));
    }

    @Value
    private static class TaskIdPartition {
        CheckTaskEntity.Id taskId;
        List<Integer> partitions;
    }
}
