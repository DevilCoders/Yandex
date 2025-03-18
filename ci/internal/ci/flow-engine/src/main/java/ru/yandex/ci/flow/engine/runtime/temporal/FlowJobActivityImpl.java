package ru.yandex.ci.flow.engine.runtime.temporal;

import io.temporal.activity.Activity;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.temporal.config.ActivityImplementation;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.JobLauncher;
import ru.yandex.ci.flow.engine.runtime.TmsTaskId;

@Slf4j
@AllArgsConstructor
public class FlowJobActivityImpl implements FlowJobActivity, ActivityImplementation {

    private final JobLauncher jobLauncher;

    @Override
    public void run(FullJobLaunchId id) {
        try {
            jobLauncher.launchJob(id, TmsTaskId.fromTemporal(Activity.getExecutionContext()));
        } catch (Exception e) {
            log.error("Exception occurred while execution of FlowTask with parameters {}", id, e);
            throw e;
        }
    }
}
