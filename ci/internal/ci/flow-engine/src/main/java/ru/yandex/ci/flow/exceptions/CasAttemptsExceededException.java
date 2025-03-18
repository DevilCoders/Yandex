package ru.yandex.ci.flow.exceptions;

public class CasAttemptsExceededException extends RuntimeException {
    public CasAttemptsExceededException(String message) {
        super(message);
    }
}
