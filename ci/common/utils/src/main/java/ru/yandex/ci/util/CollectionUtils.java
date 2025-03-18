package ru.yandex.ci.util;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

public class CollectionUtils {

    private CollectionUtils() {
        //
    }

    public static <T> List<T> extendDistinct(@Nullable List<T> base, @Nullable List<T> extend) {
        if (base != null && extend != null) {
            var result = new LinkedHashSet<T>(base.size() + extend.size());
            result.addAll(base);
            result.addAll(extend);
            return List.copyOf(result);
        } else if (base != null) {
            return base;
        } else if (extend != null) {
            return extend;
        } else {
            return List.of();
        }
    }

    public static <K> Set<K> linkedSet() {
        return Set.of();
    }

    /**
     * Когда {@link Set#of()} использовать нельзя
     *
     * @param key  ключ
     * @param keys список ключей
     * @param <K>  ключ
     * @return set с упорядоченными значениями
     */
    @SuppressWarnings("unchecked")
    public static <K> Set<K> linkedSet(@Nonnull K key, @Nonnull K... keys) {
        if (keys.length == 0) {
            return Set.of(key);
        }
        Set<K> target = new LinkedHashSet<>(keys.length + 1);
        target.add(key);
        Collections.addAll(target, keys);
        return Collections.unmodifiableSet(target);
    }

    public static <K, V> Map<K, V> linkedMap() {
        return Map.of();
    }

    /**
     * Когда {@link Map#of()} использовать нельзя
     *
     * @param key   ключ
     * @param value значение
     * @param kv    список ключей и значений
     * @param <K>   ключ
     * @param <V>   значение
     * @return map с упорядоченными значениями
     */
    @SuppressWarnings("unchecked")
    public static <K, V> Map<K, V> linkedMap(@Nonnull K key, @Nonnull V value, @Nonnull Object... kv) {
        if (kv.length == 0) {
            return Map.of(key, value);
        }

        if ((kv.length & 1) != 0) {
            throw new IllegalArgumentException("kv pair is odd");
        }

        int size = 1 + (kv.length >> 1);
        Map<K, V> target = new LinkedHashMap<>(size);
        target.put(key, value);
        for (int i = 0; i < kv.length; i += 2) {
            target.put((K) Objects.requireNonNull(kv[i]), (V) Objects.requireNonNull(kv[i + 1]));
        }
        return Collections.unmodifiableMap(target);
    }

    @SafeVarargs
    public static <T> List<T> join(List<T>... lists) {
        var result = new ArrayList<T>();
        for (var list : lists) {
            result.addAll(list);
        }
        return result;
    }

}
