package ru.yandex.monlib.metrics.encode.prometheus;

/**
 * @author Sergey Polovko
 */
final class Ascii {
    private Ascii() {}

    static boolean isSpace(byte ch) {
        return ch == ' ' || ch == '\t';
    }

    static byte toUpper(byte ch) {
        return (byte) (ch & 0x5f);
    }
}
