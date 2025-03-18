package ru.yandex.ci.core.config.registry.sandbox;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.ToString;
import lombok.With;

import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings("ReferenceEquality")
@Persisted
@EqualsAndHashCode
@ToString
@Getter
@NoArgsConstructor(access = AccessLevel.PRIVATE)
@AllArgsConstructor
public class SandboxSemaphoreAcquireConfig {

    @With
    @JsonProperty
    String name;

    @JsonProperty
    Long weight;

    @JsonProperty
    Long capacity;
}
