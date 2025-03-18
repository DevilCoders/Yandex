package ru.yandex.ci.observer.api.statistics.model;

import java.util.Map;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

@Value
public class Sensor {
    Map<String, String> labels;
    Double value;
    @Nullable
    Long ts;

    public Sensor(@Nonnull Map<String, String> labels, @Nonnull Double value) {
        this(labels, value, null);
    }

    private Sensor(@Nonnull Map<String, String> labels, @Nonnull Double value, @Nullable Long ts) {
        this.labels = labels;
        this.value = value;
        this.ts = ts;
    }
}
