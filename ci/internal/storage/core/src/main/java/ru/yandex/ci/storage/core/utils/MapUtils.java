package ru.yandex.ci.storage.core.utils;

public class MapUtils {

    private MapUtils() {

    }

    @SuppressWarnings("DoNotCallSuggester")
    public static <T> T forbidMerge(T a, T b) {
        throw new RuntimeException("Illegal merge of %s and %s".formatted(a, b));
    }

    @SuppressWarnings("DoNotCallSuggester")
    public static <T> T forbidMergeIfNotEquals(T a, T b) {
        if (a.equals(b)) {
            return a;
        }

        throw new RuntimeException("Illegal merge of not equals %s and %s".formatted(a, b));
    }

    public static <T extends Comparable<T>> T max(T a, T b) {
        return a.compareTo(b) > 0 ? a : b;
    }
}
