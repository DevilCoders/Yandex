package ru.yandex.ci.observer.reader.message.internal;

import java.util.List;
import java.util.Map;
import java.util.function.Consumer;

import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.observer.core.Internal;
import ru.yandex.ci.observer.reader.ObserverReaderYdbTestBase;
import ru.yandex.ci.observer.reader.proto.ObserverProtoMappers;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageBatch;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;

public class ObserverInternalStreamProcessorTestBase extends ObserverReaderYdbTestBase {
    static final Map<Internal.InternalMessage.MessagesCase,
            Consumer<Internal.InternalMessage.Builder>> ENQUEUEING_MESSAGES_SETTER = Map.of(
            Internal.InternalMessage.MessagesCase.CANCEL,
            ObserverInternalStreamProcessorTestBase::setGeneratedCancelMessage,
            Internal.InternalMessage.MessagesCase.ITERATION_FINISH,
            ObserverInternalStreamProcessorTestBase::setGeneratedIterationFinishMessage,
            Internal.InternalMessage.MessagesCase.AGGREGATE,
            ObserverInternalStreamProcessorTestBase::setGeneratedAggregateMessage,
            Internal.InternalMessage.MessagesCase.FATAL_ERROR,
            ObserverInternalStreamProcessorTestBase::setGeneratedFatalErrorMessage,
            Internal.InternalMessage.MessagesCase.FINISH_PARTITION,
            ObserverInternalStreamProcessorTestBase::setGeneratedFinishPartitionMessage,
            Internal.InternalMessage.MessagesCase.PESSIMIZE,
            ObserverInternalStreamProcessorTestBase::setGeneratedPessimizeMessage,
            Internal.InternalMessage.MessagesCase.REGISTERED,
            ObserverInternalStreamProcessorTestBase::setGeneratedRegisteredMessage,
            Internal.InternalMessage.MessagesCase.TECHNICAL_STATS_MESSAGE,
            ObserverInternalStreamProcessorTestBase::setGeneratedTechnicalStatsMessage
    );

    @Autowired
    ObserverEntitiesChecker entitiesChecker;

    Internal.InternalMessage createInternalStreamMessage(
            Internal.InternalMessage.MessagesCase messagesCase
    ) {
        return createInternalStreamMessage(ENQUEUEING_MESSAGES_SETTER.get(messagesCase));
    }

    Internal.InternalMessage createInternalStreamMessage(
            Consumer<Internal.InternalMessage.Builder> consumer
    ) {
        var messageBuilder = Internal.InternalMessage.newBuilder()
                .setCheckId(SAMPLE_CHECK_ID.getId());
        consumer.accept(messageBuilder);
        return messageBuilder.build();
    }

    private static void setGeneratedCancelMessage(Internal.InternalMessage.Builder wrapper) {
        wrapper.setCancel(
                Internal.Cancel.newBuilder()
                        .setIterationId(ObserverProtoMappers.toProtoIterationId(SAMPLE_ITERATION_ID))
                        .build()
        );
    }

    private static void setGeneratedIterationFinishMessage(Internal.InternalMessage.Builder wrapper) {
        wrapper.setIterationFinish(
                Internal.IterationFinish.newBuilder()
                        .setIterationId(ObserverProtoMappers.toProtoIterationId(SAMPLE_ITERATION_ID))
                        .setStatus(SAMPLE_ITERATION.getStatus())
                        .build()
        );
    }

    private static void setGeneratedAggregateMessage(Internal.InternalMessage.Builder wrapper) {
        wrapper.setAggregate(
                Internal.AggregateStages.newBuilder()
                        .setIterationId(ObserverProtoMappers.toProtoIterationId(SAMPLE_ITERATION_ID))
                        .addAllTaskIds(List.of(ObserverProtoMappers.toProtoTaskId(SAMPLE_TASK_ID)))
                        .build()
        );
    }

    private static void setGeneratedFatalErrorMessage(Internal.InternalMessage.Builder wrapper) {
        wrapper.setFatalError(
                Internal.FatalError.newBuilder()
                        .setTaskId(ObserverProtoMappers.toProtoTaskId(SAMPLE_TASK_ID))
                        .build()
        );
    }

    private static void setGeneratedFinishPartitionMessage(Internal.InternalMessage.Builder wrapper) {
        wrapper.setFinishPartition(
                Internal.FinishPartition.newBuilder()
                        .setTaskId(ObserverProtoMappers.toProtoTaskId(SAMPLE_TASK_ID))
                        .setPartition(0)
                        .build()
        );
    }

    private static void setGeneratedPessimizeMessage(Internal.InternalMessage.Builder wrapper) {
        wrapper.setPessimize(
                Internal.Pessimize.newBuilder()
                        .setIterationId(ObserverProtoMappers.toProtoIterationId(SAMPLE_ITERATION_ID))
                        .build()
        );
    }

    private static void setGeneratedRegisteredMessage(Internal.InternalMessage.Builder wrapper) {
        wrapper.setRegistered(
                Internal.Registered.newBuilder()
                        .setIterationId(ObserverProtoMappers.toProtoIterationId(SAMPLE_ITERATION_ID))
                        .setTaskId(ObserverProtoMappers.toProtoTaskId(SAMPLE_TASK_ID))
                        .build()
        );
    }

    private static void setGeneratedTechnicalStatsMessage(Internal.InternalMessage.Builder wrapper) {
        wrapper.setTechnicalStatsMessage(
                Internal.TechnicalStatisticsMessage.newBuilder()
                        .setTaskId(ObserverProtoMappers.toProtoTaskId(SAMPLE_TASK_ID))
                        .setPartition(0)
                        .setTechnicalStatistics(
                                Internal.TechnicalStatistics.newBuilder()
                                        .setMachineHours(1.0)
                                        .setCacheHit(2.0)
                                        .setNumberOfNodes(100)
                                        .build()
                        )
                        .build()
        );
    }

    LogbrokerPartitionRead createLbPartitionRead(Internal.InternalMessage.MessagesCase messagesCase) {
        return createLbPartitionRead(ENQUEUEING_MESSAGES_SETTER.get(messagesCase));
    }

    LogbrokerPartitionRead createLbPartitionRead(
            Consumer<Internal.InternalMessage.Builder> consumer
    ) {
        var internalMessages = Internal.InternalMessages.newBuilder()
                .addAllMessages(List.of(createInternalStreamMessage(consumer)))
                .build();

        var messageData = Mockito.mock(MessageData.class);
        Mockito.when(messageData.getDecompressedData()).thenReturn(internalMessages.toByteArray());

        return new LogbrokerPartitionRead(
                0,
                Mockito.mock(LbCommitCountdown.class),
                List.of(new MessageBatch("test_topic", 0, List.of(messageData)))
        );
    }
}
