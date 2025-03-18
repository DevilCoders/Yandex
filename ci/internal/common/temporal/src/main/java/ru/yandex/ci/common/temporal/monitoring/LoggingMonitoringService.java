package ru.yandex.ci.common.temporal.monitoring;

import io.temporal.activity.ActivityInfo;
import io.temporal.workflow.WorkflowInfo;
import lombok.extern.slf4j.Slf4j;

@Slf4j
public class LoggingMonitoringService implements TemporalMonitoringService {
    @Override
    public void notifyWorkflowInit(WorkflowInfo info) {
        log.info("Workflow init. Wf {}", info.getWorkflowId());
    }

    @Override
    public void notifyWorkflowSuccess(WorkflowInfo info) {
        log.info("Workflow success. Wf {}", info.getWorkflowId());

    }

    @Override
    public void notifyWorkflowFailed(WorkflowInfo info, Exception e) {
        log.warn("Workflow failed. Wf {}", info.getWorkflowId());
    }

    @Override
    public void notifyActivitySuccess(ActivityInfo info) {
        log.warn("Workflow success. Wf {}, activity {}", info.getWorkflowId(), info.getActivityId());
    }

    @Override
    public void notifyActivityFailed(ActivityInfo info, Exception e) {
        log.warn("Workflow failed. Wf {}, activity {}", info.getWorkflowId(), info.getActivityId());
    }


}
