package ru.yandex.ci.common.bazinga.monitoring;

import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.Tags;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.bolts.collection.Cf;
import ru.yandex.commune.bazinga.BazingaControllerAndWorkerApps;
import ru.yandex.commune.bazinga.BazingaControllerApp;
import ru.yandex.commune.bazinga.impl.controller.ControllerWorkerConnection;
import ru.yandex.commune.bazinga.impl.worker.WorkerInfo;
import ru.yandex.misc.time.Stopwatch;

public class BazingaScheduledMetrics {
    private static final String WORKER_PREFIX = "bazinga_worker_";

    private static final Logger log = LoggerFactory.getLogger(BazingaScheduledMetrics.class);

    private final BazingaControllerApp controllerApp;
    private final MeterRegistry registry;
    private int status;

    public BazingaScheduledMetrics(
            BazingaControllerAndWorkerApps bazingaControllerAndWorkerApps,
            MeterRegistry registry
    ) {
        this.controllerApp = bazingaControllerAndWorkerApps.getControllerApp();
        this.registry = registry;

        Gauge.builder("bazinga_master_status", () -> this.status).register(registry);
    }

    public void run() {
        log.info("BazingaScheduledMetrics started");

        var stopwatch = Stopwatch.createAndStart();
        writeBazingaMetrics(controllerApp.getBazingaController().isMaster());

        log.info("BazingaScheduledMetrics#run took {}ms", stopwatch.millisDuration());
    }

    private void writeBazingaMetrics(boolean isMaster) {
        this.status = isMaster ? controllerApp.getBazingaController().status().getSeverity() : 0;

        log.info("Found {} workers", controllerApp.getWorkersHolder().getWorkers().size());
        for (var worker : controllerApp.getWorkersHolder().getWorkers()) {
            writeThreadCounts(worker, isMaster);
            writeJobCounts(worker, isMaster);
        }
    }

    private void writeThreadCounts(ControllerWorkerConnection worker, boolean isMaster) {
        var queues = worker.getWorkerInfo().map(WorkerInfo::getTaskQueues).orElse(Cf.list());
        log.info("Found {} queues for thread counts", queues.size());

        for (var taskQueue : queues) {
            registry.gauge(
                    WORKER_PREFIX + "thread",
                    Tags.of("queue", taskQueue.getName().getName()),
                    isMaster ? taskQueue.getThreadCount() : 0
            );
        }
    }

    private void writeJobCounts(ControllerWorkerConnection worker, boolean isMaster) {
        var queueNameToJobCountMap = worker.getJobCountByQueueName();
        log.info("Found {} queues for job counts", queueNameToJobCountMap.size());

        for (var entry : queueNameToJobCountMap.entrySet()) {
            registry.gauge(
                    WORKER_PREFIX + "job", Tags.of("job", entry.getKey().getName()),
                    isMaster ? entry.getValue() : 0
            );
        }

        registry.gauge(
                WORKER_PREFIX + "active_cron",
                isMaster ? worker.getWorkerState().getTasks().stream()
                        .filter(cronTaskState -> !cronTaskState.getCurrentJobStatus().isCompleted())
                        .count() : 0
        );
    }
}
