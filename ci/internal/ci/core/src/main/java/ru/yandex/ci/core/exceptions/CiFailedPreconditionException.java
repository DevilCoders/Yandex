package ru.yandex.ci.core.exceptions;

public class CiFailedPreconditionException extends RuntimeException {
    public CiFailedPreconditionException() {
    }

    public CiFailedPreconditionException(String message) {
        super(message);
    }

    public CiFailedPreconditionException(String message, Throwable cause) {
        super(message, cause);
    }

    public CiFailedPreconditionException(Throwable cause) {
        super(cause);
    }

    public CiFailedPreconditionException(String message, Throwable cause, boolean enableSuppression,
                                         boolean writableStackTrace) {
        super(message, cause, enableSuppression, writableStackTrace);
    }
}
