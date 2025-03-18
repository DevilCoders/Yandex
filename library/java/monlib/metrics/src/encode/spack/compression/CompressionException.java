package ru.yandex.monlib.metrics.encode.spack.compression;

import ru.yandex.monlib.metrics.encode.spack.SpackException;


/**
 * @author Sergey Polovko
 */
final class CompressionException extends SpackException {

    CompressionException(String message) {
        super(message);
    }

    CompressionException(String message, Throwable cause) {
        super(message, cause);
    }
}
