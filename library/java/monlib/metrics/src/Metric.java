package ru.yandex.monlib.metrics;

/**
 * @author Sergey Polovko
 */
public interface Metric {

    MetricType type();

    void accept(long tsMillis, MetricConsumer consumer);

}
