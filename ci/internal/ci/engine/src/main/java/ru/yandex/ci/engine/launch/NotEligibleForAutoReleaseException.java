package ru.yandex.ci.engine.launch;

public class NotEligibleForAutoReleaseException extends Exception {
    public NotEligibleForAutoReleaseException(String message) {
        super(message);
    }

    public NotEligibleForAutoReleaseException(String message, Throwable cause) {
        super(message, cause);
    }
}
