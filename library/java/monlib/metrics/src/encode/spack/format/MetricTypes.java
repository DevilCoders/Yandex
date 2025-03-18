package ru.yandex.monlib.metrics.encode.spack.format;

import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.encode.spack.SpackException;


/**
 * Helper class to get {@link MetricType} and {@link MetricValuesType} from packed byte:
 *
 * (msb)  7   6   5   4   3   2   1   0  (lsb)
 *      +---+---+---+---+---+---+---+---+
 *      |      metric type      |   vt  |
 *      +---+---+---+---+---+---+---+---+
 *
 * @author Sergey Polovko
 */
public final class MetricTypes {
    private MetricTypes() {}

    private static final byte TYPE_UNKNOWN = 0x00;
    private static final byte TYPE_DGAUGE = 0x01;
    private static final byte TYPE_COUNTER = 0x02;
    private static final byte TYPE_RATE = 0x03;
    private static final byte TYPE_IGAUGE = 0x04;
    private static final byte TYPE_HIST = 0x05;
    private static final byte TYPE_HIST_RATE = 0x06;
    private static final byte TYPE_SUMMARY_DOUBLE = 0x07;
    private static final byte TYPE_SUMMARY_INT64 = 0x08;
    private static final byte TYPE_LOG_HIST = 0x09;

    private static final byte VALUES_NONE = 0x00;
    private static final byte VALUES_ONE_WITHOUT_TS = 0x01;
    private static final byte VALUES_ONE_WITH_TS = 0x02;
    private static final byte VALUES_MANY_WITH_TS = 0x03;


    public static MetricType metricType(byte typesByte) {
        final byte typePart = (byte) ((int) typesByte >>> 2);
        switch (typePart) {
            case TYPE_UNKNOWN: return MetricType.UNKNOWN;
            case TYPE_DGAUGE: return MetricType.DGAUGE;
            case TYPE_IGAUGE: return MetricType.IGAUGE;
            case TYPE_COUNTER: return MetricType.COUNTER;
            case TYPE_RATE: return MetricType.RATE;
            case TYPE_HIST: return MetricType.HIST;
            case TYPE_HIST_RATE: return MetricType.HIST_RATE;
            case TYPE_SUMMARY_DOUBLE: return MetricType.DSUMMARY;
            case TYPE_SUMMARY_INT64: return MetricType.ISUMMARY;
            case TYPE_LOG_HIST: return MetricType.LOG_HISTOGRAM;
        }
        throw new SpackException("unknown metric type value: " + Integer.toHexString(typePart));
    }

    public static MetricValuesType valuesType(byte typesByte) {
        final byte valuesPart = (byte) (typesByte & 0x03);
        switch (valuesPart) {
            case VALUES_NONE: return MetricValuesType.NONE;
            case VALUES_ONE_WITHOUT_TS: return MetricValuesType.ONE_WITHOUT_TS;
            case VALUES_ONE_WITH_TS: return MetricValuesType.ONE_WITH_TS;
            case VALUES_MANY_WITH_TS: return MetricValuesType.MANY_WITH_TS;
        }
        // must never happen
        throw new SpackException("unknown metric values type: " + Integer.toHexString(valuesPart));
    }

    public static byte pack(MetricType type, MetricValuesType valuesType) {
        final byte typePart;
        switch (type) {
            case DGAUGE:
                typePart = TYPE_DGAUGE;
                break;
            case IGAUGE:
                typePart = TYPE_IGAUGE;
                break;
            case COUNTER:
                typePart = TYPE_COUNTER;
                break;
            case RATE:
                typePart = TYPE_RATE;
                break;
            case HIST:
                typePart = TYPE_HIST;
                break;
            case HIST_RATE:
                typePart = TYPE_HIST_RATE;
                break;
            case ISUMMARY:
                typePart = TYPE_SUMMARY_INT64;
                break;
            case DSUMMARY:
                typePart = TYPE_SUMMARY_DOUBLE;
                break;
            case LOG_HISTOGRAM:
                typePart = TYPE_LOG_HIST;
                break;
            default:
                typePart = TYPE_UNKNOWN;
                break;
        }

        final byte valuePart;
        switch (valuesType) {
            case NONE:
                valuePart = VALUES_NONE;
                break;
            case ONE_WITHOUT_TS:
                valuePart = VALUES_ONE_WITHOUT_TS;
                break;
            case ONE_WITH_TS:
                valuePart = VALUES_ONE_WITH_TS;
                break;
            case MANY_WITH_TS:
                valuePart = VALUES_MANY_WITH_TS;
                break;
            default:
                // must never happen
                throw new IllegalStateException("unknown values type: " + valuesType);
        }

        return (byte) (typePart << 2 | valuePart);
    }
}
