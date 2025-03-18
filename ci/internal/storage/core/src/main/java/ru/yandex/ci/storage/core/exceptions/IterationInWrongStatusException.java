package ru.yandex.ci.storage.core.exceptions;

import ru.yandex.ci.core.exceptions.CiFailedPreconditionException;

public class IterationInWrongStatusException extends CiFailedPreconditionException {
    public IterationInWrongStatusException(String message) {
        super(message);
    }
}
