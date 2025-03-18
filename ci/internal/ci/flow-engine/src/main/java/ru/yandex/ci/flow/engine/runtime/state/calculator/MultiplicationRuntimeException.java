package ru.yandex.ci.flow.engine.runtime.state.calculator;

import ru.yandex.ci.core.job.TaskUnrecoverableException;

public class MultiplicationRuntimeException extends TaskUnrecoverableException {
    public MultiplicationRuntimeException(String message) {
        super(message);
    }

    public MultiplicationRuntimeException(String message, Throwable cause) {
        super(message, cause);
    }

    public MultiplicationRuntimeException(Throwable cause) {
        super(cause);
    }
}
