package ru.yandex.monlib.metrics.encode.text;

import java.nio.ByteBuffer;

import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.encode.MetricDecoder;


/**
 * @author Sergey Polovko
 */
public class MetricTextDecoder implements MetricDecoder {

    @Override
    public void decode(byte[] buffer, MetricConsumer consumer) {
    }

    @Override
    public void decode(ByteBuffer buffer, MetricConsumer consumer) {
    }
}
