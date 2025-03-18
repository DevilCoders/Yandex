package ru.yandex.ci.flow.engine.runtime.events;

import java.util.List;

import lombok.ToString;


@ToString
public class DisableJobsGracefullyEvent implements FlowEvent {
    private final boolean ignoreUninterruptableStage;
    private final boolean killJobs;
    private final List<String> jobIds;

    public DisableJobsGracefullyEvent(
        boolean ignoreUninterruptableStage, boolean killJobs, List<String> jobIds
    ) {
        this.ignoreUninterruptableStage = ignoreUninterruptableStage;
        this.killJobs = killJobs;
        this.jobIds = jobIds;
    }

    public boolean isIgnoreUninterruptableStage() {
        return ignoreUninterruptableStage;
    }

    public boolean isKillJobs() {
        return killJobs;
    }

    public List<String> getJobIds() {
        return jobIds;
    }
}
