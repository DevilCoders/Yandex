package ru.yandex.ci.engine.autocheck.model;

import java.util.Arrays;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import lombok.Getter;

@Getter
public class BalancerConfig {
    private Map<Integer, Map<Integer, String[]>> partitions;

    public Set<String> getBiggestPartition() {
        int max = partitions.keySet().stream()
                .mapToInt(Integer::intValue)
                .max()
                .orElseThrow(() -> new IllegalStateException("No partitions in the config"));
        return partitions.get(max).values().stream()
                .flatMap(Arrays::stream)
                .collect(Collectors.toUnmodifiableSet());
    }
}
