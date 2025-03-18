package ru.yandex.ci.common.temporal.heartbeat;

import io.temporal.common.interceptors.ActivityInboundCallsInterceptor;
import io.temporal.common.interceptors.WorkerInterceptor;
import io.temporal.common.interceptors.WorkflowInboundCallsInterceptor;

public class TemporalHeartbeatWorkerInterceptor implements WorkerInterceptor {

    private final TemporalWorkerHeartbeatService heartbeatService;

    public TemporalHeartbeatWorkerInterceptor(TemporalWorkerHeartbeatService heartbeatService) {
        this.heartbeatService = heartbeatService;
    }

    @Override
    public WorkflowInboundCallsInterceptor interceptWorkflow(WorkflowInboundCallsInterceptor next) {
        return next;
    }

    @Override
    public ActivityInboundCallsInterceptor interceptActivity(ActivityInboundCallsInterceptor next) {
        return new TemporalHeartbeatActivityInboundCallsInterceptor(heartbeatService, next);
    }
}
