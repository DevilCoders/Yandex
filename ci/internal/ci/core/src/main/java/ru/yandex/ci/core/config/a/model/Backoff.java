package ru.yandex.ci.core.config.a.model;

import java.time.Duration;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Preconditions;
import org.apache.commons.lang3.ObjectUtils;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum Backoff {
    @JsonProperty("const")
    @JsonAlias("CONSTANT") // For old settings in database
    CONSTANT {
        @Override
        public Duration next(Duration base, int attempt) {
            return base;
        }
    },
    @JsonProperty("exp")
    @JsonAlias("EXPONENTIAL") // For old settings in database
    EXPONENTIAL {
        @Override
        public Duration next(Duration base, int attempt) {
            if (attempt > 62) {
                // 1 наносекунда через 62 попытки = 53к дней
                return MAX_DURATION;
            }
            try {
                return base.multipliedBy(1L << (attempt - 1));
            } catch (ArithmeticException e) {
                return MAX_DURATION;
            }
        }
    };

    private static final Duration MAX_DURATION = Duration.ofDays(54_000);

    protected abstract Duration next(Duration base, int attempt);

    public Duration next(Duration base, int attempt, @Nullable Duration maxTimeout) {
        Preconditions.checkArgument(attempt >= 1 && attempt <= 1_000);
        var next = next(base, attempt);
        if (maxTimeout != null) {
            return ObjectUtils.min(next, maxTimeout);
        }
        return next;
    }
}
