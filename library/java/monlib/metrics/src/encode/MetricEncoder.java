package ru.yandex.monlib.metrics.encode;

import ru.yandex.monlib.metrics.MetricConsumer;


/**
 * @author Sergey Polovko
 */
public interface MetricEncoder extends MetricConsumer, AutoCloseable {
    @Override
    void close();
}
