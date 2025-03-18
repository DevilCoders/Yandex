package ru.yandex.ci.engine.flow;

import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.core.job.JobInstanceSource;

public class TaskletExecutionException extends RuntimeException implements JobInstanceSource {
    private final JobInstance jobInstance;

    private TaskletExecutionException(String message, JobInstance jobInstance) {
        super(message);
        this.jobInstance = jobInstance;
    }

    @Override
    public JobInstance getJobInstance() {
        return jobInstance;
    }

    public static TaskletExecutionException of(JobInstance jobInstance) {
        return switch (jobInstance.getRuntime()) {
            case SANDBOX -> {
                var message = "Tasklet " + jobInstance.getSandboxTaskId() + " failed";
                yield new TaskletExecutionException(message, jobInstance);
            }
            case TASKLET_V2 -> {
                var message = "Tasklet v2 failed: " + jobInstance.getExecutionId();
                if (StringUtils.isNotEmpty(jobInstance.getErrorDescription())) {
                    message += ", error: " + jobInstance.getErrorDescription(); // TODO: separate field?
                }
                yield new TaskletExecutionException(message, jobInstance);
            }
        };
    }
}
