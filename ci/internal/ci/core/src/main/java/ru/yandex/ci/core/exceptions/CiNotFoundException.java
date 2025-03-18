package ru.yandex.ci.core.exceptions;

public class CiNotFoundException extends RuntimeException {
    public CiNotFoundException() {
    }

    public CiNotFoundException(String message) {
        super(message);
    }

    public CiNotFoundException(String message, Throwable cause) {
        super(message, cause);
    }

    public CiNotFoundException(Throwable cause) {
        super(cause);
    }

    public CiNotFoundException(String message, Throwable cause, boolean enableSuppression, boolean writableStackTrace) {
        super(message, cause, enableSuppression, writableStackTrace);
    }
}
