package ru.yandex.ci.storage.core.exceptions;

import ru.yandex.ci.core.exceptions.CiNotFoundException;

public class CheckTaskNotFoundException extends CiNotFoundException {
    public CheckTaskNotFoundException(String message) {
        super(message);
    }
}
