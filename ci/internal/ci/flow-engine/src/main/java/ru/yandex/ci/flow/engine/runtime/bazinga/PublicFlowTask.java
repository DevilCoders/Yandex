package ru.yandex.ci.flow.engine.runtime.bazinga;

import java.time.Duration;

import ru.yandex.ci.flow.engine.runtime.JobLauncher;
import ru.yandex.commune.bazinga.scheduler.TaskQueueName;

public class PublicFlowTask extends FlowTaskBase {

    public PublicFlowTask(JobLauncher jobLauncher, Duration timeout) {
        super(jobLauncher, timeout);
    }

    public PublicFlowTask(FlowTaskParameters parameters) {
        super(parameters);
    }

    @Override
    public TaskQueueName queueName() {
        //TODO consider using separate task queue after migration to temporal
        return super.queueName();
    }
}
