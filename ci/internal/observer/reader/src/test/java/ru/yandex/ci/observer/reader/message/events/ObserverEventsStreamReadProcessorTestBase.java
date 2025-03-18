package ru.yandex.ci.observer.reader.message.events;

import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;

import org.mockito.Mockito;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageBatch;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;

import static org.mockito.Mockito.when;

public class ObserverEventsStreamReadProcessorTestBase {
    static final Instant TIME = Instant.parse("2021-10-20T12:00:00Z");
    static final Map<EventsStreamMessages.EventsStreamMessage.MessagesCase,
            Consumer<EventsStreamMessages.EventsStreamMessage.Builder>> ENQUEUEING_MESSAGES_SETTER = Map.of(
                    EventsStreamMessages.EventsStreamMessage.MessagesCase.REGISTRATION,
                    ObserverEventsStreamReadProcessorTestBase::setGeneratedRegistrationMessage,
                    EventsStreamMessages.EventsStreamMessage.MessagesCase.CANCEL,
                    ObserverEventsStreamReadProcessorTestBase::setGeneratedCancelMessage,
                    EventsStreamMessages.EventsStreamMessage.MessagesCase.ITERATION_FINISH,
                    ObserverEventsStreamReadProcessorTestBase::setGeneratedIterationFinishMessage,
                    EventsStreamMessages.EventsStreamMessage.MessagesCase.TRACE,
                    ObserverEventsStreamReadProcessorTestBase::setGeneratedTraceMessage
    );

    LogbrokerPartitionRead createLbPartitionRead(
            EventsStreamMessages.EventsStreamMessage.MessagesCase messagesCase,
            Consumer<EventsStreamMessages.EventsStreamMessage.Builder> messageSetter
    ) {
        var eventsMessage = createEventsStreamMessage(messagesCase, messageSetter);

        var messageData = Mockito.mock(MessageData.class);
        when(messageData.getDecompressedData()).thenReturn(eventsMessage.toByteArray());

        var lbCommitCountdown = Mockito.mock(LbCommitCountdown.class);
        when(lbCommitCountdown.getNumberOfMessages()).thenReturn(1);
        return new LogbrokerPartitionRead(
                0,
                lbCommitCountdown,
                List.of(new MessageBatch("test_topic", 0, List.of(messageData)))
        );
    }

    EventsStreamMessages.EventsStreamMessage createEventsStreamMessage(
            EventsStreamMessages.EventsStreamMessage.MessagesCase messagesCase,
            Consumer<EventsStreamMessages.EventsStreamMessage.Builder> messageSetter
    ) {
        var eventsMessageBuilder = EventsStreamMessages.EventsStreamMessage.newBuilder();
        messageSetter.accept(eventsMessageBuilder);
        return eventsMessageBuilder.build();
    }

    private static void setGeneratedRegistrationMessage(EventsStreamMessages.EventsStreamMessage.Builder wrapper) {
        wrapper.setMeta(
                Common.MessageMeta.newBuilder()
                        .setTimestamp(ProtoConverter.convert(TIME))
                        .build()
        ).setRegistration(
                EventsStreamMessages.RegistrationMessage.newBuilder()
                        .setCheck(CheckOuterClass.Check.newBuilder().setId("123").build())
                        .build()
        );
    }

    private static void setGeneratedCancelMessage(EventsStreamMessages.EventsStreamMessage.Builder wrapper) {
        wrapper.setCancel(
                EventsStreamMessages.CancelMessage.newBuilder()
                        .setIterationId(
                                CheckIteration.IterationId.newBuilder()
                                        .setCheckId("123")
                                        .setCheckType(CheckIteration.IterationType.FULL)
                                        .setNumber(1)
                                        .build()
                        )
                        .build()
        );
    }

    private static void setGeneratedIterationFinishMessage(EventsStreamMessages.EventsStreamMessage.Builder wrapper) {
        wrapper.setIterationFinish(
                EventsStreamMessages.StorageIterationFinishMessage.newBuilder()
                        .setIterationId(
                                CheckIteration.IterationId.newBuilder()
                                        .setCheckId("123")
                                        .setCheckType(CheckIteration.IterationType.FULL)
                                        .setNumber(1)
                                        .build()
                        )
                        .build()
        );
    }

    private static void setGeneratedTraceMessage(EventsStreamMessages.EventsStreamMessage.Builder wrapper) {
        wrapper.setTrace(
                EventsStreamMessages.StorageTraceStage.newBuilder()
                        .setFullTaskId(
                                CheckTaskOuterClass.FullTaskId.newBuilder()
                                        .setIterationId(
                                                CheckIteration.IterationId.newBuilder()
                                                        .setCheckId("123")
                                                        .setCheckType(CheckIteration.IterationType.FULL)
                                                        .setNumber(1)
                                                        .build()
                                        )
                                        .setTaskId("456")
                                        .build()
                        )
                        .setTrace(
                                Common.TraceStage.newBuilder()
                                        .setType("sample")
                                        .setTimestamp(ProtoConverter.convert(TIME))
                                        .build()
                        )
                        .build()
        );
    }
}
