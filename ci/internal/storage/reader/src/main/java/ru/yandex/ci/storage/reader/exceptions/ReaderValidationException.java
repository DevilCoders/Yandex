package ru.yandex.ci.storage.reader.exceptions;

import ru.yandex.ci.storage.core.exceptions.StorageException;

public class ReaderValidationException extends StorageException {
    public ReaderValidationException(String message) {
        super(message);
    }

    public ReaderValidationException(String message, Throwable cause) {
        super(message, cause);
    }
}
