package ru.yandex.ci.util;

import javax.annotation.Nullable;

public class ObjectUtils {

    private ObjectUtils() {
    }

    @Nullable
    public static <T> T firstNonNullOrNull(@Nullable T first, T second) {
        if (first != null) {
            return first;
        }
        return second;
    }

}
