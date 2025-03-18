package ru.yandex.ci.storage.core.cache.impl;

import ru.yandex.ci.storage.core.exceptions.StorageException;

public class StaleModificationException extends StorageException {
    public StaleModificationException(String message) {
        super(message);
    }
}
