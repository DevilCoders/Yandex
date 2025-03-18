package ru.yandex.monlib.metrics.encode.json;

import java.nio.ByteBuffer;

import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.encode.MetricDecoder;


/**
 * @author Sergey Polovko
 */
public class MetricJsonDecoder implements MetricDecoder {

    @Override
    public void decode(byte[] buffer, MetricConsumer consumer) {

    }

    @Override
    public void decode(ByteBuffer buffer, MetricConsumer consumer) {

    }
}
