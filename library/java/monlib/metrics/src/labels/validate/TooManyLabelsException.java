package ru.yandex.monlib.metrics.labels.validate;

/**
 * @author Sergey Polovko
 */
public class TooManyLabelsException extends RuntimeException {

    public TooManyLabelsException(String message) {
        super(message);
    }
}
