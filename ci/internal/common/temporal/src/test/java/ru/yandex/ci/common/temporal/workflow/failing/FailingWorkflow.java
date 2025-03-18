package ru.yandex.ci.common.temporal.workflow.failing;

import io.temporal.workflow.WorkflowInterface;
import io.temporal.workflow.WorkflowMethod;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestId;

@WorkflowInterface
public interface FailingWorkflow extends BaseTemporalWorkflow<SimpleTestId> {
    @WorkflowMethod
    void run(SimpleTestId id);
}
