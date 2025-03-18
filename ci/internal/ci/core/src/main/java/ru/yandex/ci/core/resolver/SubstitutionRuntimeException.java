package ru.yandex.ci.core.resolver;

import ru.yandex.ci.core.job.TaskUnrecoverableException;

public class SubstitutionRuntimeException extends TaskUnrecoverableException {
    public SubstitutionRuntimeException(String message, Throwable cause) {
        super(message, cause);
    }
}
