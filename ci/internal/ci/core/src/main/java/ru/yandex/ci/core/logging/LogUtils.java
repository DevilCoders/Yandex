package ru.yandex.ci.core.logging;

import java.util.List;
import java.util.Map;
import java.util.function.Consumer;

import org.apache.logging.log4j.CloseableThreadContext;
import org.apache.logging.log4j.ThreadContext;

public class LogUtils {
    private LogUtils() {

    }

    public static Runnable withCurrentThreadLogContext(Runnable action) {
        final Map<String, String> values = ThreadContext.getImmutableContext();
        final List<String> messages = ThreadContext.getImmutableStack().asList();

        return () -> {
            try (var ignore = CloseableThreadContext.putAll(values).pushAll(messages)) {
                action.run();
            }
        };
    }

    public static <T> Consumer<T> withCurrentThreadLogContext(Consumer<T> action) {
        final Map<String, String> values = ThreadContext.getImmutableContext();
        final List<String> messages = ThreadContext.getImmutableStack().asList();

        return (value) -> {
            try (var ignore = CloseableThreadContext.putAll(values).pushAll(messages)) {
                action.accept(value);
            }
        };
    }
}
