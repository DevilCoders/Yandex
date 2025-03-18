package ru.yandex.ci.flow.engine.runtime;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

public interface JobExecutorProvider {
    JobExecutor createJobExecutor(JobContext jobContext);
}
