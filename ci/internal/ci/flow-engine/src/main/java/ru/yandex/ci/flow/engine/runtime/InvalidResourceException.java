package ru.yandex.ci.flow.engine.runtime;

import ru.yandex.ci.core.job.TaskUnrecoverableException;

public class InvalidResourceException extends TaskUnrecoverableException {
    public InvalidResourceException(String message) {
        super(message);
    }

    public InvalidResourceException(String message, Throwable cause) {
        super(message, cause);
    }

    public InvalidResourceException(Throwable cause) {
        super(cause);
    }
}
