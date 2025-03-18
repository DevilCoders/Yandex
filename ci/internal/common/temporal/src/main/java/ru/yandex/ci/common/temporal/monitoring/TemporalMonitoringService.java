package ru.yandex.ci.common.temporal.monitoring;

import io.temporal.activity.ActivityInfo;
import io.temporal.workflow.WorkflowInfo;

public interface TemporalMonitoringService {
    void notifyWorkflowInit(WorkflowInfo info);

    void notifyWorkflowSuccess(WorkflowInfo info);

    void notifyWorkflowFailed(WorkflowInfo info, Exception e);

    void notifyActivityFailed(ActivityInfo info, Exception e);

    void notifyActivitySuccess(ActivityInfo info);
}
