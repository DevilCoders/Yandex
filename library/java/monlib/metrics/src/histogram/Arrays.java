package ru.yandex.monlib.metrics.histogram;

import java.util.concurrent.atomic.AtomicLongArray;


/**
 * @author Sergey Polovko
 */
public final class Arrays {
    private Arrays() {}

    static boolean isSorted(double[] array) {
        if (array.length < 2) {
            return true;
        }
        double previous = array[0];
        for (int i = 1; i < array.length; i++) {
            final double current = array[i];
            if (previous > current) {
                return false;
            }
            previous = current;
        }
        return true;
    }

    static long[] copyOf(AtomicLongArray original) {
        final int len = original.length();
        final long[] copy = new long[len];
        for (int i = 0; i < len; i++) {
            copy[i] = original.get(i);
        }
        return copy;
    }

    public static int lowerBound(double[] bounds, long value) {
        return lowerBound(bounds, bounds.length, value);
    }

    public static int lowerBound(double[] bounds, int size, long value) {
        int res = size;
        int l = 0;
        int r = size - 1;
        while (l <= r) {
            final int mid = l + (r - l) / 2;
            if (bounds[mid] >= value) {
                res = mid;
                r = mid - 1;
            } else {
                l = mid + 1;
            }
        }
        return res;
    }

    public static int lowerBound(double[] bounds, double value) {
        return lowerBound(bounds, bounds.length, value);
    }

    public static int lowerBound(double[] bounds, int size, double value) {
        int res = size;
        int l = 0;
        int r = size - 1;
        while (l <= r) {
            final int mid = l + (r - l) / 2;
            if (Double.compare(bounds[mid], value) >= 0) {
                res = mid;
                r = mid - 1;
            } else {
                l = mid + 1;
            }
        }
        return res;
    }

    static void toString(StringBuilder sb, AtomicLongArray a) {
        sb.append('[');
        for (int i = 0, len = a.length(); i < len; i++) {
            if (i > 0) {
                sb.append(", ");
            }
            sb.append(a.get(i));
        }
        sb.append(']');
    }
}
