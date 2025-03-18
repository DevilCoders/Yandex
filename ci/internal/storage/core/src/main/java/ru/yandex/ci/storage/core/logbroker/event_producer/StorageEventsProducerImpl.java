package ru.yandex.ci.storage.core.logbroker.event_producer;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import com.google.common.base.Preconditions;
import io.micrometer.core.instrument.MeterRegistry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.StorageMessageWriter;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;

public class StorageEventsProducerImpl extends StorageMessageWriter implements StorageEventsProducer {
    private static final Logger log = LoggerFactory.getLogger(StorageEventsProducerImpl.class);

    public StorageEventsProducerImpl(
            int numberOfPartitions,
            MeterRegistry meterRegistry,
            LogbrokerWriterFactory logbrokerWriterFactory
    ) {
        super(meterRegistry, numberOfPartitions, logbrokerWriterFactory);
    }

    @Override
    public void onIterationRegistered(CheckOuterClass.Check check, CheckIteration.Iteration iteration) {
        try {
            onIterationRegisteredInternal(check, iteration);
            log.info("Iteration registration {} sent to logbroker", iteration.getId());
        } catch (RuntimeException e) {
            log.error("Failed to send iteration registration message for iteration " + iteration.getId(), e);
            throw e;
        }
    }

    @Override
    public void onTasksRegistered(
            CheckOuterClass.Check check, CheckIteration.Iteration iteration, List<CheckTaskOuterClass.CheckTask> tasks
    ) {
        var taskIds = tasks.stream().map(CheckTaskOuterClass.CheckTask::getId).collect(Collectors.toList());
        try {
            onRegisteredInternal(check, iteration, tasks);
            log.info("Tasks registration sent to logbroker, tasks: {}", taskIds);
        } catch (RuntimeException e) {
            log.error("Failed to send task registration message for tasks " + taskIds, e);
            throw e;
        }
    }

    @Override
    public void onCancelRequested(CheckIterationEntity.Id iterationId) {
        Preconditions.checkNotNull(iterationId.getCheckId().getId());
        var message = EventsStreamMessages.EventsStreamMessage.newBuilder()
                .setMeta(createMeta())
                .setCancel(CheckProtoMappers.toProtoCancelMessage(iterationId))
                .build();

        this.writeWithRetries(
                List.of(
                        new StorageMessageWriter.Message(
                                message.getMeta(),
                                getPartition(iterationId.getCheckId().getId()),
                                message, "Cancel " + iterationId
                        )
                )
        );
    }

    @Override
    public void onTaskFinished(CheckTaskEntity.Id taskId) {
        try {
            var trace = EventsStreamMessages.EventsStreamMessage.newBuilder()
                    .setMeta(createMeta())
                    .setTrace(
                            EventsStreamMessages.StorageTraceStage.newBuilder()
                                    .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                                    .setTrace(
                                            Common.TraceStage.newBuilder()
                                                    .setTimestamp(ProtoConverter.convert(Instant.now()))
                                                    .setType("storage/check_task_finished")
                                                    .build()
                                    )
                    )
                    .build();
            this.writeWithRetries(
                    List.of(
                            new StorageMessageWriter.Message(
                                    trace.getMeta(),
                                    getPartition(taskId.getIterationId().getCheckId().getId()),
                                    trace,
                                    "Task finished trace " + taskId
                            )
                    )
            );
            log.info("Task finish trace {} sent to logbroker", taskId);
        } catch (RuntimeException e) {
            log.error("Failed to send finish trace message for task " + taskId, e);
            throw e;
        }
    }

    @Override
    public void onIterationFinished(CheckIterationEntity.Id iterationId, Common.CheckStatus iterationStatus) {
        try {
            var iterationFinish = EventsStreamMessages.EventsStreamMessage.newBuilder()
                    .setMeta(createMeta())
                    .setIterationFinish(
                            EventsStreamMessages.StorageIterationFinishMessage.newBuilder()
                                    .setIterationId(CheckProtoMappers.toProtoIterationId(iterationId))
                                    .setStatus(iterationStatus)
                                    .build()
                    )
                    .build();
            this.writeWithRetries(
                    List.of(
                            new StorageMessageWriter.Message(
                                    iterationFinish.getMeta(),
                                    getPartition(iterationId.getCheckId().getId()),
                                    iterationFinish,
                                    "Iteration %s finish message".formatted(iterationId)
                            )
                    )
            );
            log.info("Iteration {} finish message with status {} sent to logbroker", iterationId, iterationStatus);
        } catch (RuntimeException e) {
            log.error("Failed to send iteration finish message for iteration " + iterationId, e);
            throw e;
        }
    }

    public void onIterationRegisteredInternal(CheckOuterClass.Check check, CheckIteration.Iteration iteration) {
        var message = EventsStreamMessages.EventsStreamMessage.newBuilder()
                .setMeta(createMeta())
                .setRegistration(
                        EventsStreamMessages.RegistrationMessage.newBuilder()
                                .setCheck(check)
                                .setIteration(iteration)
                                .build()
                )
                .build();

        this.writeWithRetries(
                List.of(
                        new StorageMessageWriter.Message(
                                message.getMeta(),
                                getPartition(Long.parseLong(check.getId())),
                                message,
                                "iteration registered" + iteration.getId()
                        )
                )
        );
    }

    public void onRegisteredInternal(
            CheckOuterClass.Check check, CheckIteration.Iteration iteration, List<CheckTaskOuterClass.CheckTask> tasks
    ) {

        List<Message> messages = new ArrayList<>(tasks.size() * 2);

        for (var task : tasks) {
            var message = EventsStreamMessages.EventsStreamMessage.newBuilder()
                    .setMeta(createMeta())
                    .setRegistration(
                            EventsStreamMessages.RegistrationMessage.newBuilder()
                                    .setCheck(check)
                                    .setIteration(iteration)
                                    .setTask(task)
                                    .build()
                    )
                    .build();

            var trace = EventsStreamMessages.EventsStreamMessage.newBuilder()
                    .setMeta(createMeta())
                    .setTrace(
                            EventsStreamMessages.StorageTraceStage.newBuilder()
                                    .setFullTaskId(task.getId())
                                    .setTrace(
                                            Common.TraceStage.newBuilder()
                                                    .setTimestamp(ProtoConverter.convert(Instant.now()))
                                                    .setType("storage/check_task_created")
                                                    .build()
                                    )
                    )
                    .build();
            messages.add(new StorageMessageWriter.Message(
                    message.getMeta(),
                    getPartition(Long.parseLong(check.getId())),
                    message,
                    "Check and iteration registered " + iteration.getId()
            ));
            messages.add(new StorageMessageWriter.Message(
                    trace.getMeta(),
                    getPartition(Long.parseLong(check.getId())),
                    trace,
                    "Check and iteration registered trace" + iteration.getId()
            ));
        }

        this.writeWithRetries(messages);
    }

    @Override
    public void onCheckFatalError(
            CheckEntity.Id checkId,
            List<CheckIterationEntity> runningIterations,
            CheckIterationEntity brokenIteration
    ) {
        var messages = new ArrayList<Message>(runningIterations.size());

        for (var iteration : runningIterations) {
            var message = EventsStreamMessages.EventsStreamMessage.newBuilder()
                    .setMeta(createMeta())
                    .setCancel(CheckProtoMappers.toProtoCancelMessage(iteration.getId()))
                    .build();

            messages.add(new StorageMessageWriter.Message(
                    message.getMeta(),
                    getPartition(checkId.getId()),
                    message,
                    "Cancel " + iteration.getId()
            ));
        }

        if (!messages.isEmpty()) {
            this.writeWithRetries(messages);
        }
    }

    private int getPartition(Long id) {
        return (int) (id % this.numberOfPartitions);
    }
}
