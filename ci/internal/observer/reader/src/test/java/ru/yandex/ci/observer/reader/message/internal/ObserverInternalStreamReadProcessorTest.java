package ru.yandex.ci.observer.reader.message.internal;

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
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.observer.core.Internal;
import ru.yandex.ci.observer.reader.message.internal.cache.LoadedPartitionEntitiesCache;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;

class ObserverInternalStreamReadProcessorTest extends ObserverInternalStreamProcessorTestBase {
    private static final Map<Internal.InternalMessage.MessagesCase,
            Consumer<Internal.InternalMessage.Builder>> NOT_ENQUEUEING_MESSAGES_SETTER = Map.of(
            Internal.InternalMessage.MessagesCase.MESSAGES_NOT_SET, wrapper -> {
            }
    );

    @Autowired
    LoadedPartitionEntitiesCache loadedEntitiesCache;
    @Autowired
    InternalStreamStatistics statistics;

    @Mock
    ObserverInternalStreamQueuedReadProcessor queuedReadProcessor;

    private ObserverInternalStreamReadProcessor readProcessor;

    @BeforeEach
    public void setup() {
        MockitoAnnotations.openMocks(this);
        Mockito.when(queuedReadProcessor.getQueueMaxNumber()).thenReturn(7);
        loadedEntitiesCache.getIterations().invalidateAll();
        loadedEntitiesCache.getChecks().invalidateAll();

        this.readProcessor = new ObserverInternalStreamReadProcessor(
                cache, entitiesChecker, loadedEntitiesCache, statistics, queuedReadProcessor
        );

        db.tx(() -> {
            db.checks().save(SAMPLE_CHECK);
            db.iterations().save(SAMPLE_ITERATION);
            db.tasks().save(SAMPLE_TASK);
        });
    }

    @Test
    void checkAllExistingMessageCasesRegistered() {
        var expectedMessageCases = new HashSet<Internal.InternalMessage.MessagesCase>();
        expectedMessageCases.addAll(ENQUEUEING_MESSAGES_SETTER.keySet());
        expectedMessageCases.addAll(NOT_ENQUEUEING_MESSAGES_SETTER.keySet());

        var actualMessageCases = Arrays.stream(Internal.InternalMessage.MessagesCase.values())
                .collect(Collectors.toSet());

        Assertions.assertEquals(expectedMessageCases, actualMessageCases);
    }

    @Test
    void process_EnqueueRegisteredMessageCases() {
        for (var entry : ENQUEUEING_MESSAGES_SETTER.entrySet()) {
            var lbPartitionRead = createLbPartitionRead(entry.getValue());

            readProcessor.process(List.of(lbPartitionRead));

            Mockito.verify(queuedReadProcessor, Mockito.times(1)).enqueue(Mockito.anyInt(), Mockito.any());
            Mockito.verify(lbPartitionRead.getCommitCountdown(), Mockito.times(2)).getCookie();
            Mockito.clearInvocations(queuedReadProcessor);
            Mockito.verifyNoMoreInteractions(lbPartitionRead.getCommitCountdown());
        }
    }

    @Test
    void process_CommitNotEnqueueingMessageCases() {
        for (var entry : NOT_ENQUEUEING_MESSAGES_SETTER.entrySet()) {
            var lbPartitionRead = createLbPartitionRead(entry.getValue());

            readProcessor.process(List.of(lbPartitionRead));

            Mockito.verify(lbPartitionRead.getCommitCountdown(), Mockito.times(1)).notifyMessageProcessed(1);
            Mockito.verify(lbPartitionRead.getCommitCountdown(), Mockito.times(3)).getCookie();
            Mockito.verifyNoMoreInteractions(lbPartitionRead.getCommitCountdown());
        }
    }

    @Test
    void onLock() {
        testCleanPartitionEntitiesCache(() -> readProcessor.onLock(
                new ConsumerLockMessage("test_topic", loadedEntitiesCache.getPartition(SAMPLE_CHECK_ID), 0, 0, 0)
        ));
    }

    @Test
    void onRelease() {
        testCleanPartitionEntitiesCache(() -> readProcessor.onRelease(
                new ConsumerReleaseMessage("test_topic", loadedEntitiesCache.getPartition(SAMPLE_CHECK_ID), false, 0)
        ));
    }

    private void testCleanPartitionEntitiesCache(Runnable runnable) {
        cache.checks().getOrThrow(SAMPLE_CHECK_ID);
        cache.iterationsGrouped().getOrThrow(SAMPLE_ITERATION_ID);
        cache.tasksGrouped().getOrThrow(SAMPLE_TASK_ID);
        loadedEntitiesCache.onCheckLoaded(SAMPLE_CHECK_ID);
        loadedEntitiesCache.onIterationLoaded(SAMPLE_ITERATION_ID);

        Assertions.assertTrue(cache.checks().getIfPresent(SAMPLE_CHECK_ID).isPresent());
        Assertions.assertTrue(cache.iterationsGrouped().getIfPresent(SAMPLE_ITERATION_ID).isPresent());
        Assertions.assertTrue(cache.tasksGrouped().getIfPresent(SAMPLE_TASK_ID).isPresent());

        runnable.run();

        Assertions.assertFalse(cache.checks().getIfPresent(SAMPLE_CHECK_ID).isPresent());
        Assertions.assertFalse(cache.iterationsGrouped().getIfPresent(SAMPLE_ITERATION_ID).isPresent());
        Assertions.assertFalse(cache.tasksGrouped().getIfPresent(SAMPLE_TASK_ID).isPresent());
        Assertions.assertNull(
                loadedEntitiesCache.getChecks().getIfPresent(loadedEntitiesCache.getPartition(SAMPLE_CHECK_ID))
        );
        Assertions.assertNull(
                loadedEntitiesCache.getIterations().getIfPresent(loadedEntitiesCache.getPartition(SAMPLE_ITERATION_ID))
        );
    }
}
