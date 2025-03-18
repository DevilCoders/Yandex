package ru.yandex.monlib.metrics.encode.prometheus;

import java.nio.ByteBuffer;

import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.encode.MetricDecoder;


/**
 * @author Sergey Polovko
 */
public class MetricPrometheusDecoder implements MetricDecoder {

    @Override
    public void decode(byte[] buffer, MetricConsumer consumer) {
        decode(ByteBuffer.wrap(buffer), consumer);
    }

    @Override
    public void decode(ByteBuffer buffer, MetricConsumer consumer) {
        PrometheusParserImpl parser = new PrometheusParserImpl(buffer, consumer);
        parser.parse();
    }
}
