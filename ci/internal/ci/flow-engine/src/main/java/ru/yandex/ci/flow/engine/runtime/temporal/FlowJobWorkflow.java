package ru.yandex.ci.flow.engine.runtime.temporal;

import io.temporal.workflow.WorkflowInterface;
import io.temporal.workflow.WorkflowMethod;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;

@WorkflowInterface
public interface FlowJobWorkflow extends BaseTemporalWorkflow<FullJobLaunchId> {
    @WorkflowMethod
    void run(FullJobLaunchId id);
}
