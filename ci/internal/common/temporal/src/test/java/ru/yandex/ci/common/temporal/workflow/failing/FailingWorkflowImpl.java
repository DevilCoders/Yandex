package ru.yandex.ci.common.temporal.workflow.failing;

import java.time.Duration;

import io.temporal.workflow.Workflow;

import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestId;

public class FailingWorkflowImpl implements FailingWorkflow {
    @Override
    public void run(SimpleTestId id) {
        Workflow.sleep(Duration.ofSeconds(10));
        throw new RuntimeException("Fail as expected");
    }
}
