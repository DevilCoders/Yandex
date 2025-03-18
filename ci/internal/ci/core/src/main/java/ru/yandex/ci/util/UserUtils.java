package ru.yandex.ci.util;

import ru.yandex.lang.NonNullApi;

@NonNullApi
public final class UserUtils {

    private UserUtils() {
    }

    public static String loginForInternalCiProcesses() {
        return "robot-ci";
    }

}
