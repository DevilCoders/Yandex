package ru.yandex.monlib.metrics.labels;

/**
 * @author Sergey Polovko
 */
public interface LabelAllocator {

    Label alloc(String key, String value);
}
