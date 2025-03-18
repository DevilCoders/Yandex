package ru.yandex.ci.flow.engine.runtime.events;

import java.util.List;
import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;

@ToString
public class JobStateChangedEvent extends JobLaunchEvent {
    private final String statusText;
    private final Float totalProgress;
    private final List<TaskBadge> taskStates;

    public JobStateChangedEvent(
            String jobId,
            int jobLaunchNumber,
            String statusText,
            Float totalProgress,
            List<TaskBadge> taskStates
    ) {
        super(jobId, jobLaunchNumber, Set.of());
        this.statusText = statusText;
        this.totalProgress = totalProgress;
        this.taskStates = taskStates;
    }

    public String getStatusText() {
        return statusText;
    }

    public Float getTotalProgress() {
        return totalProgress;
    }

    public List<TaskBadge> getTaskStates() {
        return taskStates;
    }

    @Override
    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
        jobLaunch.setTotalProgress(this.totalProgress);
        jobLaunch.setStatusText(this.statusText);
        jobLaunch.setTaskStates(this.taskStates);
    }

    @Override
    public StatusChange createNextStatusChange() {
        return null;
    }
}
