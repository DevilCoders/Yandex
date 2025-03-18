package ru.yandex.monlib.metrics.histogram;

/**
 * @author Sergey Polovko
 */
public final class Doubles {
    private Doubles() {}

    public static String toString(double value) {
        if (Double.doubleToRawLongBits(value - (long) value) == 0L) {
            return Long.toString((long) value);
        }
        return Double.toString(value);
    }
}
