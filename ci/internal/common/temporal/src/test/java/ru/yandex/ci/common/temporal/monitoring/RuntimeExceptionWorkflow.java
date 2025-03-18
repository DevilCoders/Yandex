package ru.yandex.ci.common.temporal.monitoring;

import io.temporal.workflow.WorkflowInterface;
import io.temporal.workflow.WorkflowMethod;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestId;

@WorkflowInterface
public interface RuntimeExceptionWorkflow extends BaseTemporalWorkflow<SimpleTestId> {
    @WorkflowMethod
    void run(SimpleTestId id);

    class Impl implements RuntimeExceptionWorkflow {
        @Override
        public void run(SimpleTestId id) {
            throw new RuntimeException();
        }
    }
}
