package ru.yandex.ci.util.queue;

import java.util.Collection;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.function.BiFunction;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.util.StorageInterruptedException;

@Slf4j
public final class ThreadPerQueueExecutor<T> implements QueueExecutor<T> {

    private final String name;
    private final int queueCapacity;
    private final BiFunction<String, LinkedBlockingQueue<T>, QueueWorker<T>> workerFactory;

    private final ConcurrentMap<String, BlockingQueue<T>> queues;

    public ThreadPerQueueExecutor(
            String name, BiFunction<String, LinkedBlockingQueue<T>, QueueWorker<T>> workerFactory
    ) {
        this(name, 0, workerFactory);
    }

    public ThreadPerQueueExecutor(
            String name,
            int queueCapacity,
            BiFunction<String, LinkedBlockingQueue<T>, QueueWorker<T>> workerFactory
    ) {
        this.name = name;
        this.queueCapacity = queueCapacity;
        this.workerFactory = workerFactory;
        this.queues = new ConcurrentHashMap<>();
    }

    @Override
    public void enqueue(String queueName, T message) {
        try {
            queues.computeIfAbsent(queueName, number -> createQueue(number, queueCapacity)).put(message);
        } catch (InterruptedException e) {
            throw new StorageInterruptedException(e);
        }
    }

    @Override
    public int getQueueSize() {
        return queues.values().stream().mapToInt(Collection::size).sum();
    }

    private BlockingQueue<T> createQueue(String queueName, int capacity) {
        log.info("Creating queue {} for {}, capacity: {}", queueName, name, capacity);
        var queue = capacity > 0 ? new LinkedBlockingQueue<T>(capacity) : new LinkedBlockingQueue<T>();

        var worker = this.workerFactory.apply(queueName, queue);

        var thread = new Thread(worker, "%s-%s".formatted(name, queueName));
        thread.start();

        return queue;
    }
}
