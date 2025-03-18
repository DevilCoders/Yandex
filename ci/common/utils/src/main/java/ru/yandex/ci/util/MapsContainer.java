package ru.yandex.ci.util;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.atomic.AtomicReference;

import javax.annotation.Nullable;

public class MapsContainer implements Clearable {
    private final Collection<Map<?, ?>> maps = new ArrayList<>();

    public <K, V> ConcurrentMap<K, V> concurrentMap() {
        return concurrentMap(null);
    }

    public <K, V> ConcurrentMap<K, V> concurrentMap(@Nullable AtomicReference<V> defaultValue) {
        var map = new ConcurrentHashMap<K, V>() {
            @Nullable
            @Override
            public V get(Object key) {
                var value = super.get(key);
                if (value == null && defaultValue != null) {
                    return defaultValue.get();
                } else {
                    return value;
                }
            }
        };
        maps.add(map);
        return map;
    }

    @Override
    public void clear() {
        maps.forEach(Map::clear);
    }

}
