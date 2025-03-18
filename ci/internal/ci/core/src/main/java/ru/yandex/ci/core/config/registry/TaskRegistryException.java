package ru.yandex.ci.core.config.registry;

public class TaskRegistryException extends RuntimeException {
    public TaskRegistryException(String yamlSource) {
        super(yamlSource);
    }

    public TaskRegistryException(Throwable cause) {
        super(cause);
    }
}
