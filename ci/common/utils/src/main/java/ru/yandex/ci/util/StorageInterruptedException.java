package ru.yandex.ci.util;

public class StorageInterruptedException extends RuntimeException {

    public StorageInterruptedException() {
    }

    public StorageInterruptedException(String message) {
        super(message);
    }

    public StorageInterruptedException(String message, Throwable cause) {
        super(message, cause);
    }

    public StorageInterruptedException(Throwable cause) {
        super(cause);
    }

    public StorageInterruptedException(String message, Throwable cause, boolean enableSuppression,
                                       boolean writableStackTrace) {
        super(message, cause, enableSuppression, writableStackTrace);
    }
}
