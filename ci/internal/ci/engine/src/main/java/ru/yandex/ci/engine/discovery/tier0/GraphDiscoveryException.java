package ru.yandex.ci.engine.discovery.tier0;

public class GraphDiscoveryException extends RuntimeException {
    public GraphDiscoveryException(String message) {
        super(message);
    }

    public GraphDiscoveryException(Throwable cause) {
        super(cause);
    }

    public GraphDiscoveryException(String message, Throwable cause) {
        super(message, cause);
    }
}
