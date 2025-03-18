package ru.yandex.ci.flow.engine.runtime.events;

import java.util.List;

import lombok.ToString;

import ru.yandex.ci.flow.engine.definition.context.impl.SupportType;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;

@ToString
public class JobExecutorExpectedFailedEvent extends JobExecutorFailedEvent {
    private final List<SupportType> supportInfo;

    public JobExecutorExpectedFailedEvent(
            String jobId,
            int jobLaunchNumber,
            ResourceRefContainer producedResources,
            List<SupportType> supportInfo,
            Throwable executionException
    ) {
        super(jobId, jobLaunchNumber, producedResources, executionException);
        this.supportInfo = supportInfo;
    }

    @Override
    public StatusChange createNextStatusChange() {
        return StatusChange.executorExpectedFailed();
    }

    @Override
    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
        super.afterStatusChange(jobLaunch, jobState);
        jobLaunch.setSupportInfo(supportInfo);
    }
}
