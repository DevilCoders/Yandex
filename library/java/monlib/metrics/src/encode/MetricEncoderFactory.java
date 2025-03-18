package ru.yandex.monlib.metrics.encode;

import java.io.OutputStream;

import ru.yandex.monlib.metrics.encode.json.MetricJsonEncoder;
import ru.yandex.monlib.metrics.encode.prometheus.MetricPrometheusEncoder;
import ru.yandex.monlib.metrics.encode.spack.MetricSpackEncoder;
import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg;
import ru.yandex.monlib.metrics.encode.spack.format.TimePrecision;
import ru.yandex.monlib.metrics.encode.text.MetricTextEncoder;

/**
 * @author Vladimir Gordiychuk
 */
public final class MetricEncoderFactory {
    private MetricEncoderFactory() {
    }

    public static MetricEncoder createEncoder(OutputStream out, MetricFormat format, CompressionAlg compression) {
        switch (format) {
            case TEXT: return new MetricTextEncoder(out, true);
            case JSON: return new MetricJsonEncoder(out);
            case SPACK: return new MetricSpackEncoder(TimePrecision.SECONDS, compression, out);
            case PROMETHEUS: return new MetricPrometheusEncoder(out);
        }
        throw new IllegalStateException("unknown metrics format: " + format);
    }
}
