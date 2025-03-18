package ru.yandex.ci.storage.reader.message.events;

import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.util.queue.QueueExecutor;
import ru.yandex.ci.util.queue.QueueWorker;
import ru.yandex.ci.util.queue.SyncQueueExecutor;
import ru.yandex.ci.util.queue.ThreadPerQueueExecutor;

public class EventsPoolProcessor {
    private final QueueExecutor<EventMessage> executor;

    private final ReaderStatistics statistics;
    private final int numberOfThreads;

    private final int queueDrainLimit;

    private final EventsMessageProcessor eventsMessageProcessor;

    public EventsPoolProcessor(
            EventsMessageProcessor eventsMessageProcessor,
            ReaderStatistics statistics,
            int queueCapacity,
            int queueDrainLimit,
            int numberOfThreads
    ) {
        this.eventsMessageProcessor = eventsMessageProcessor;
        this.numberOfThreads = numberOfThreads;
        this.statistics = statistics;
        this.queueDrainLimit = queueDrainLimit;
        this.executor = numberOfThreads < 1 ?
                new SyncQueueExecutor<>(this::process) :
                new ThreadPerQueueExecutor<>("events-pool", queueCapacity, this::createQueueWorker);
    }

    public void enqueue(EventMessage message) {
        this.executor.enqueue(Integer.toString(message.getCheckId().distribute(this.numberOfThreads)), message);
    }

    public void process(List<EventMessage> messages) {
        this.eventsMessageProcessor.process(messages);
    }

    private QueueWorker<EventMessage> createQueueWorker(
            String queueNumber,
            LinkedBlockingQueue<EventMessage> queue
    ) {
        return new PoolWorker(queue, this.queueDrainLimit);
    }

    class PoolWorker extends QueueWorker<EventMessage> {

        PoolWorker(BlockingQueue<EventMessage> queue, int drainLimit) {
            super(queue, drainLimit);
        }

        @Override
        public void onDrain(int amount) {
        }

        @Override
        public void process(List<EventMessage> items) {
            EventsPoolProcessor.this.process(items);
        }

        @Override
        public void onFailed() {
            EventsPoolProcessor.this.statistics.getEvents().onEventWorkerError();
        }
    }
}
