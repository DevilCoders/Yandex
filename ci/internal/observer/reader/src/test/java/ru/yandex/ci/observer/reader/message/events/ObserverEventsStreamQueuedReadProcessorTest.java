package ru.yandex.ci.observer.reader.message.events;

import java.util.List;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import ru.yandex.ci.observer.reader.check.ObserverCheckService;
import ru.yandex.ci.observer.reader.message.internal.writer.ObserverInternalStreamWriter;
import ru.yandex.ci.observer.reader.registration.ObserverRegistrationProcessor;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;

class ObserverEventsStreamQueuedReadProcessorTest extends ObserverEventsStreamReadProcessorTestBase {
    @Mock
    ObserverCheckService checkService;
    @Mock
    ObserverInternalStreamWriter internalStreamWriter;
    @Mock
    EventsStreamStatistics statistics;
    @Mock
    ObserverRegistrationProcessor registrationProcessor;

    private ObserverEventsStreamQueuedReadProcessor readProcessor;

    @BeforeEach
    void setup() {
        MockitoAnnotations.openMocks(this);

        readProcessor = new ObserverEventsStreamQueuedReadProcessor(
                checkService, internalStreamWriter, statistics, registrationProcessor, 1, 1, true
        );
    }

    @Test
    void process_CommitRegisteredMessageCases() {
        for (var entry : ENQUEUEING_MESSAGES_SETTER.entrySet()) {
            var lbPartitionRead = createLbPartitionRead(entry.getKey(), entry.getValue());
            var readEventMessage = new ReadEventStreamMessage(
                    lbPartitionRead, createEventsStreamMessage(entry.getKey(), entry.getValue())
            );

            readProcessor.process(List.of(readEventMessage));

            Mockito.verify(lbPartitionRead.getCommitCountdown(), Mockito.times(1)).notifyMessageProcessed(1);
            Mockito.verifyNoMoreInteractions(lbPartitionRead.getCommitCountdown());
        }
    }

    @Test
    void process_RegistrationMessage() {
        var readEventMessage = createReadEventStreamMessage(
                EventsStreamMessages.EventsStreamMessage.MessagesCase.REGISTRATION
        );

        readProcessor.process(List.of(readEventMessage));

        Mockito.verify(registrationProcessor, Mockito.times(1)).processMessages(
                Mockito.eq(List.of(readEventMessage.getMessage()))
        );
        Mockito.verifyNoMoreInteractions(checkService, internalStreamWriter, registrationProcessor);
    }

    @Test
    void process_TraceMessage() {
        var readEventMessage = createReadEventStreamMessage(
                EventsStreamMessages.EventsStreamMessage.MessagesCase.TRACE
        );

        readProcessor.process(List.of(readEventMessage));

        Mockito.verify(checkService, Mockito.times(1)).processStoragePartitionTrace(Mockito.any(), Mockito.any());
        Mockito.verify(registrationProcessor, Mockito.times(1)).processMessages(List.of());
        Mockito.verifyNoMoreInteractions(checkService, internalStreamWriter, registrationProcessor);
    }

    @Test
    void process_TraceMessage_WhenNeedAggregate() {
        Mockito.when(checkService.processStoragePartitionTrace(Mockito.any(), Mockito.any())).thenReturn(true);
        var readEventMessage = createReadEventStreamMessage(
                EventsStreamMessages.EventsStreamMessage.MessagesCase.TRACE
        );

        readProcessor.process(List.of(readEventMessage));

        Mockito.verify(checkService, Mockito.times(1)).processStoragePartitionTrace(Mockito.any(), Mockito.any());
        Mockito.verify(internalStreamWriter, Mockito.times(1)).onCheckTasksAggregation(Mockito.anyMap());
        Mockito.verify(registrationProcessor, Mockito.times(1)).processMessages(List.of());
        Mockito.verifyNoMoreInteractions(checkService, internalStreamWriter, registrationProcessor);
    }

    @Test
    void process_CancelMessage() {
        var readEventMessage = createReadEventStreamMessage(
                EventsStreamMessages.EventsStreamMessage.MessagesCase.CANCEL
        );

        readProcessor.process(List.of(readEventMessage));

        Mockito.verify(internalStreamWriter, Mockito.times(1)).onIterationsCancel(Mockito.anyList());
        Mockito.verify(registrationProcessor, Mockito.times(1)).processMessages(List.of());
        Mockito.verifyNoMoreInteractions(checkService, internalStreamWriter, registrationProcessor);
    }

    @Test
    void process_IterationFinishMessage() {
        var readEventMessage = createReadEventStreamMessage(
                EventsStreamMessages.EventsStreamMessage.MessagesCase.ITERATION_FINISH
        );

        readProcessor.process(List.of(readEventMessage));

        Mockito.verify(internalStreamWriter, Mockito.times(1)).onIterationFinish(Mockito.anyMap());
        Mockito.verify(registrationProcessor, Mockito.times(1)).processMessages(List.of());
        Mockito.verifyNoMoreInteractions(checkService, internalStreamWriter, registrationProcessor);
    }

    private ReadEventStreamMessage createReadEventStreamMessage(
            EventsStreamMessages.EventsStreamMessage.MessagesCase messagesCase
    ) {
        var lbPartitionRead = createLbPartitionRead(messagesCase, ENQUEUEING_MESSAGES_SETTER.get(messagesCase));
        return new ReadEventStreamMessage(
                lbPartitionRead,
                createEventsStreamMessage(messagesCase, ENQUEUEING_MESSAGES_SETTER.get(messagesCase))
        );
    }
}
