package ru.yandex.ci.flow.engine.definition.common;

public class InstantiateException extends RuntimeException {
    public InstantiateException() {
    }

    public InstantiateException(String message) {
        super(message);
    }

    public InstantiateException(String message, Throwable cause) {
        super(message, cause);
    }

    public InstantiateException(Throwable cause) {
        super(cause);
    }
}
