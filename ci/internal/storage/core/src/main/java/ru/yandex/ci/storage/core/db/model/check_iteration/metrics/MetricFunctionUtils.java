package ru.yandex.ci.storage.core.db.model.check_iteration.metrics;

import ru.yandex.ci.storage.core.Common;

public class MetricFunctionUtils {
    private MetricFunctionUtils() {

    }

    public static double apply(Common.MetricAggregateFunction function, double a, double b) {
        return switch (function) {
            case SUM -> a + b;
            case MAX -> Math.max(a, b);
            case UNRECOGNIZED -> throw new IllegalArgumentException("unrecognized func " + function);
        };
    }

    public static double aggregate(
            Common.MetricAggregateFunction function,
            double aggregated, double old, double updated
    ) {
        return switch (function) {
            case SUM -> aggregated - old + updated;
            case MAX -> Math.max(aggregated, Math.max(old, updated));
            case UNRECOGNIZED -> throw new IllegalArgumentException("unrecognized func " + function);
        };
    }
}
