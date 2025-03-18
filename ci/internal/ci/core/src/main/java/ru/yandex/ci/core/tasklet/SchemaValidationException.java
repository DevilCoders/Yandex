package ru.yandex.ci.core.tasklet;

public class SchemaValidationException extends SchemaException {
    public SchemaValidationException(String message) {
        super(message);
    }

    public SchemaValidationException(String message, Throwable cause) {
        super(message, cause);
    }
}
