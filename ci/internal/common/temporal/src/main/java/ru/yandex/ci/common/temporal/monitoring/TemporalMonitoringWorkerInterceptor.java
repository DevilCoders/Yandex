package ru.yandex.ci.common.temporal.monitoring;

import io.temporal.common.interceptors.ActivityInboundCallsInterceptor;
import io.temporal.common.interceptors.WorkerInterceptor;
import io.temporal.common.interceptors.WorkflowInboundCallsInterceptor;

public class TemporalMonitoringWorkerInterceptor implements WorkerInterceptor {
    private final TemporalMonitoringService monitoringService;

    public TemporalMonitoringWorkerInterceptor(TemporalMonitoringService monitoringService) {
        this.monitoringService = monitoringService;
    }

    @Override
    public WorkflowInboundCallsInterceptor interceptWorkflow(WorkflowInboundCallsInterceptor next) {
        return new TemporalMonitoringWorkflowInboundCallsInterceptor(monitoringService, next);
    }

    @Override
    public ActivityInboundCallsInterceptor interceptActivity(ActivityInboundCallsInterceptor next) {
        return new TemporalMonitoringActivityInboundCallsInterceptor(monitoringService, next);
    }
}
