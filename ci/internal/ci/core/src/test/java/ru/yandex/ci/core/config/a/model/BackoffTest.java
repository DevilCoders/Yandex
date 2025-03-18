package ru.yandex.ci.core.config.a.model;

import java.time.Duration;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class BackoffTest {
    @Test
    void exp() {
        assertThat(Backoff.EXPONENTIAL.next(Duration.ofSeconds(20), 1)).isEqualTo(Duration.ofSeconds(20));
        assertThat(Backoff.EXPONENTIAL.next(Duration.ofSeconds(20), 2)).isEqualTo(Duration.ofSeconds(40));
        assertThat(Backoff.EXPONENTIAL.next(Duration.ofSeconds(20), 3)).isEqualTo(Duration.ofSeconds(80));
    }

    @Test
    void constant() {
        assertThat(Backoff.CONSTANT.next(Duration.ofSeconds(20), 1)).isEqualTo(Duration.ofSeconds(20));
        assertThat(Backoff.CONSTANT.next(Duration.ofSeconds(20), 2)).isEqualTo(Duration.ofSeconds(20));
        assertThat(Backoff.CONSTANT.next(Duration.ofSeconds(20), 3)).isEqualTo(Duration.ofSeconds(20));
    }

    @Test
    void limit() {
        var at60 = Backoff.EXPONENTIAL.next(Duration.ofNanos(1), 60);
        var at61 = Backoff.EXPONENTIAL.next(Duration.ofNanos(1), 61);
        var at62 = Backoff.EXPONENTIAL.next(Duration.ofNanos(1), 62);
        assertThat(at60).isLessThan(Duration.ofDays(54_000));
        assertThat(at61).isGreaterThan(at60).isLessThan(Duration.ofDays(54_000));
        assertThat(at62).isGreaterThan(at61).isLessThan(Duration.ofDays(54_000));
        assertThat(Backoff.EXPONENTIAL.next(Duration.ofNanos(1), 63)).isEqualTo(Duration.ofDays(54_000));
        assertThat(Backoff.EXPONENTIAL.next(Duration.ofNanos(1), 1_000, Duration.ofDays(7)))
                .isEqualTo(Duration.ofDays(7));
    }
}
