package ru.yandex.monlib.metrics.encode.spack;

/**
 * @author Sergey Polovko
 */
public class SpackException extends RuntimeException {

    public SpackException(String message) {
        super(message);
    }

    public SpackException(String message, Throwable cause) {
        super(message, cause);
    }
}
