package ru.yandex.ci.common.temporal.workflow.simple;

import io.temporal.workflow.WorkflowInterface;
import io.temporal.workflow.WorkflowMethod;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;

@WorkflowInterface
public interface SimpleTestWorkflow2 extends BaseTemporalWorkflow<SimpleTestLongId> {

    @WorkflowMethod
    void run(SimpleTestLongId input);
}


