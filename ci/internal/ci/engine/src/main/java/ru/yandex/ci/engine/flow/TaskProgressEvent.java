package ru.yandex.ci.engine.flow;

import lombok.Value;

import ru.yandex.ci.core.job.JobStatus;

@Value
public class TaskProgressEvent {
    String url;
    JobStatus jobStatus;
    KnownTaskModules taskModules;

    public static TaskProgressEvent ofSandbox(String url, JobStatus jobStatus) {
        return new TaskProgressEvent(url, jobStatus, KnownTaskModules.SANDBOX);
    }
}
