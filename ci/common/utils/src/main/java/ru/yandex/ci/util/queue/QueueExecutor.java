package ru.yandex.ci.util.queue;

public interface QueueExecutor<T> {
    void enqueue(String queueName, T message);

    int getQueueSize();
}
