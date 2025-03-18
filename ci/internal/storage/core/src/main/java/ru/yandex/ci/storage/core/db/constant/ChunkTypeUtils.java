package ru.yandex.ci.storage.core.db.constant;

import ru.yandex.ci.storage.core.Common;

public class ChunkTypeUtils {
    private ChunkTypeUtils() {

    }

    public static String toHumanReadable(Common.ChunkType chunkType) {
        return switch (chunkType) {
            case CT_CONFIGURE -> "configure";
            case CT_BUILD -> "build";
            case CT_STYLE -> "style tests";
            case CT_SMALL_TEST -> "small tests";
            case CT_MEDIUM_TEST -> "medium tests";
            case CT_LARGE_TEST -> "large tests";
            case CT_TESTENV -> "testenv tests";
            case CT_NATIVE_BUILD -> "native builds";
            case UNRECOGNIZED -> throw new RuntimeException();
        };
    }
}
