package ru.yandex.ci.core.config.a;

// this exception extends IllegalStateException for backward compatibility
public class AYamlNotFoundException extends IllegalStateException {
    public AYamlNotFoundException(String message) {
        super(message);
    }
}
