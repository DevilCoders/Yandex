package ru.yandex.ci.observer.reader.message.events;

import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.stream.Collectors;

import com.google.common.annotations.VisibleForTesting;
import io.micrometer.core.instrument.Gauge;
import lombok.Getter;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.reader.check.ObserverCheckService;
import ru.yandex.ci.observer.reader.message.IterationFinishStatusWithTime;
import ru.yandex.ci.observer.reader.message.ObserverEntitiesIdChecker;
import ru.yandex.ci.observer.reader.message.internal.writer.ObserverInternalStreamWriter;
import ru.yandex.ci.observer.reader.proto.ObserverProtoMappers;
import ru.yandex.ci.observer.reader.registration.ObserverRegistrationProcessor;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.util.queue.QueueExecutor;
import ru.yandex.ci.util.queue.QueueWorker;
import ru.yandex.ci.util.queue.SyncQueueExecutor;
import ru.yandex.ci.util.queue.ThreadPerQueueExecutor;

@Slf4j
public class ObserverEventsStreamQueuedReadProcessor {
    private static final String NAME = "events-queued-processor";

    private final ObserverCheckService checkService;
    private final ObserverInternalStreamWriter internalStreamWriter;
    private final EventsStreamStatistics statistics;
    private final ObserverRegistrationProcessor registrationProcessor;
    private final int drainLimit;
    @Getter
    private final int queueMaxNumber;
    private final QueueExecutor<ReadEventStreamMessage> executor;

    public ObserverEventsStreamQueuedReadProcessor(
            ObserverCheckService checkService,
            ObserverInternalStreamWriter internalStreamWriter,
            EventsStreamStatistics statistics,
            ObserverRegistrationProcessor registrationProcessor,
            int drainLimit,
            int queueMaxNumber,
            boolean syncMode
    ) {
        this.executor = syncMode ?
                new SyncQueueExecutor<>(this::process) :
                new ThreadPerQueueExecutor<>(NAME, this::createQueueWorker);

        this.checkService = checkService;
        this.internalStreamWriter = internalStreamWriter;
        this.statistics = statistics;
        this.registrationProcessor = registrationProcessor;
        this.drainLimit = drainLimit;
        this.queueMaxNumber = queueMaxNumber;

        statistics.register(
                Gauge.builder(NAME + "-queue_size", this.executor::getQueueSize)
                        .tag(StorageMetrics.QUEUE, "all")
        );
    }

    public void enqueue(int queueNumber, ReadEventStreamMessage message) {
        this.executor.enqueue(String.valueOf(queueNumber), message);
    }

    private QueueWorker<ReadEventStreamMessage> createQueueWorker(
            String queueName, LinkedBlockingQueue<ReadEventStreamMessage> queue
    ) {
        return new Worker(queue, drainLimit);
    }

    @VisibleForTesting
    void process(List<ReadEventStreamMessage> messages) {
        var byCase = messages.stream()
                .collect(Collectors.groupingBy(
                        m -> m.getMessage().getMessagesCase(),
                        Collectors.mapping(ReadEventStreamMessage::getMessage, Collectors.toList())
                ));

        registrationProcessor.processMessages(
                byCase.getOrDefault(EventsStreamMessages.EventsStreamMessage.MessagesCase.REGISTRATION, List.of())
        );

        this.processCancel(
                byCase.getOrDefault(EventsStreamMessages.EventsStreamMessage.MessagesCase.CANCEL, List.of())
        );

        this.processTrace(
                byCase.getOrDefault(EventsStreamMessages.EventsStreamMessage.MessagesCase.TRACE, List.of())
        );

        this.processIterationFinish(
                byCase.getOrDefault(EventsStreamMessages.EventsStreamMessage.MessagesCase.ITERATION_FINISH, List.of())
        );

        byCase.forEach((c, m) -> statistics.onMessageProcessed(c, m.size()));
        messages.stream()
                .collect(Collectors.groupingBy(ReadEventStreamMessage::getRead, Collectors.counting()))
                .forEach((c, num) -> c.getCommitCountdown().notifyMessageProcessed(num.intValue()));
    }

    private void processTrace(List<EventsStreamMessages.EventsStreamMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        Map<CheckIterationEntity.Id, Set<CheckTaskEntity.Id>> checkTasksToAggregate = new HashMap<>();

        for (var message : messages) {
            if (ObserverEntitiesIdChecker.isMetaIteration(message.getTrace().getFullTaskId().getIterationId())) {
                log.info("Skipping meta iteration {} trace", message.getTrace().getFullTaskId().getIterationId());
                continue;
            }

            var taskId = ObserverProtoMappers.toTaskId(message.getTrace().getFullTaskId());

            if (checkService.processStoragePartitionTrace(taskId, message.getTrace().getTrace())) {
                checkTasksToAggregate.computeIfAbsent(taskId.getIterationId(), k -> new HashSet<>()).add(taskId);
            }
        }

        if (!checkTasksToAggregate.isEmpty()) {
            internalStreamWriter.onCheckTasksAggregation(checkTasksToAggregate);
        }
    }

    private void processCancel(List<EventsStreamMessages.EventsStreamMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var iterationIds = messages.stream()
                .map(m -> ObserverProtoMappers.toIterationId(m.getCancel().getIterationId()))
                .filter(id -> {
                    if (ObserverEntitiesIdChecker.isMetaIteration(id)) {
                        log.info("Skipping meta iteration {} cancel", id);
                        return false;
                    }

                    return true;
                })
                .collect(Collectors.toList());

        internalStreamWriter.onIterationsCancel(iterationIds);
    }

    private void processIterationFinish(List<EventsStreamMessages.EventsStreamMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var iterationsFinish = messages.stream()
                .filter(m -> {
                    var id = ObserverProtoMappers.toIterationId(m.getIterationFinish().getIterationId());
                    if (ObserverEntitiesIdChecker.isMetaIteration(id)) {
                        log.info("Skipping meta iteration {} cancel", id);
                        return false;
                    }

                    return true;
                })
                .collect(
                        Collectors.toMap(
                                m -> ObserverProtoMappers.toIterationId(m.getIterationFinish().getIterationId()),
                                m -> new IterationFinishStatusWithTime(
                                        ObserverProtoMappers.toIterationId(m.getIterationFinish().getIterationId()),
                                        m.getIterationFinish().getStatus(),
                                        ProtoConverter.convert(m.getMeta().getTimestamp())
                                ),
                                (s1, s2) -> s1
                        )
                );

        internalStreamWriter.onIterationFinish(iterationsFinish);
    }


    class Worker extends QueueWorker<ReadEventStreamMessage> {
        Worker(BlockingQueue<ReadEventStreamMessage> queue, int drainLimit) {
            super(queue, drainLimit);
        }

        @Override
        public void onDrain(int amount) {
        }

        @Override
        public void process(List<ReadEventStreamMessage> messages) {
            ObserverEventsStreamQueuedReadProcessor.this.process(messages);
        }

        @Override
        public void onFailed() {
        }
    }
}
