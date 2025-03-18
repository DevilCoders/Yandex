package ru.yandex.ci.storage.core.exceptions;

import ru.yandex.ci.core.exceptions.CiNotFoundException;

public class IterationNotFoundException extends CiNotFoundException {
    public IterationNotFoundException(String message) {
        super(message);
    }
}
