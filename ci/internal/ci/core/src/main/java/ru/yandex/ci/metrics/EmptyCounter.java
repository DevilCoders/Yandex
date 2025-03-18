package ru.yandex.ci.metrics;

import io.micrometer.core.instrument.Counter;

public class EmptyCounter implements Counter {
    @Override
    public void increment(double amount) {

    }

    @Override
    public double count() {
        return 0;
    }

    @Override
    public Id getId() {
        return null;
    }
}
