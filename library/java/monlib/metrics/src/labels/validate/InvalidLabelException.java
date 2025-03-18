package ru.yandex.monlib.metrics.labels.validate;

/**
 * @author Sergey Polovko
 */
public class InvalidLabelException extends RuntimeException {

    public InvalidLabelException(String message) {
        super(message);
    }
}
