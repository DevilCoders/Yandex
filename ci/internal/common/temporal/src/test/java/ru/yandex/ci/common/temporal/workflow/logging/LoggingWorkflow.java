package ru.yandex.ci.common.temporal.workflow.logging;

import java.time.Duration;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleActivity;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestId;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestWorkflow;

@Slf4j
public class LoggingWorkflow implements SimpleTestWorkflow {

    private final SimpleActivity activity = TemporalService.createActivity(
            SimpleActivity.class,
            Duration.ofMinutes(30)
    );

    @Override
    public void run(SimpleTestId input) {
        log.info("Starting workflow");
        activity.runActivity();
        log.info("Finished");
    }
}
