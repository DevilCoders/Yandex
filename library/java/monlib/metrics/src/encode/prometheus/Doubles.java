package ru.yandex.monlib.metrics.encode.prometheus;

/**
 * @author Sergey Polovko
 */
final class Doubles {
    private Doubles() {}

    static double fromString(String val) {
        if ("-Inf".equalsIgnoreCase(val)) {
            return Double.NEGATIVE_INFINITY;
        }
        if ("+Inf".equalsIgnoreCase(val) || "Inf".equalsIgnoreCase(val)) {
            return Double.POSITIVE_INFINITY;
        }
        if ("NaN".equalsIgnoreCase(val)) {
            return Double.NaN;
        }
        return Double.parseDouble(val);
    }

    static String toString(double val) {
        if (Double.isNaN(val)) {
            return "NaN";
        }
        if (val == Double.POSITIVE_INFINITY) {
            return "+Inf";
        }
        if (val == Double.NEGATIVE_INFINITY) {
            return "-Inf";
        }
        return Double.toString(val);
    }
}
