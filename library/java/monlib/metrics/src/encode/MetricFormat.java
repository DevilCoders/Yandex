package ru.yandex.monlib.metrics.encode;

/**
 * @author Sergey Polovko
 */
public enum MetricFormat {
    UNKNOWN(""),
    JSON("application/json"),
    TEXT("application/x-solomon-txt"),
    SPACK("application/x-solomon-spack"),
    PROTOBUF("application/x-solomon-pb"),
    PROMETHEUS("text/plain")
    ;

    private static final MetricFormat[] KNOWN_VALUES = { TEXT, JSON, SPACK, PROTOBUF, PROMETHEUS };

    private final String contentType;

    MetricFormat(String contentType) {
        this.contentType = contentType;
    }

    public String contentType() {
        return contentType;
    }

    public static MetricFormat byContentType(String contentType) {
        for (MetricFormat format : KNOWN_VALUES) {
            if (format.contentType.equals(contentType)) {
                return format;
            }
        }
        return UNKNOWN;
    }
}
