package ru.yandex.monlib.metrics;

/**
 * The type of metric. It describes how the data is reported and processed.
 *
 * @author Sergey Polovko
 */
public enum MetricType {
    /**
     * Sentinel value. Must not be used directly.
     */
    UNKNOWN,

    /**
     * A double gauge is a metric that represents a single double value that can arbitrarily go up and down.
     * Double gauges are typically used for measured values like temperature, percent of CPU utilisation, etc.
     */
    DGAUGE,

    /**
     * An integer gauge is a metric that represents a single integer value that can arbitrarily go up and down.
     * Integer gauges are typically used for measured values like current memory usage, number of active sessions, etc.
     */
    IGAUGE,

    /**
     * A counter is a cumulative metric that represents a single monotonically increasing counter whose value
     * can only increase or be reset to zero on restart. For example, you can use a counter to represent the
     * number of requests served, tasks completed, or errors occurred.
     *
     * Do not use a counter to expose a value that can decrease. For example, do not use a counter for the number
     * of currently running processes; instead use a IGAUGE.
     */
    COUNTER,

    /**
     * A rate is a metric that represents the total number of event occurrences per second. A RATE can be used to
     * track how often something happens — like the frequency of requests made to a database, bytes transferred, etc.
     *
     * NOTE: A RATE is different from the COUNTER metric type only in whether differentiation is performed on the Solomon
     * side or not.
     */
    RATE,

    /**
     * A histogram is a metric that represents a snapshot of aggregated samples (usually things like request durations
     * or response sizes) in one time interval. Each configured bucket can be treated as GAUGE metric.
     */
    HIST,

    /**
     * A rate histogram is a metric that represents accumulated samples (usually things like request durations
     * or response sizes) over time. Each configured bucket can be treated as RATE metric.
     */
    HIST_RATE,

    /**
     * Integer summary is a metric that represents aggregated integer statistics (max, min, sum, count) in one time
     * interval. This type of metric is helpful when it is needed to track all those statistics in each interval.
     *
     * NOTE: this type of metric is currently under development.
     */
    ISUMMARY,

    /**
     * Double summary is a metric that represents aggregated double statistics (max, min, sum, count) in one time
     * interval. This type of metric is helpful when it is needed to track all those statistics in each interval.
     *
     * NOTE: this type of metric is currently under development.
     */
    DSUMMARY,

    @Deprecated
    LOG_HISTOGRAM,
    ;

    public static boolean isHistogram(MetricType type) {
        return type == HIST || type == HIST_RATE;
    }
}
