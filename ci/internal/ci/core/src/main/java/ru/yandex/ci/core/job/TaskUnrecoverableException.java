package ru.yandex.ci.core.job;

/**
 * Exception to be thrown when task is failed and cannot be recovered
 */
public class TaskUnrecoverableException extends RuntimeException {
    public TaskUnrecoverableException(String message) {
        super(message);
    }

    public TaskUnrecoverableException(String message, Throwable cause) {
        super(message, cause);
    }

    public TaskUnrecoverableException(Throwable cause) {
        super(cause);
    }
}
