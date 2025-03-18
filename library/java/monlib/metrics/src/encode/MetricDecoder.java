package ru.yandex.monlib.metrics.encode;

import java.nio.ByteBuffer;

import ru.yandex.monlib.metrics.MetricConsumer;


/**
 * @author Sergey Polovko
 */
public interface MetricDecoder {

    void decode(byte[] buffer, MetricConsumer consumer);
    void decode(ByteBuffer buffer, MetricConsumer consumer);
}
