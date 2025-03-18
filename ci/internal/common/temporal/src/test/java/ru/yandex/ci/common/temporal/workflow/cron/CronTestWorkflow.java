package ru.yandex.ci.common.temporal.workflow.cron;

import io.temporal.workflow.WorkflowInterface;
import io.temporal.workflow.WorkflowMethod;

import ru.yandex.ci.common.temporal.BaseTemporalCronWorkflow;

@WorkflowInterface
public interface CronTestWorkflow extends BaseTemporalCronWorkflow {
    @WorkflowMethod
    void run();
}
