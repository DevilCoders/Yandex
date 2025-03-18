package ru.yandex.ci.flow.engine.runtime.bazinga;

import java.time.Duration;

import ru.yandex.ci.flow.engine.runtime.JobLauncher;

public class FlowTask extends FlowTaskBase {

    public FlowTask(JobLauncher jobLauncher, Duration timeout) {
        super(jobLauncher, timeout);
    }

    public FlowTask(FlowTaskParameters parameters) {
        super(parameters);
    }
}
