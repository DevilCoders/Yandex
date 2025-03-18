package ru.yandex.ci.client.base.http;

import lombok.Value;

@Value(staticConstructor = "of")
public class LoggingConfig {

    private static final LoggingConfig NONE = LoggingConfig.of(false, false, false, false, false);
    private static final LoggingConfig REQUEST_BODIES = LoggingConfig.of(false, true, false, false, false);
    private static final LoggingConfig ALL = LoggingConfig.of(false, true, true, true, false);
    private static final LoggingConfig ALL_UNSAFE = LoggingConfig.of(true, true, true, true, false);
    private static final LoggingConfig ALL_LIMITED = LoggingConfig.of(false, true, true, true, true);

    // Never log request headers, could expose secret keys
    boolean logRequestHeaders;
    boolean logRequestBody;
    boolean logResponseHeaders;
    boolean logResponseBody;
    boolean limitResponseBodySize;

    public static LoggingConfig none() {
        return NONE;
    }

    public static LoggingConfig standard() {
        return REQUEST_BODIES;
    }

    public static LoggingConfig all() {
        return ALL;
    }

    public static LoggingConfig allUnsafe() {
        return ALL_UNSAFE;
    }

    public static LoggingConfig allLimited() {
        return ALL_LIMITED;
    }

}
