package yandex.cloud.util;

import java.util.LinkedHashMap;
import java.util.Map;
import java.util.function.BinaryOperator;
import java.util.function.Function;
import java.util.stream.Collector;
import java.util.stream.Collectors;

import static java.util.stream.Collector.Characteristics.IDENTITY_FINISH;

/**
 * Better versions of collectors from {@link Collectors java.util.stream.Collectors}.
 *
 * @author entropia
 */
public final class BetterCollectors {
    private BetterCollectors() {
        throw new InternalError("This should never happen");
    }

    /**
     * A variant of {@link Collectors#toMap(Function, Function) Collectors.toMap()} that allows
     * {@code valueMapper} to produce {@code null} values for map entries.
     *
     * @param keyMapper   a mapping function to produce keys
     * @param valueMapper a mapping function to produce values
     * @param <T>         the type of input elements
     * @param <K>         the type of keys
     * @param <U>         the type of values
     * @return a map produced from the source stream, possibly with null values
     * @see <a href="https://bugs.openjdk.java.net/browse/JDK-8148463">JDK-8148463 Bug</a>
     * @see <a href="https://stackoverflow.com/questions/24630963/java-8-nullpointerexception-in-collectors-tomap">The Workaround</a>
     */
    public static <T, K, U> Collector<T, ?, Map<K, U>> toMapNullFriendly(Function<? super T, ? extends K> keyMapper,
                                                                         Function<? super T, ? extends U> valueMapper) {
        return Collector.of(
                LinkedHashMap::new,
                (map, elem) -> map.compute(keyMapper.apply(elem), (key, oldValue) -> {
                    if (oldValue != null) {
                        throw new IllegalStateException("Duplicate key: " + key);
                    }
                    return valueMapper.apply(elem);
                }),
                (m1, m2) -> {
                    for (Map.Entry<K, U> e : m2.entrySet()) {
                        if (m1.putIfAbsent(e.getKey(), e.getValue()) != null) {
                            throw new IllegalStateException("Duplicate key: " + e.getKey());
                        }
                    }
                    return m1;
                },
                IDENTITY_FINISH
        );
    }

    // copy/paste from Collectors#throwingMerger (since it's private)
    public static <T> BinaryOperator<T> throwingMerger() {
        return (u, v) -> {
            throw new IllegalStateException(String.format("Duplicate key %s", u));
        };
    }

}
