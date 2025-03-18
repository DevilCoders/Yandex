package ru.yandex.ci.tms.task.autocheck.degradation;

import lombok.AllArgsConstructor;
import lombok.Value;

@Value
@AllArgsConstructor
public class AutomaticDegradation {

    private static final int LIMIT_DEFAULT = 100;

    boolean disabled;
    int limit;

    public static AutomaticDegradation enabled() {
        return new AutomaticDegradation(false, LIMIT_DEFAULT);
    }

    public static AutomaticDegradation disabled() {
        return new AutomaticDegradation(true, LIMIT_DEFAULT);
    }

    public static int getLimitDefault() {
        return LIMIT_DEFAULT;
    }
}
