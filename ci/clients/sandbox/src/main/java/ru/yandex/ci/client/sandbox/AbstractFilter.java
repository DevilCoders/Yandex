package ru.yandex.ci.client.sandbox;

import java.util.AbstractMap;
import java.util.Collection;
import java.util.Map;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

public abstract class AbstractFilter extends AbstractMap<String, Object> {
    protected static void addNonNull(Map<String, Object> target, String key, @Nullable Object value) {
        if (value == null) {
            return;
        }

        target.put(key, value);
    }

    protected static void addNonNull(Map<String, Object> target, String key, Collection<?> value) {
        if (value.isEmpty()) {
            return;
        }

        var joinedString = value.stream().map(String::valueOf).collect(Collectors.joining(","));
        target.put(key, joinedString);
    }

}
