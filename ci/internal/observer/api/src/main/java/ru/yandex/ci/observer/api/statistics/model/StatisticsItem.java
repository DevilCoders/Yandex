package ru.yandex.ci.observer.api.statistics.model;

import java.util.HashMap;
import java.util.Map;

import javax.annotation.Nonnull;

import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.ToString;

@ToString
@EqualsAndHashCode
@Getter
public abstract class StatisticsItem {
    private final Map<String, String> labels;
    private final double value;

    public StatisticsItem(@Nonnull String sensor, @Nonnull Map<String, String> labels, double value) {
        this.labels = new HashMap<>(labels);
        this.labels.put("sensor", sensor);
        this.value = value;
    }

    public Sensor toSensor() {
        return new Sensor(labels, value);
    }
}
