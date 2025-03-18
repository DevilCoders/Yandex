package ru.yandex.ci.engine.event;

import io.temporal.workflow.WorkflowInterface;
import io.temporal.workflow.WorkflowMethod;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;

@WorkflowInterface
public interface CiEventWorkflow extends BaseTemporalWorkflow<CiEventPayload> {
    @WorkflowMethod
    void send(CiEventPayload id);
}
