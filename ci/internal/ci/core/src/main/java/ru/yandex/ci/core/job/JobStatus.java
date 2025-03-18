package ru.yandex.ci.core.job;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum JobStatus {
    CREATED(false),
    RUNNING(false),
    SUCCESS(true),
    FAILED(true);

    private final boolean finished;

    JobStatus(boolean finished) {
        this.finished = finished;
    }

    public boolean isFinished() {
        return finished;
    }
}
