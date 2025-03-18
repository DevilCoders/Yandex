package ru.yandex.monlib.metrics.encode.spack.compression;

/**
 * @author Sergey Polovko
 */
interface Checksum {

    int calc(byte[] buffer, int offset, int length);

    default void check(byte[] buffer, int offset, int length, int checksum) {
        if (calc(buffer, offset, length) != checksum) {
            throw new CompressionException("stream corrupted: checksum mismatch");
        }
    }
}
