package ru.yandex.ci.core.launch;

import ru.yandex.ci.core.job.TaskUnrecoverableException;

public class FlowVarsValidationException extends TaskUnrecoverableException {
    public FlowVarsValidationException(String message) {
        super(message);
    }
    public FlowVarsValidationException(String message, Throwable e) {
        super(message, e);
    }
}
