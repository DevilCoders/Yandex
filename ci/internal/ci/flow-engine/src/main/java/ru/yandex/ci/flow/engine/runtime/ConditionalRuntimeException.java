package ru.yandex.ci.flow.engine.runtime;

import ru.yandex.ci.core.job.TaskUnrecoverableException;

public class ConditionalRuntimeException extends TaskUnrecoverableException {
    public ConditionalRuntimeException(String message) {
        super(message);
    }

    public ConditionalRuntimeException(String message, Throwable cause) {
        super(message, cause);
    }

    public ConditionalRuntimeException(Throwable cause) {
        super(cause);
    }
}
