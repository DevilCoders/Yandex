package ru.yandex.ci.util.queue;

import java.util.List;
import java.util.function.Consumer;

public final class SyncQueueExecutor<T> implements QueueExecutor<T> {

    private final Consumer<List<T>> executor;

    public SyncQueueExecutor(Consumer<List<T>> executor) {
        this.executor = executor;
    }

    @Override
    public void enqueue(String queueNumber, T message) {
        this.executor.accept(List.of(message));
    }

    @Override
    public int getQueueSize() {
        return 0;
    }
}
