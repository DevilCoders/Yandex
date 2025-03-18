package ru.yandex.ci.observer.reader.message.internal;

import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.observer.core.Internal;
import ru.yandex.ci.observer.reader.message.internal.cache.LoadedPartitionEntitiesCache;

public class ObserverInternalStreamQueuedReadProcessorTest extends ObserverInternalStreamProcessorTestBase {
    @Autowired
    LoadedPartitionEntitiesCache loadedEntitiesCache;
    @Autowired
    InternalStreamStatistics statistics;

    @Mock
    ObserverInternalStreamMessageProcessor processor;

    private ObserverInternalStreamQueuedReadProcessor queuedReadProcessor;

    @BeforeEach
    public void setup() {
        MockitoAnnotations.openMocks(this);
        loadedEntitiesCache.getIterations().invalidateAll();
        loadedEntitiesCache.getChecks().invalidateAll();

        this.queuedReadProcessor = new ObserverInternalStreamQueuedReadProcessor(statistics, processor, 10, 1, false);

        db.tx(() -> db.checks().save(SAMPLE_CHECK));
    }

    @Test
    void process_CommitAllMessageCases() {
        for (var messagesCase : ENQUEUEING_MESSAGES_SETTER.keySet()) {
            var lbPartitionRead = createLbPartitionRead(messagesCase);
            var readInternalMessage = createReadInternalStreamMessage(
                    lbPartitionRead, createInternalStreamMessage(messagesCase)
            );

            queuedReadProcessor.process(List.of(readInternalMessage));

            Mockito.verify(lbPartitionRead.getCommitCountdown(), Mockito.times(1)).notifyMessageProcessed(1);
        }
    }

    @Test
    void process_CancelMessage() {
        var messageCase = Internal.InternalMessage.MessagesCase.CANCEL;
        var lbPartitionRead = createLbPartitionRead(messageCase);
        var internalMessage = createInternalStreamMessage(messageCase);
        var readInternalMessage = createReadInternalStreamMessage(lbPartitionRead, internalMessage);

        queuedReadProcessor.process(List.of(readInternalMessage));

        Mockito.verify(processor, Mockito.times(1)).processCancelMessages(List.of(internalMessage));
        Mockito.verifyNoMoreInteractions(processor);
    }

    @Test
    void process_IterationFinishMessage() {
        var messageCase = Internal.InternalMessage.MessagesCase.ITERATION_FINISH;
        var lbPartitionRead = createLbPartitionRead(messageCase);
        var internalMessage = createInternalStreamMessage(messageCase);
        var readInternalMessage = createReadInternalStreamMessage(lbPartitionRead, internalMessage);

        queuedReadProcessor.process(List.of(readInternalMessage));

        Mockito.verify(processor, Mockito.times(1))
                .processIterationFinishMessages(
                        Map.of(
                                internalMessage.getIterationFinish().getIterationId(),
                                internalMessage.getIterationFinish()
                        )
                );
        Mockito.verifyNoMoreInteractions(processor);
    }

    @Test
    void process_AggregateMessage() {
        var messageCase = Internal.InternalMessage.MessagesCase.AGGREGATE;
        var lbPartitionRead = createLbPartitionRead(messageCase);
        var internalMessage = createInternalStreamMessage(messageCase);
        var readInternalMessage = createReadInternalStreamMessage(lbPartitionRead, internalMessage);

        queuedReadProcessor.process(List.of(readInternalMessage));

        Mockito.verify(processor, Mockito.times(1))
                .processAggregateMessages(Map.of(
                        internalMessage.getAggregate().getIterationId(),
                        List.of(internalMessage.getAggregate())
                ));
        Mockito.verifyNoMoreInteractions(processor);
    }

    @Test
    void process_FatalErrorMessage() {
        var messageCase = Internal.InternalMessage.MessagesCase.FATAL_ERROR;
        var lbPartitionRead = createLbPartitionRead(messageCase);
        var internalMessage = createInternalStreamMessage(messageCase);
        var readInternalMessage = createReadInternalStreamMessage(lbPartitionRead, internalMessage);

        queuedReadProcessor.process(List.of(readInternalMessage));

        Mockito.verify(processor, Mockito.times(1)).processFatalErrorMessages(List.of(internalMessage));
        Mockito.verifyNoMoreInteractions(processor);
    }

    @Test
    void process_PessimizeMessage() {
        var messageCase = Internal.InternalMessage.MessagesCase.PESSIMIZE;
        var lbPartitionRead = createLbPartitionRead(messageCase);
        var internalMessage = createInternalStreamMessage(messageCase);
        var readInternalMessage = createReadInternalStreamMessage(lbPartitionRead, internalMessage);

        queuedReadProcessor.process(List.of(readInternalMessage));

        Mockito.verify(processor, Mockito.times(1))
                .processPessimizeMessages(List.of(internalMessage.getPessimize().getIterationId()));
        Mockito.verifyNoMoreInteractions(processor);
    }

    @Test
    void process_RegisteredMessage() {
        var messageCase = Internal.InternalMessage.MessagesCase.REGISTERED;
        var lbPartitionRead = createLbPartitionRead(messageCase);
        var internalMessage = createInternalStreamMessage(messageCase);
        var readInternalMessage = createReadInternalStreamMessage(lbPartitionRead, internalMessage);

        queuedReadProcessor.process(List.of(readInternalMessage));

        Mockito.verify(processor, Mockito.times(1))
                .processRegisteredMessages(List.of(internalMessage.getRegistered()));
        Mockito.verifyNoMoreInteractions(processor);
    }

    @Test
    void process_TechnicalStatsMessage() {
        var messageCase = Internal.InternalMessage.MessagesCase.TECHNICAL_STATS_MESSAGE;
        var lbPartitionRead = createLbPartitionRead(messageCase);
        var internalMessage = createInternalStreamMessage(messageCase);
        var readInternalMessage = createReadInternalStreamMessage(lbPartitionRead, internalMessage);

        queuedReadProcessor.process(List.of(readInternalMessage));

        Mockito.verify(processor, Mockito.times(1))
                .processTechnicalStatsMessages(List.of(internalMessage.getTechnicalStatsMessage()));
        Mockito.verifyNoMoreInteractions(processor);
    }

    private ReadInternalStreamMessage createReadInternalStreamMessage(
            LogbrokerPartitionRead lbPartitionRead,
            Internal.InternalMessage internalMessage
    ) {
        return new ReadInternalStreamMessage(new AtomicInteger(1), lbPartitionRead, internalMessage);
    }
}
