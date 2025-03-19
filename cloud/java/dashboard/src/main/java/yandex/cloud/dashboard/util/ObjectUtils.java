package yandex.cloud.dashboard.util;

import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Ordering;
import lombok.experimental.UtilityClass;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.function.Function;
import java.util.function.Supplier;
import java.util.function.UnaryOperator;
import java.util.stream.Stream;

import static java.util.stream.Collectors.toList;

/**
 * @author ssytnik
 */
@UtilityClass
public class ObjectUtils {

    @SafeVarargs
    public static <T> T firstNonNull(T... values) {
        return Stream.of(values).filter(Objects::nonNull).findFirst().orElseThrow();
    }

    @SafeVarargs
    public static <T> T firstNonNullOrNull(T... values) {
        return Stream.of(values).filter(Objects::nonNull).findFirst().orElse(null);
    }

    public static <T, R> R mapOrDefault(T t, Function<T, R> mapper, R defaultValue) {
        return t == null ? defaultValue : mapper.apply(t);
    }

    public static <T, R> R mapOrNull(T t, Function<T, R> mapper) {
        return mapOrDefault(t, mapper, null);
    }

    public static <T> T takeOrDefault(T t, Supplier<T> defaultSupplier) {
        return t == null ? defaultSupplier.get() : t;
    }

    public static <T, R> List<R> mapList(List<T> list, Function<T, R> mapper, List<R> defaultValue) {
        return list == null ? defaultValue : list.stream().map(mapper).collect(toList());
    }

    public static <T, R> List<R> mapList(List<T> list, Function<T, R> mapper) {
        return mapList(list, mapper, List.of());
    }

    @SafeVarargs
    public static <T> List<T> addToList(List<T> list, T... elements) {
        return addToListIf(true, list, elements);
    }

    @SafeVarargs
    public static <T> List<T> addToListIf(boolean condition, List<T> list, T... elements) {
        if (condition && elements.length > 0) {
            return ImmutableList.<T>builder()
                    .addAll(firstNonNullOrNull(list, List.of()))
                    .add(elements)
                    .build();
        } else {
            return list;
        }
    }

    public static <T extends Comparable<T>> List<T> sorted(List<T> list) {
        return Ordering.natural().sortedCopy(list);
    }

    public static <K, V> Map<K, V> addToMap(Map<K, V> map, K key, V value) {
        return addToMapIf(true, map, key, value);
    }

    public static <K, V> Map<K, V> addToMapIf(boolean condition, Map<K, V> map, K key, V value) {
        if (condition) {
            return ImmutableMap.<K, V>builder()
                    .putAll(map)
                    .put(key, value)
                    .build();
        } else {
            return map;
        }
    }

    /**
     * Creates result list of the specified <code>resultSize</code>.
     * <p>
     * At first, elements from the nullable source <code>list</code>, mapped with the <code>itemMapper</code>,
     * are put to the result list (list size <= resultSize), which is then padded with <code>defaultValue</code> elements.
     */
    public static <T, R> List<R> mapListOfSize(int resultSize, List<T> list, Function<T, R> itemMapper, Supplier<R> defaultValue) {
        ArrayList<R> result = new ArrayList<>(resultSize);
        if (list != null) {
            Preconditions.checkArgument(list.size() <= resultSize);
            result.addAll(list.stream().map(itemMapper).collect(toList()));
        }
        if (result.size() < resultSize) {
            result.addAll(Collections.nCopies(resultSize - result.size(), defaultValue.get()));
        }
        return result;
    }

    public static boolean isValidDouble(String s) {
        try {
            return Double.isFinite(Double.parseDouble(s));
        } catch (NumberFormatException e) {
            return false;
        }
    }

    public static <T> T modifyWhileChanging(T source, UnaryOperator<T> modifier) {
        T current;
        T next = source;
        do {
            current = next;
            next = modifier.apply(current);
        } while (!Objects.equals(current, next));
        return next;
    }

    public static Boolean negate(Boolean b) {
        return b == null ? null : !b;
    }

    @SuppressWarnings("unchecked")
    public static <T, R> Stream<R> filterAndCast(T t, Class<R> clazz) {
        return clazz.isInstance(t) ? Stream.of((R) t) : Stream.empty();
    }
}
