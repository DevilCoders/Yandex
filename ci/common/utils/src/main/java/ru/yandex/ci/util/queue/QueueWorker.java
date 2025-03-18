package ru.yandex.ci.util.queue;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.BlockingQueue;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.util.Retryable;
import ru.yandex.ci.util.StorageInterruptedException;

@Slf4j
public abstract class QueueWorker<T> implements Runnable {
    private final BlockingQueue<T> queue;
    protected final int drainLimit;

    protected QueueWorker(BlockingQueue<T> queue, int drainLimit) {
        this.queue = queue;
        this.drainLimit = drainLimit;
    }

    @Override
    public void run() {
        while (!Thread.interrupted()) {
            var items = new ArrayList<T>();

            try {
                if (this.queue.isEmpty()) {
                    items.add(this.queue.take());
                } else {
                    this.queue.drainTo(items, this.drainLimit);
                }
            } catch (InterruptedException e) {
                log.info("Interrupted", e);
                return;
            }

            this.onDrain(items.size());

            try {
                processRetryable(items);
            } catch (StorageInterruptedException e) {
                log.info("Worker interrupted", e);
                return;
            } catch (RuntimeException e) {
                // Can happen when Retryable is disabled (in tests) or code is broken.
                log.error("[Critical] Unhandled exception in worker!", e);
                this.onFailed();
                return;
            }
        }
    }

    private void processRetryable(ArrayList<T> items) {
        Retryable.retryUntilInterruptedOrSucceeded(
                () -> this.process(items),
                t -> {
                    log.error("{} worker error", this.getClass().getSimpleName(), t);
                    this.onFailed();
                }
        );
    }

    public abstract void onDrain(int amount);

    public abstract void process(List<T> items);

    public abstract void onFailed();
}
