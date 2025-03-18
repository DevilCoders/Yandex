package ru.yandex.ci.common.temporal.workflow.logging;


import java.util.concurrent.TimeUnit;

import io.temporal.activity.Activity;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.temporal.workflow.simple.SimpleActivity;


@Slf4j
public class LoggingActivity implements SimpleActivity {


    @Override
    public void runActivity() {
        log.info("Starting activity");

        int steps = 10000;
        for (int i = 1; i <= steps; i++) {
            try {
                TimeUnit.SECONDS.sleep(10);
            } catch (InterruptedException e) {
                throw Activity.wrap(e);
            }
            log.info("Step {}/{}", i, steps);
        }

        log.info("Finished");
    }
}
