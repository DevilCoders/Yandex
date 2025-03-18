package ru.yandex.ci.storage.core.exceptions;

import ru.yandex.ci.core.exceptions.CiNotFoundException;

public class CheckNotFoundException extends CiNotFoundException {
    public CheckNotFoundException(String message) {
        super(message);
    }
}
