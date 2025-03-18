package ru.yandex.ci.engine.autocheck.model;

import java.util.Random;

import ru.yandex.ci.core.db.CiMainDb;

public final class AutocheckFeature {

    private static final String ENABLED_FOR_PERCENT = "enabledForPercent";

    private static final String FAST_TARGET_DETECTION = "AutocheckInfoCollector";

    private static final Random RANDOM = new Random();

    private AutocheckFeature() {
    }

    public static boolean isAutoFastTargetEnabledForPercent(CiMainDb db) {
        var enabledForUserPercent = db.currentOrReadOnly(() -> db.keyValue()
                .findObject(FAST_TARGET_DETECTION, ENABLED_FOR_PERCENT, Integer.class)
                .orElse(0)
        );
        return RANDOM.nextInt(100) < enabledForUserPercent;
    }
}
