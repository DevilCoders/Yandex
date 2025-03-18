package ru.yandex.ci.observer.reader.message.events;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;

class ObserverEventsStreamReadProcessorTest extends ObserverEventsStreamReadProcessorTestBase {
    private static final Map<EventsStreamMessages.EventsStreamMessage.MessagesCase,
            Consumer<EventsStreamMessages.EventsStreamMessage.Builder>> NOT_ENQUEUEING_MESSAGES_SETTER = Map.of(
                    EventsStreamMessages.EventsStreamMessage.MessagesCase.MESSAGES_NOT_SET, wrapper -> { }
    );

    @Mock
    EventsStreamStatistics statistics;
    @Mock
    ObserverEventsStreamQueuedReadProcessor queuedReadProcessor;

    private ObserverEventsStreamReadProcessor readProcessor;

    @BeforeEach
    void setup() {
        MockitoAnnotations.openMocks(this);
        Mockito.when(queuedReadProcessor.getQueueMaxNumber()).thenReturn(6);

        readProcessor = new ObserverEventsStreamReadProcessor(statistics, queuedReadProcessor);
    }

    @Test
    void checkAllExistingMessageCasesRegistered() {
        var expectedMessageCases = new HashSet<EventsStreamMessages.EventsStreamMessage.MessagesCase>();
        expectedMessageCases.addAll(ENQUEUEING_MESSAGES_SETTER.keySet());
        expectedMessageCases.addAll(NOT_ENQUEUEING_MESSAGES_SETTER.keySet());

        var actualMessageCases = Arrays.stream(EventsStreamMessages.EventsStreamMessage.MessagesCase.values())
                .collect(Collectors.toSet());

        Assertions.assertEquals(expectedMessageCases, actualMessageCases);
    }

    @Test
    void process_EnqueueRegisteredMessageCases() {
        for (var entry : ENQUEUEING_MESSAGES_SETTER.entrySet()) {
            var lbPartitionRead = createLbPartitionRead(entry.getKey(), entry.getValue());
            var readEventMessage = new ReadEventStreamMessage(
                    lbPartitionRead, createEventsStreamMessage(entry.getKey(), entry.getValue())
            );

            readProcessor.process(List.of(lbPartitionRead));

            Mockito.verify(queuedReadProcessor, Mockito.times(1))
                    .enqueue(Mockito.anyInt(), Mockito.eq(readEventMessage));
            Mockito.verify(lbPartitionRead.getCommitCountdown()).getCookie();
            Mockito.verifyNoMoreInteractions(lbPartitionRead.getCommitCountdown());
        }
    }

    @Test
    void process_CommitNotEnqueueingMessageCases() {
        for (var entry : NOT_ENQUEUEING_MESSAGES_SETTER.entrySet()) {
            var lbPartitionRead = createLbPartitionRead(entry.getKey(), entry.getValue());

            readProcessor.process(List.of(lbPartitionRead));

            Mockito.verify(lbPartitionRead.getCommitCountdown(), Mockito.times(1)).notifyMessageProcessed(1);
            Mockito.verify(lbPartitionRead.getCommitCountdown()).getCookie();
            Mockito.verifyNoMoreInteractions(lbPartitionRead.getCommitCountdown());
        }
    }
}
