package ru.yandex.monlib.metrics.encode;

import javax.annotation.ParametersAreNonnullByDefault;

/**
 * @author Oleg Baryshnikov
 */
@ParametersAreNonnullByDefault
public class ParseException extends RuntimeException {

    public ParseException(String message) {
        super(message);
    }

    public ParseException(String message, Throwable cause) {
        super(message, cause);
    }
}
