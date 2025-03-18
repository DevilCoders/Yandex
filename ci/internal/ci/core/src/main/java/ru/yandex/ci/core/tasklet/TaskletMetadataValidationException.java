package ru.yandex.ci.core.tasklet;

import ru.yandex.ci.core.job.TaskUnrecoverableException;

public class TaskletMetadataValidationException extends TaskUnrecoverableException {
    public TaskletMetadataValidationException(String message) {
        super(message);
    }

    public TaskletMetadataValidationException(String message, Throwable cause) {
        super(message, cause);
    }
}
