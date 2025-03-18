package ru.yandex.ci.tms.data;

import java.util.Arrays;
import java.util.Map;

import javax.annotation.Nullable;

/**
 * Mapper for range and value. Also takes one extra value for case,
 * when checking R value greater than max upper bound.
 * Mapping: (x < K_1) -> V_1, (K_1 <= x < K_2) -> V_2, ..., (K_n <= x) -> extra value
 *
 * @param <V> type of value of selected range
 */
public class RangeSelector<V> {
    private final Map<Integer, V> upperBoundsMapping;
    private final V extraValue;
    private final Integer[] upperBounds;

    public RangeSelector(Map<Integer, V> upperBoundsMapping, V extraValue) {
        this.upperBoundsMapping = Map.copyOf(upperBoundsMapping);
        this.extraValue = extraValue;
        this.upperBounds = upperBoundsMapping.keySet().stream().sorted().toArray(Integer[]::new);
    }

    public Range<V> selectRange(Integer key) {
        int res = Math.abs(Arrays.binarySearch(upperBounds, key) + 1);

        if (res == upperBounds.length) {
            return new Range<>(upperBounds[res - 1], null, extraValue);
        }

        return new Range<>(
                res > 0 ? upperBounds[res - 1] : null,
                upperBounds[res],
                upperBoundsMapping.get(upperBounds[res])
        );
    }

    public static class Range<V> {
        @Nullable
        private final Integer upperEndpoint;
        @Nullable
        private final Integer lowerEndpoint;
        private final V value;

        /**
         * @param lowerEndpoint - inclusive
         * @param upperEndpoint - exclusive
         * @param value         - mapped value
         */
        public Range(@Nullable Integer lowerEndpoint, @Nullable Integer upperEndpoint, V value) {
            this.upperEndpoint = upperEndpoint;
            this.lowerEndpoint = lowerEndpoint;
            this.value = value;
        }

        /**
         * Upper endpoint
         *
         * @return exclusive upper endpoint
         */
        @Nullable
        public Integer getUpperEndpoint() {
            return upperEndpoint;
        }

        /**
         * Lower endpoint
         *
         * @return inclusive lower endpoint
         */
        @Nullable
        public Integer getLowerEndpoint() {
            return lowerEndpoint;
        }

        public V getValue() {
            return value;
        }
    }
}
