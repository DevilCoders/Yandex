package ru.yandex.ci.common.temporal.monitoring;

import io.temporal.common.interceptors.WorkflowInboundCallsInterceptor;
import io.temporal.common.interceptors.WorkflowInboundCallsInterceptorBase;
import io.temporal.common.interceptors.WorkflowOutboundCallsInterceptor;
import io.temporal.workflow.Workflow;

public class TemporalMonitoringWorkflowInboundCallsInterceptor extends WorkflowInboundCallsInterceptorBase {

    private final TemporalMonitoringService monitoringService;

    public TemporalMonitoringWorkflowInboundCallsInterceptor(TemporalMonitoringService monitoringService,
                                                             WorkflowInboundCallsInterceptor next) {
        super(next);
        this.monitoringService = monitoringService;
    }

    @Override
    public void init(WorkflowOutboundCallsInterceptor outboundCalls) {
        monitoringService.notifyWorkflowInit(Workflow.getInfo());
        super.init(outboundCalls);
    }

    @Override
    public WorkflowOutput execute(WorkflowInput input) {
        try {
            var output = super.execute(input);
            monitoringService.notifyWorkflowSuccess(Workflow.getInfo());
            return output;
        } catch (Exception e) {
            monitoringService.notifyWorkflowFailed(Workflow.getInfo(), e);
            throw e;
        }
    }
}
