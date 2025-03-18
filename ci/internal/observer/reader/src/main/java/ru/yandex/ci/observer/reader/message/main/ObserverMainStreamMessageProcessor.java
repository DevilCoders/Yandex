package ru.yandex.ci.observer.reader.message.main;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.reader.check.ObserverCheckService;
import ru.yandex.ci.observer.reader.message.FullTaskIdWithPartition;
import ru.yandex.ci.observer.reader.message.internal.writer.ObserverInternalStreamWriter;
import ru.yandex.ci.observer.reader.proto.ObserverProtoMappers;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;
import ru.yandex.ci.storage.core.message.main.MainStreamMessageProcessor;
import ru.yandex.ci.storage.core.message.main.MainStreamStatistics;
import ru.yandex.ci.storage.core.utils.TimeTraceService;

import static com.google.protobuf.TextFormat.shortDebugString;

@Slf4j
public class ObserverMainStreamMessageProcessor extends MainStreamMessageProcessor {
    private final ObserverCheckService checkService;
    private final MainStreamStatistics statistics;
    private final ObserverInternalStreamWriter internalStreamWriter;

    public ObserverMainStreamMessageProcessor(
            ObserverCheckService checkService,
            MainStreamStatistics statistics,
            ObserverInternalStreamWriter internalStreamWriter
    ) {
        this.checkService = checkService;
        this.statistics = statistics;
        this.internalStreamWriter = internalStreamWriter;
    }

    @Override
    public void process(List<TaskMessages.TaskMessage> protoTaskMessages, TimeTraceService.Trace trace) {
        protoTaskMessages = protoTaskMessages.stream()
                .filter(m -> m.getMessagesCase() != TaskMessages.TaskMessage.MessagesCase.AUTOCHECK_TEST_RESULTS)
                .collect(Collectors.toList());

        var taskMessages = toTaskMessageWithId(protoTaskMessages);
        if (taskMessages.isEmpty()) {
            log.info("All messages skipped");
            return;
        }

        var messagesByCase = taskMessages.stream()
                .collect(Collectors.groupingBy(m -> m.getMessage().getMessagesCase()));

        log.info(
                "Task messages by type: {}",
                messagesByCase.entrySet().stream()
                        .map(x -> "%s - %d".formatted(x.getKey(), x.getValue().size()))
                        .collect(Collectors.joining(", "))
        );

        processFatalError(
                messagesByCase.getOrDefault(TaskMessages.TaskMessage.MessagesCase.AUTOCHECK_FATAL_ERROR, List.of())
        );
        processPartitionFinish(
                messagesByCase.getOrDefault(TaskMessages.TaskMessage.MessagesCase.FINISHED, List.of())
        );
        processPessimizeMessage(
                messagesByCase.getOrDefault(TaskMessages.TaskMessage.MessagesCase.PESSIMIZE, List.of())
        );

        processTechnicalStatisticsMessage(
                messagesByCase.getOrDefault(TaskMessages.TaskMessage.MessagesCase.METRIC, List.of())
        );

        processTrace(
                messagesByCase.getOrDefault(TaskMessages.TaskMessage.MessagesCase.TRACE_STAGE, List.of())
        );

        for (var entry : messagesByCase.entrySet()) {
            this.statistics.onMessageProcessed(entry.getKey(), entry.getValue().size());
        }
    }

    private void processTrace(List<TaskMessageWithId> messages) {
        if (messages.isEmpty()) {
            return;
        }

        Map<CheckIterationEntity.Id, Set<CheckTaskEntity.Id>> checkTasksToAggregate = new HashMap<>();

        for (var message : messages) {
            var aggregationNeeded = checkService.processPartitionTrace(
                    message.getId(), message.getMessage().getPartition(), message.getMessage().getTraceStage()
            );

            if (aggregationNeeded) {
                checkTasksToAggregate.computeIfAbsent(message.getId().getIterationId(), k -> new HashSet<>())
                        .add(message.getId());
            }
        }

        if (!checkTasksToAggregate.isEmpty()) {
            log.info("Writing tasks to aggregate to internal stream: {}", checkTasksToAggregate);
            internalStreamWriter.onCheckTasksAggregation(checkTasksToAggregate);
        }
    }

    private void processPartitionFinish(List<TaskMessageWithId> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var partitionTaskIds = messages.stream()
                .collect(Collectors.groupingBy(
                        m -> m.getId().getIterationId(),
                        Collectors.mapping(
                                m -> new FullTaskIdWithPartition(
                                        m.getMessage().getFullTaskId(), m.getMessage().getPartition()
                                ),
                                Collectors.toList()
                        )
                ));

        log.info("Writing finish task partitions to internal stream: {}", partitionTaskIds);
        internalStreamWriter.onCheckTaskPartitionsFinish(partitionTaskIds);
    }

    private void processFatalError(List<TaskMessageWithId> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var taskIds = messages.stream()
                .map(m -> m.getMessage().getFullTaskId())
                .collect(Collectors.toList());

        log.info("Writing fatal error tasks to internal stream: {}", taskIds);
        internalStreamWriter.onFatalError(taskIds);
    }

    private void processTechnicalStatisticsMessage(List<TaskMessageWithId> messages) {
        var byTaskIdPartition = messages.stream()
                .map(TaskMessageWithId::getMessage)
                .collect(Collectors.groupingBy(
                        m -> new FullTaskIdWithPartition(m.getFullTaskId(), m.getPartition()),
                        Collectors.reducing(
                                new TechnicalStatistics(0, 0, 0),
                                m -> {
                                    var metric = m.getMetric();
                                    var value = metric.getValue();

                                    var builder = TechnicalStatistics.builder();
                                    switch (metric.getName()) {
                                        case Metrics.Name.MACHINE_HOURS -> builder.machineHours(value);
                                        case Metrics.Name.NUMBER_OF_NODES -> builder.totalNumberOfNodes((int) value);
                                        case Metrics.Name.CACHE_HIT -> builder.cacheHit(value);
                                        default -> {
                                            // noop
                                        }
                                    }

                                    return builder.build();
                                },
                                TechnicalStatistics::max
                        )
                ));

        if (!byTaskIdPartition.isEmpty()) {
            internalStreamWriter.onIterationsTechnicalStats(byTaskIdPartition);
        }
    }

    private void processPessimizeMessage(List<TaskMessageWithId> messages) {
        var iterationIds = messages.stream()
                .map(m -> m.getId().getIterationId())
                .collect(Collectors.toList());

        if (!iterationIds.isEmpty()) {
            internalStreamWriter.onIterationsPessimize(iterationIds);
        }
    }

    public List<TaskMessageWithId> toTaskMessageWithId(List<TaskMessages.TaskMessage> protoTaskMessages) {
        var taskMessages = new ArrayList<TaskMessageWithId>(protoTaskMessages.size());
        for (var message : protoTaskMessages) {
            var fullTaskId = message.getFullTaskId();

            try {
                Preconditions.checkNotNull(fullTaskId, "FullTaskId missing");
                Preconditions.checkNotNull(fullTaskId.getIterationId(), "IterationId missing");
                Preconditions.checkState(!fullTaskId.getTaskId().isEmpty(), "TaskId missing");
            } catch (NullPointerException | IllegalStateException e) {
                this.statistics.onValidationError();
                log.warn("Skipping validation error: {}, fullTaskId: {}, message: {}",
                        e.getMessage(), shortDebugString(fullTaskId), message.getMessagesCase());
                continue;
            }

            taskMessages.add(new TaskMessageWithId(ObserverProtoMappers.toTaskId(fullTaskId), message));
        }
        return taskMessages;
    }

    @Value
    public static class TaskMessageWithId {
        CheckTaskEntity.Id id;
        TaskMessages.TaskMessage message;
    }
}
