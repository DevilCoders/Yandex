package ru.yandex.ci.core.arc;

public class BadRevisionFormatException extends RuntimeException {
    public BadRevisionFormatException(String message) {
        super(message);
    }
}
