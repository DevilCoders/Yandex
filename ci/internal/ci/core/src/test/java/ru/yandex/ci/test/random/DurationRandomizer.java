package ru.yandex.ci.test.random;

import java.time.Duration;
import java.time.temporal.ChronoUnit;

import io.github.benas.randombeans.api.Randomizer;
import io.github.benas.randombeans.randomizers.range.IntegerRangeRandomizer;

public class DurationRandomizer implements Randomizer<Duration> {

    private final IntegerRangeRandomizer random;

    public DurationRandomizer(long seed) {
        random = new IntegerRangeRandomizer(1, (int) Duration.ofDays(15).toSeconds(), seed);
    }

    @Override
    public Duration getRandomValue() {
        return Duration.of(random.getRandomValue(), ChronoUnit.SECONDS);
    }
}
