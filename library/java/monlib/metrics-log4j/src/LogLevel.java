package ru.yandex.monlib.metrics;

import org.apache.log4j.Level;

/**
 * @author Vladimir Gordiychuk
 */
public enum LogLevel {
    UNKNOWN,
    OFF,
    FATAL,
    ERROR,
    WARN,
    INFO,
    DEBUG,
    TRACE,
    ALL,

    ;

    public static LogLevel valueOf(Level level) {
        switch (level.toInt()) {
            case Level.OFF_INT:
                return OFF;
            case Level.FATAL_INT:
                return FATAL;
            case Level.ERROR_INT:
                return ERROR;
            case Level.WARN_INT:
                return WARN;
            case Level.INFO_INT:
                return INFO;
            case Level.DEBUG_INT:
                return DEBUG;
            case Level.TRACE_INT:
                return TRACE;
            case Level.ALL_INT:
                return ALL;
            default:
                return UNKNOWN;
        }
    }
}
