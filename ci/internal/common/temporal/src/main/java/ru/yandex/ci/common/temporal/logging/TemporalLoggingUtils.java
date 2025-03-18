package ru.yandex.ci.common.temporal.logging;

import org.apache.logging.log4j.ThreadContext;

class TemporalLoggingUtils {

    private static final String TEMPORAL = "Temporal";

    private TemporalLoggingUtils() {
    }

    public static void setContext() {
        ThreadContext.put(TEMPORAL, "true");
    }

    public static void clearContext() {
        ThreadContext.remove(TEMPORAL);
    }
}
