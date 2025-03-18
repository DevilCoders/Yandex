package ru.yandex.ci.common.temporal.worflow;

import io.temporal.workflow.WorkflowInterface;
import io.temporal.workflow.WorkflowMethod;

import ru.yandex.ci.common.temporal.BaseTemporalCronWorkflow;

@WorkflowInterface
public interface TemporalCiSystemCronWorkflow extends BaseTemporalCronWorkflow {
    @WorkflowMethod
    void run();
}
