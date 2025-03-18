package ru.yandex.ci.core.exceptions;

public class CiDuplicateException extends RuntimeException {
    public CiDuplicateException() {
    }

    public CiDuplicateException(String message) {
        super(message);
    }

    public CiDuplicateException(String message, Throwable cause) {
        super(message, cause);
    }

    public CiDuplicateException(Throwable cause) {
        super(cause);
    }

    public CiDuplicateException(String message, Throwable cause, boolean enableSuppression,
                                boolean writableStackTrace) {
        super(message, cause, enableSuppression, writableStackTrace);
    }
}
