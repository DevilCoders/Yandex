package ru.yandex.ci.common.temporal.config;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import io.temporal.client.WorkflowClient;
import io.temporal.common.interceptors.WorkerInterceptor;
import io.temporal.worker.WorkerFactory;
import io.temporal.worker.WorkerFactoryOptions;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.temporal.heartbeat.TemporalHeartbeatWorkerInterceptor;
import ru.yandex.ci.common.temporal.heartbeat.TemporalWorkerHeartbeatService;
import ru.yandex.ci.common.temporal.monitoring.TemporalMonitoringService;
import ru.yandex.ci.common.temporal.monitoring.TemporalMonitoringWorkerInterceptor;

@Slf4j
public class TemporalWorkerFactoryBuilder {

    private final WorkflowClient workflowClient;
    private final List<TemporalWorkerBuilder> workers = new ArrayList<>();
    private final List<WorkerInterceptor> customInterceptors = new ArrayList<>();

    private boolean created = false;

    @Nullable
    private TemporalWorkerHeartbeatService heartbeatService;

    @Nullable
    private TemporalMonitoringService monitoringService;

    private TemporalWorkerFactoryBuilder(WorkflowClient workflowClient) {
        this.workflowClient = workflowClient;
    }

    static TemporalWorkerFactoryBuilder create(WorkflowClient workflowClient) {
        return new TemporalWorkerFactoryBuilder(workflowClient);
    }

    public TemporalWorkerFactoryBuilder monitoringService(TemporalMonitoringService monitoringService) {
        this.monitoringService = monitoringService;
        return this;
    }

    public TemporalWorkerFactoryBuilder heartbeatService(TemporalWorkerHeartbeatService heartbeatService) {
        this.heartbeatService = heartbeatService;
        return this;
    }

    public TemporalWorkerFactoryBuilder worker(TemporalWorkerBuilder workerBuilder) {
        workers.add(workerBuilder);
        return this;
    }

    public TemporalWorkerFactoryBuilder customInterceptor(WorkerInterceptor workerInterceptor) {
        customInterceptors.add(workerInterceptor);
        return this;
    }

    private void validate() {
        Preconditions.checkState(!created, "Already create. You can call build() only once.");
        Preconditions.checkState(!workers.isEmpty(), "At least one worker worker must be created");
        Preconditions.checkState(monitoringService != null, "Monitoring service is not provided");
        var queues = new HashSet<>(workers.size());
        for (var worker : workers) {
            if (!queues.add(worker.getQueue())) {
                throw new IllegalStateException("Duplicate worker queue: " + worker.getQueue());
            }
        }
    }

    private WorkerFactory createWorkerFactory() {
        List<WorkerInterceptor> workerInterceptors = new ArrayList<>();
        workerInterceptors.add(new TemporalMonitoringWorkerInterceptor(Preconditions.checkNotNull(monitoringService)));
        if (heartbeatService != null) {
            workerInterceptors.add(new TemporalHeartbeatWorkerInterceptor(heartbeatService));
        } else {
            log.warn("HeartbeatService is not configured");
        }
        workerInterceptors.addAll(customInterceptors);

        var options = WorkerFactoryOptions.newBuilder()
                .setWorkerInterceptors(workerInterceptors.toArray(new WorkerInterceptor[0]))
                .validateAndBuildWithDefaults();

        return WorkerFactory.newInstance(workflowClient, options);
    }

    public TemporalWorkerFactoryWrapper build() {
        validate();
        var workerFactory = createWorkerFactory();
        for (var worker : workers) {
            worker.createWorker(workerFactory);
        }
        created = true;
        return new TemporalWorkerFactoryWrapper(workerFactory);
    }

}
