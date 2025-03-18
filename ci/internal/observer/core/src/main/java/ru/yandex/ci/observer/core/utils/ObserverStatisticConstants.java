package ru.yandex.ci.observer.core.utils;

import java.util.List;

import ru.yandex.ci.storage.core.CheckIteration;

public final class ObserverStatisticConstants {

    public static final List<CheckIteration.IterationType> ITERATION_TYPES_USED_IN_STATISTICS = List.of(
            CheckIteration.IterationType.FAST,
            CheckIteration.IterationType.FULL
    );

    private ObserverStatisticConstants() {
    }
}
