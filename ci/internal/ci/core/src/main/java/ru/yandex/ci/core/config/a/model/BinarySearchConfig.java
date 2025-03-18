package ru.yandex.ci.core.config.a.model;

import java.time.Duration;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.core.time.DurationDeserializer;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
@JsonDeserialize(builder = BinarySearchConfig.Builder.class)
public class BinarySearchConfig {

    @Nullable
    @JsonProperty("min-interval-duration")
    @JsonDeserialize(converter = DurationDeserializer.class)
    @JsonSerialize(converter = DurationSerializer.class)
    Duration minIntervalDuration;

    @Nullable
    @JsonProperty("min-interval-size")
    Integer minIntervalSize;

    @Nonnull
    @JsonProperty("close-intervals-older-than")
    @JsonDeserialize(converter = DurationDeserializer.class)
    @JsonSerialize(converter = DurationSerializer.class)
    Duration closeIntervalsOlderThan;

    public static class Builder {
        {
            closeIntervalsOlderThan = Duration.ofDays(14);
        }
    }

}
