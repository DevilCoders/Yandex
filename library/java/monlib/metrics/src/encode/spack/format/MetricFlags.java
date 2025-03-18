package ru.yandex.monlib.metrics.encode.spack.format;

/**
 * @author Sergey Polovko
 */
public final class MetricFlags {
    private MetricFlags() {}

    public static boolean isMemOnly(byte value) {
        return (value & 0x01) != 0;
    }
}
