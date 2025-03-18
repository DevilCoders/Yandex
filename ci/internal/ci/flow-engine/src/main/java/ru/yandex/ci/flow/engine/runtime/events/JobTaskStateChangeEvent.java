package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;

@ToString
public class JobTaskStateChangeEvent extends JobLaunchEvent {
    private final TaskBadge taskBadge;

    public JobTaskStateChangeEvent(String jobId, int jobLaunchNumber, TaskBadge taskBadge) {
        super(jobId, jobLaunchNumber, Set.of());
        this.taskBadge = taskBadge;
    }

    @Override
    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
        jobLaunch.setTaskState(taskBadge);
    }

    @Override
    public StatusChange createNextStatusChange() {
        return null;
    }
}
