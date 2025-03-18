package ru.yandex.ci.observer.reader.message.internal.writer;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.common.collect.Lists;
import io.micrometer.core.instrument.MeterRegistry;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.observer.core.Internal;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.reader.message.FullTaskIdWithPartition;
import ru.yandex.ci.observer.reader.message.InternalPartitionGenerator;
import ru.yandex.ci.observer.reader.message.IterationFinishStatusWithTime;
import ru.yandex.ci.observer.reader.proto.ObserverInternalMessagesProtoMappers;
import ru.yandex.ci.observer.reader.proto.ObserverProtoMappers;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.StorageMessageWriter;

public class ObserverInternalStreamWriterImpl extends StorageMessageWriter implements ObserverInternalStreamWriter {
    private static final int MAX_NUMBER_OF_MESSAGES_IN_WRITE = 512;

    private final InternalPartitionGenerator partitionGenerator;

    public ObserverInternalStreamWriterImpl(
            MeterRegistry meterRegistry,
            int numberOfPartitions,
            InternalPartitionGenerator partitionGenerator,
            LogbrokerWriterFactory logbrokerWriterFactory
    ) {
        super(meterRegistry, numberOfPartitions, logbrokerWriterFactory);
        this.partitionGenerator = partitionGenerator;
    }

    @Override
    public void onCheckTasksAggregation(Map<CheckIterationEntity.Id, Set<CheckTaskEntity.Id>> tasksToAggregation) {
        var byPartition = tasksToAggregation.entrySet().stream().collect(
                Collectors.groupingBy(e -> partitionGenerator.generatePartition(e.getKey().getCheckId()))
        );

        var messages = tasksToLbMessages(
                byPartition,
                byCheckIterationEntry -> Stream.of(
                        toInternalAggregateStageMessage(
                                byCheckIterationEntry.getKey(), byCheckIterationEntry.getValue()
                        )
                )
        );

        this.writeWithRetries(messages);
    }

    @Override
    public void onIterationsCancel(List<CheckIterationEntity.Id> iterationsToCancel) {
        var byPartition = iterationsToCancel.stream()
                .collect(Collectors.groupingBy(i -> partitionGenerator.generatePartition(i.getCheckId())));

        List<Message> messages = entitiesToLbMessages(
                byPartition, this::toInternalCancelMessage, i -> i.getCheckId().toString()
        );

        this.writeWithRetries(messages);
    }

    @Override
    public void onIterationsTechnicalStats(Map<FullTaskIdWithPartition, TechnicalStatistics> taskPartitionStats) {
        var byPartition = taskPartitionStats.entrySet().stream().collect(
                Collectors.groupingBy(e -> partitionGenerator.generatePartition(
                        CheckEntity.Id.of(e.getKey().getFullTaskId().getIterationId().getCheckId())
                ))
        );

        List<Message> messages = entitiesToLbMessages(
                byPartition,
                e -> toInternalTechnicalStatsMessage(
                        e.getKey().getFullTaskId(), e.getKey().getPartition(), e.getValue()
                ),
                e -> e.getKey().getFullTaskId().getIterationId().getCheckId()
        );

        this.writeWithRetries(messages);
    }

    @Override
    public void onIterationsPessimize(List<CheckIterationEntity.Id> iterationsToPessimize) {
        var byPartition = iterationsToPessimize.stream()
                .collect(Collectors.groupingBy(i -> partitionGenerator.generatePartition(i.getCheckId())));

        List<Message> messages = entitiesToLbMessages(
                byPartition, this::toInternalPessimizeMessage, i -> i.getCheckId().toString()
        );

        this.writeWithRetries(messages);
    }

    @Override
    public void onIterationFinish(Map<CheckIterationEntity.Id, IterationFinishStatusWithTime> iterationFinishStatuses) {
        var byPartition = iterationFinishStatuses.entrySet().stream()
                .collect(Collectors.groupingBy(e -> partitionGenerator.generatePartition(e.getKey().getCheckId())));

        List<Message> messages = entitiesToLbMessages(
                byPartition,
                e -> toInternalIterationFinishMessage(e.getKey(), e.getValue().getStatus(), e.getValue().getTime()),
                e -> e.getKey().getCheckId().toString()
        );

        this.writeWithRetries(messages);
    }

    @Override
    public void onCheckTaskPartitionsFinish(
            Map<CheckIterationEntity.Id, List<FullTaskIdWithPartition>> taskPartitionsToFinish
    ) {
        var byPartition = taskPartitionsToFinish.entrySet().stream().collect(
                Collectors.groupingBy(e -> partitionGenerator.generatePartition(e.getKey().getCheckId()))
        );

        List<Message> messages = tasksToLbMessages(
                byPartition,
                byCheckIterationEntry -> byCheckIterationEntry.getValue().stream().map(
                        t -> toInternalFinishPartitionMessage(
                                byCheckIterationEntry.getKey(), t.getFullTaskId(), t.getPartition()
                        )
                )
        );

        this.writeWithRetries(messages);
    }

    @Override
    public void onRegistered(List<EventsStreamMessages.EventsStreamMessage> registrationMessages) {
        var byPartition = registrationMessages.stream()
                .map(EventsStreamMessages.EventsStreamMessage::getRegistration)
                .filter(m -> m.hasIteration() || m.hasTask())
                .collect(
                        Collectors.groupingBy(m -> partitionGenerator.generatePartition(
                                ObserverProtoMappers.toIterationId(m.getIteration().getId()).getCheckId()
                        ))
                );

        List<Message> messages = entitiesToLbMessages(
                byPartition, this::toInternalRegisteredMessage, m -> getCheckId(m).toString()
        );

        this.writeWithRetries(messages);
    }

    @Override
    public void onFatalError(List<CheckTaskOuterClass.FullTaskId> fatalErrorTaskIds) {
        var byPartition = fatalErrorTaskIds.stream()
                .collect(
                        Collectors.groupingBy(t -> partitionGenerator.generatePartition(
                                CheckEntity.Id.of(t.getIterationId().getCheckId())
                        ))
                );

        List<Message> messages = entitiesToLbMessages(
                byPartition, this::toInternalFatalErrorMessage, t -> t.getIterationId().getCheckId()
        );

        this.writeWithRetries(messages);
    }

    private <T> List<Message> tasksToLbMessages(
            Map<Integer, List<Map.Entry<CheckIterationEntity.Id, T>>> byPartition,
            Function<Map.Entry<CheckIterationEntity.Id, T>, Stream<Internal.InternalMessage>> applier
    ) {
        List<Message> messages = new ArrayList<>();

        for (var partition : byPartition.entrySet()) {
            for (var batch : Lists.partition(partition.getValue(), MAX_NUMBER_OF_MESSAGES_IN_WRITE)) {
                var meta = createMeta();
                messages.add(
                        new Message(
                                meta,
                                partition.getKey(),
                                Internal.InternalMessages.newBuilder()
                                        .setMeta(meta)
                                        .addAllMessages(
                                                batch.stream()
                                                        .flatMap(applier)
                                                        .collect(Collectors.toList())
                                        )
                                        .build(),
                                "Internal for: " + batch.stream()
                                        .map(Map.Entry::getKey)
                                        .map(CheckIterationEntity.Id::toString)
                                        .collect(Collectors.joining(", "))
                        )
                );
            }
        }

        return messages;
    }

    private <T> List<Message> entitiesToLbMessages(
            Map<Integer, List<T>> byPartition,
            Function<T, Internal.InternalMessage> applier,
            Function<T, String> checkIdApplier
    ) {
        List<Message> messages = new ArrayList<>();

        for (var partition : byPartition.entrySet()) {
            for (var batch : Lists.partition(partition.getValue(), MAX_NUMBER_OF_MESSAGES_IN_WRITE)) {
                var meta = createMeta();
                messages.add(
                        new Message(
                                meta,
                                partition.getKey(),
                                Internal.InternalMessages.newBuilder()
                                        .setMeta(meta)
                                        .addAllMessages(
                                                batch.stream()
                                                        .map(applier)
                                                        .collect(Collectors.toList())
                                        )
                                        .build(),
                                "Internal for: " + batch.stream()
                                        .map(checkIdApplier)
                                        .collect(Collectors.joining(", "))
                        )
                );
            }
        }

        return messages;
    }

    private Internal.InternalMessage.Builder toInternalMessageBuilder(CheckEntity.Id checkId) {
        return Internal.InternalMessage.newBuilder()
                .setMeta(createMeta())
                .setCheckId(checkId.getId());
    }

    private Internal.InternalMessage toInternalAggregateStageMessage(
            CheckIterationEntity.Id iterationId,
            Set<CheckTaskEntity.Id> taskIds
    ) {
        return toInternalMessageBuilder(iterationId.getCheckId())
                .setAggregate(ObserverInternalMessagesProtoMappers.toProtoAggregateStages(iterationId, taskIds))
                .build();
    }

    private Internal.InternalMessage toInternalCancelMessage(
            CheckIterationEntity.Id iterationId
    ) {
        return toInternalMessageBuilder(iterationId.getCheckId())
                .setCancel(ObserverInternalMessagesProtoMappers.toProtoCancel(iterationId))
                .build();
    }

    private Internal.InternalMessage toInternalFinishPartitionMessage(
            CheckIterationEntity.Id iterationId,
            CheckTaskOuterClass.FullTaskId taskId,
            int partition
    ) {
        return toInternalMessageBuilder(iterationId.getCheckId())
                .setFinishPartition(ObserverInternalMessagesProtoMappers.toProtoFinishPartition(taskId, partition))
                .build();
    }

    private Internal.InternalMessage toInternalPessimizeMessage(
            CheckIterationEntity.Id iterationId
    ) {
        return toInternalMessageBuilder(iterationId.getCheckId())
                .setPessimize(ObserverInternalMessagesProtoMappers.toProtoPessimize(iterationId))
                .build();
    }

    private Internal.InternalMessage toInternalTechnicalStatsMessage(
            CheckTaskOuterClass.FullTaskId taskId, int partition, TechnicalStatistics technicalStatistics
    ) {
        return toInternalMessageBuilder(CheckEntity.Id.of(taskId.getIterationId().getCheckId()))
                .setTechnicalStatsMessage(
                        ObserverInternalMessagesProtoMappers.toProtoTechnicalStatisticsMessage(
                                taskId, partition, technicalStatistics
                        )
                )
                .build();
    }

    private Internal.InternalMessage toInternalRegisteredMessage(EventsStreamMessages.RegistrationMessage message) {
        CheckEntity.Id checkId = getCheckId(message);

        return toInternalMessageBuilder(checkId)
                .setRegistered(ObserverInternalMessagesProtoMappers.toProtoRegistered(message))
                .build();
    }

    private Internal.InternalMessage toInternalFatalErrorMessage(CheckTaskOuterClass.FullTaskId taskId) {
        return toInternalMessageBuilder(CheckEntity.Id.of(taskId.getIterationId().getCheckId()))
                .setFatalError(ObserverInternalMessagesProtoMappers.toProtoFatalError(taskId))
                .build();
    }

    private Internal.InternalMessage toInternalIterationFinishMessage(
            CheckIterationEntity.Id iterationId,
            Common.CheckStatus status,
            Instant finishTime
    ) {
        return toInternalMessageBuilder(iterationId.getCheckId())
                .setIterationFinish(
                        ObserverInternalMessagesProtoMappers.toProtoIterationFinish(
                                iterationId, status, ProtoConverter.convert(finishTime)
                        )
                )
                .build();
    }

    private CheckEntity.Id getCheckId(EventsStreamMessages.RegistrationMessage message) {
        if (message.hasCheck()) {
            return CheckEntity.Id.of(message.getCheck().getId());
        }
        if (message.hasIteration()) {
            return CheckEntity.Id.of(message.getIteration().getId().getCheckId());
        }
        if (message.hasTask()) {
            return CheckEntity.Id.of(message.getTask().getId().getIterationId().getCheckId());
        }

        throw new RuntimeException(String.format("Invalid empty registration message: %s", message));
    }
}
