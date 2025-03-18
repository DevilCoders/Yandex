package ru.yandex.ci.observer.api.statistics.aggregated;

import java.util.Collection;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.base.Preconditions;
import lombok.Value;

@Value
public class TimingPercentiles {
    Set<Integer> levels;

    public TimingPercentiles(Collection<Integer> levels) {
        Preconditions.checkArgument(
                levels.stream().allMatch(l -> l >= 1 && l <= 100), "Percentile levels are allowed only from 1 to 100"
        );

        this.levels = Set.copyOf(levels);
    }

    public Map<Integer, Long> compute(Collection<Long> values) {
        return compute(values, 0L);
    }

    private <T extends Number> Map<Integer, T> compute(Collection<T> values, T zeroValue) {
        var sorted = values.stream().sorted().collect(Collectors.toList());
        return levels.stream()
                .collect(Collectors.toMap(
                        Function.identity(),
                        l -> {
                            if (sorted.isEmpty()) {
                                return zeroValue;
                            }

                            return sorted.get(Math.min((sorted.size() * l) / 100, sorted.size() - 1));
                        }
                ));
    }
}
