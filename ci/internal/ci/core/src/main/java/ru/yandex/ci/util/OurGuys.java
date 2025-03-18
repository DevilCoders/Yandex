package ru.yandex.ci.util;

import java.util.Set;

import javax.annotation.Nullable;

public class OurGuys {

    private static final Set<String> VALUE = Set.of(
            "albazh",
            "andreevdm",
            "firov",
            "miroslav2",
            "user42",
            "pochemuto"
    );

    private OurGuys() {

    }

    public static boolean isOurGuy(@Nullable String login) {
        if (login == null) {
            return false;
        }

        return VALUE.contains(login);
    }
}
