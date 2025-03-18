package ru.yandex.ci.common.temporal.config;

import java.util.concurrent.TimeUnit;

import io.temporal.worker.WorkerFactory;
import lombok.extern.slf4j.Slf4j;

@Slf4j
public class TemporalWorkerFactoryWrapper {
    private final WorkerFactory workerFactory;

    public TemporalWorkerFactoryWrapper(WorkerFactory workerFactory) {
        this.workerFactory = workerFactory;
    }

    public void start() {
        workerFactory.start();
    }

    public void shutdown() {
        log.info("Shutting down temporal WorkerFactory (and all worker");
        workerFactory.shutdownNow();
        while (!workerFactory.isTerminated()) {
            log.info("Awaiting WorkerFactory termination...");
            workerFactory.awaitTermination(1, TimeUnit.SECONDS);
        }
        log.info("Temporal WorkerFactory terminated");
    }

    public WorkerFactory getWorkerFactory() {
        return workerFactory;
    }
}
