package ru.yandex.ci.core.tasklet;

public class SchemaException extends TaskletMetadataValidationException {
    public SchemaException(String message) {
        super(message);
    }

    public SchemaException(String message, Throwable cause) {
        super(message, cause);
    }
}
