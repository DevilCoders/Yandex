package ru.yandex.ci.observer.reader.message.internal;

import java.util.List;
import java.util.Map;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import ru.yandex.ci.observer.core.Internal;
import ru.yandex.ci.observer.reader.check.ObserverInternalCheckService;

class ObserverInternalStreamMessageProcessorTest extends ObserverInternalStreamProcessorTestBase {
    @Mock
    ObserverInternalCheckService checkService;

    private ObserverInternalStreamMessageProcessor processor;

    @BeforeEach
    public void setup() {
        MockitoAnnotations.openMocks(this);

        this.processor = new ObserverInternalStreamMessageProcessor(checkService, entitiesChecker);

        db.tx(() -> {
            db.checks().save(SAMPLE_CHECK);
            db.iterations().save(SAMPLE_ITERATION);
            db.tasks().save(SAMPLE_TASK);
        });
    }

    @Test
    void processRegisteredMessages() {
        var registeredMessage = createInternalStreamMessage(
                Internal.InternalMessage.MessagesCase.REGISTERED
        ).getRegistered();

        processor.processRegisteredMessages(List.of(registeredMessage));

        Assertions.assertTrue(cache.iterationsGrouped().getIfPresent(SAMPLE_ITERATION_ID).isPresent());
        Assertions.assertTrue(cache.tasksGrouped().getIfPresent(SAMPLE_TASK_ID).isPresent());
        Mockito.verifyNoMoreInteractions(checkService);
    }

    @Test
    void processAggregateMessages() {
        var aggregateMessage = createInternalStreamMessage(
                Internal.InternalMessage.MessagesCase.AGGREGATE
        ).getAggregate();

        processor.processAggregateMessages(Map.of(
                aggregateMessage.getIterationId(), List.of(aggregateMessage)
        ));

        Assertions.assertTrue(cache.iterationsGrouped().getIfPresent(SAMPLE_ITERATION_ID).isPresent());
        Assertions.assertTrue(cache.tasksGrouped().getIfPresent(SAMPLE_TASK_ID).isPresent());
        Mockito.verify(checkService, Mockito.times(1)).aggregateTraceStages(Mockito.any(), Mockito.anySet());
        Mockito.verifyNoMoreInteractions(checkService);
    }

    @Test
    void processFatalErrorMessages() {
        var message = createInternalStreamMessage(
                Internal.InternalMessage.MessagesCase.FATAL_ERROR
        );

        processor.processFatalErrorMessages(List.of(message));

        Assertions.assertTrue(cache.tasksGrouped().getIfPresent(SAMPLE_TASK_ID).isPresent());
        Mockito.verify(checkService, Mockito.times(1)).processCheckTaskFatalError(Mockito.any(), Mockito.any());
        Mockito.verifyNoMoreInteractions(checkService);
    }

    @Test
    void processCancelMessages() {
        var message = createInternalStreamMessage(
                Internal.InternalMessage.MessagesCase.CANCEL
        );

        processor.processCancelMessages(List.of(message));

        Assertions.assertTrue(cache.iterationsGrouped().getIfPresent(SAMPLE_ITERATION_ID).isPresent());
        Mockito.verify(checkService, Mockito.times(1)).processCheckTaskCancel(Mockito.any(), Mockito.any());
        Mockito.verifyNoMoreInteractions(checkService);
    }

    @Test
    void processPessimizeMessages() {
        var iterationId = createInternalStreamMessage(
                Internal.InternalMessage.MessagesCase.PESSIMIZE
        ).getPessimize().getIterationId();

        processor.processPessimizeMessages(List.of(iterationId));

        Assertions.assertTrue(cache.iterationsGrouped().getIfPresent(SAMPLE_ITERATION_ID).isPresent());
        Mockito.verify(checkService, Mockito.times(1)).processPessimize(Mockito.any());
        Mockito.verifyNoMoreInteractions(checkService);
    }

    @Test
    void processTechnicalStatsMessages() {
        var techStatsMessage = createInternalStreamMessage(
                Internal.InternalMessage.MessagesCase.TECHNICAL_STATS_MESSAGE
        ).getTechnicalStatsMessage();

        processor.processTechnicalStatsMessages(List.of(techStatsMessage));

        Assertions.assertTrue(cache.iterationsGrouped().getIfPresent(SAMPLE_ITERATION_ID).isPresent());
        Mockito.verify(checkService, Mockito.times(1))
                .addTechnicalStatistics(Mockito.any(), Mockito.anyInt(), Mockito.any());
        Mockito.verifyNoMoreInteractions(checkService);
    }

    @Test
    void processFinishPartitionMessages() {
        var finishPartition = createInternalStreamMessage(
                Internal.InternalMessage.MessagesCase.FINISH_PARTITION
        ).getFinishPartition();

        processor.processFinishPartitionMessages(Map.of(finishPartition.getTaskId(), List.of(finishPartition)));

        Assertions.assertTrue(cache.tasksGrouped().getIfPresent(SAMPLE_TASK_ID).isPresent());
        Mockito.verify(checkService, Mockito.times(1)).processPartitionFinished(Mockito.any(), Mockito.anyList());
        Mockito.verifyNoMoreInteractions(checkService);
    }

    @Test
    void processIterationFinishMessages() {
        var iterationFinishMessage = createInternalStreamMessage(
                Internal.InternalMessage.MessagesCase.ITERATION_FINISH
        ).getIterationFinish();

        processor.processIterationFinishMessages(
                Map.of(iterationFinishMessage.getIterationId(), iterationFinishMessage)
        );

        Assertions.assertTrue(cache.iterationsGrouped().getIfPresent(SAMPLE_ITERATION_ID).isPresent());
        Mockito.verify(checkService, Mockito.times(1))
                .processIterationFinish(Mockito.any(), Mockito.any(), Mockito.any());
        Mockito.verifyNoMoreInteractions(checkService);
    }
}
