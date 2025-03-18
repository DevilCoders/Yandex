package ru.yandex.ci.storage.core.utils;

public class MathUtils {
    private MathUtils() {

    }

    public static double percent(double obtained, double total) {
        return obtained * 100d / total;
    }

    public static int intPercent(double obtained, double total) {
        return (int) Math.round(obtained * 100d / total);
    }
}
