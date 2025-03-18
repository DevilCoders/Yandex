package ru.yandex.ci.core.config.a.model.auto;

import java.time.Duration;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Value;

import ru.yandex.ci.core.config.a.model.DurationSerializer;
import ru.yandex.ci.core.time.DurationDeserializer;

@Value
public class Conditions {
    @Nullable
    @JsonProperty("min-commits")
    Integer minCommits;

    @Nullable
    Schedule schedule;

    @Nullable
    @JsonDeserialize(converter = DurationDeserializer.class)
    @JsonSerialize(converter = DurationSerializer.class)
    @JsonProperty("since-last-release")
    Duration sinceLastRelease;
}
