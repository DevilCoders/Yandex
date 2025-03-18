package ru.yandex.ci.tms.task.autocheck.degradation;

public class AutomaticDegradationStoppedException extends RuntimeException {
    public AutomaticDegradationStoppedException(String message) {
        super(message);
    }

    public AutomaticDegradationStoppedException(String message, Throwable cause) {
        super(message, cause);
    }
}
