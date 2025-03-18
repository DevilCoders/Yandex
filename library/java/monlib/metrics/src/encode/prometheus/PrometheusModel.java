package ru.yandex.monlib.metrics.encode.prometheus;

/**
 * Prometheus specific names and validation rules.
 * See <a href="https://github.com/prometheus/common/blob/master/expfmt/text_parse.go">Prometheus text parser</a>
 *
 * @author Sergey Polovko
 */
final class PrometheusModel {

    private static final String BUCKET_SUFFIX = "_bucket";
    private static final String COUNT_SUFFIX = "_count";
    private static final String SUM_SUFFIX = "_sum";

    private PrometheusModel() {}

    /**
     * Used for the label that defines the upper bound of a bucket of a
     * histogram ("le" -> "less or equal").
     */
    static final String BUCKET_LABEL = "le";


    static boolean isValidLabelNameStart(byte ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
    }

    static boolean isValidLabelNameContinuation(byte ch) {
        return isValidLabelNameStart(ch) || (ch >= '0' && ch <= '9');
    }

    static boolean isValidMetricNameStart(byte ch) {
        return isValidLabelNameStart(ch) || ch == ':';
    }

    static boolean isValidMetricNameContinuation(byte ch) {
        return isValidLabelNameContinuation(ch) || ch == ':';
    }

    static boolean isSum(String metricName) {
        return metricName.endsWith(SUM_SUFFIX);
    }

    static boolean isCount(String metricName) {
        return metricName.endsWith(COUNT_SUFFIX);
    }

    static boolean isBucket(String metricName) {
        return metricName.endsWith(BUCKET_SUFFIX);
    }

    static String toBaseName(String metricName) {
        if (isBucket(metricName)) {
            return metricName.substring(0, metricName.length() - BUCKET_SUFFIX.length());
        }
        if (isCount(metricName)) {
            return metricName.substring(0, metricName.length() - COUNT_SUFFIX.length());
        }
        if (isSum(metricName)) {
            return metricName.substring(0, metricName.length() - SUM_SUFFIX.length());
        }
        return metricName;
    }
}
