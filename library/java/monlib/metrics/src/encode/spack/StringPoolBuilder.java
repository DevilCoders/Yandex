package ru.yandex.monlib.metrics.encode.spack;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Consumer;

import javax.annotation.ParametersAreNonnullByDefault;


/**
 * @author Sergey Polovko
 */
public class StringPoolBuilder {

    private final Map<String, PooledString> stringsMap = new HashMap<>();
    private PooledString[] stringsArray;
    private boolean sorted = false;
    private int bytesSize;

    public PooledString putIfAbsent(String value) {
        if (sorted) {
            throw new IllegalStateException("strings already sorted");
        }
        PooledString stats = stringsMap.get(value);
        if (stats == null) {
            stringsMap.put(value, stats = new PooledString(value));
            bytesSize += value.length();
        } else {
            stats.frequency++;
        }
        return stats;
    }

    public void sortByFrequencies() {
        if (sorted) {
            throw new IllegalStateException("strings already sorted");
        }

        int i = 0;
        stringsArray = new PooledString[stringsMap.size()];
        for (PooledString s : stringsMap.values()) {
            stringsArray[i++] = s;
        }

        Arrays.sort(stringsArray);
        for (i = 0; i < stringsArray.length; i++) {
            stringsArray[i].index = i;
        }
        sorted = true;
    }

    public void forEachString(Consumer<String> consumer) {
        if (!sorted) {
            throw new IllegalStateException("strings not yet sorted");
        }
        for (PooledString stats : stringsArray) {
            consumer.accept(stats.value);
        }
    }

    public int getBytesSize() {
        return bytesSize;
    }

    public int size() {
        return stringsArray.length;
    }

    @ParametersAreNonnullByDefault
    public static final class PooledString implements Comparable<PooledString> {
        public String value;
        public int frequency = 1;
        public int index = -1;

        public PooledString(String value) {
            this.value = value;
        }

        @Override
        public int compareTo(PooledString rhs) {
            // reversed order
            return Integer.compare(rhs.frequency, frequency);
        }

        @Override
        public String toString() {
            return String.format("%s {freq: %d, idx: %d}", value, frequency, index);
        }
    }
}
