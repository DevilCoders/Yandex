package ru.yandex.ci.storage.core.utils;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;
import java.util.stream.Stream;

public class StreamBatchUtils {
    private StreamBatchUtils() {
    }

    public static <T> void process(Stream<T> stream, int batchSize, Consumer<List<T>> consumer) {
        var buffer = new ArrayList<T>(batchSize);
        stream.forEach(value -> {
                    if (buffer.size() < batchSize) {
                        buffer.add(value);
                    } else {
                        consumer.accept(buffer);
                        buffer.clear();
                        buffer.add(value);
                    }
                }
        );

        if (buffer.size() > 0) {
            consumer.accept(buffer);
        }
    }
}
